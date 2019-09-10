/*
 * Copyright (C) 2006, Jamie McCracken <jamiemcc@gnome.org>
 * Copyright (C) 2008, Nokia <ivan.frade@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "config-miners.h"

#include <libtracker-miners-common/tracker-common.h>

#include <libtracker-extract/tracker-extract.h>

#include "tracker-main.h"
#include "tracker-gsf.h"
#include "tracker-read.h"

#include <unistd.h>

typedef enum {
	ODT_TAG_TYPE_UNKNOWN,
	ODT_TAG_TYPE_TITLE,
	ODT_TAG_TYPE_SUBJECT,
	ODT_TAG_TYPE_AUTHOR,
	ODT_TAG_TYPE_KEYWORDS,
	ODT_TAG_TYPE_COMMENTS,
	ODT_TAG_TYPE_STATS,
	ODT_TAG_TYPE_CREATED,
	ODT_TAG_TYPE_GENERATOR,
	ODT_TAG_TYPE_WORD_TEXT,
	ODT_TAG_TYPE_WORD_TABLE_CELL,
	ODT_TAG_TYPE_SLIDE_TEXT,
	ODT_TAG_TYPE_SPREADSHEET_TEXT,
	ODT_TAG_TYPE_GRAPHICS_TEXT
} ODTTagType;

typedef enum {
	FILE_TYPE_INVALID,
	FILE_TYPE_ODP,
	FILE_TYPE_ODT,
	FILE_TYPE_ODS,
	FILE_TYPE_ODG
} ODTFileType;

typedef struct {
	TrackerResource *metadata;
	GQueue *tag_stack;            /* (element-type: ODTTagType) */
	const gchar *uri;
	guint has_title           : 1;
	guint has_subject         : 1;
	guint has_publisher       : 1;
	guint has_comment         : 1;
	guint has_generator       : 1;
	guint has_word_count      : 1;
	guint has_page_count      : 1;
	guint has_content_created : 1;
} ODTMetadataParseInfo;

typedef struct {
	GQueue *tag_stack;            /* (element-type: ODTTagType) */
	ODTFileType file_type;
	GString *content;
	gulong bytes_pending;
} ODTContentParseInfo;

GQuark maximum_size_error_quark = 0;

static void xml_start_element_handler_metadata (GMarkupParseContext   *context,
                                                const gchar           *element_name,
                                                const gchar          **attribute_names,
                                                const gchar          **attribute_values,
                                                gpointer               user_data,
                                                GError               **error);
static void xml_end_element_handler_metadata   (GMarkupParseContext   *context,
                                                const gchar           *element_name,
                                                gpointer               user_data,
                                                GError               **error);
static void xml_text_handler_metadata          (GMarkupParseContext   *context,
                                                const gchar           *text,
                                                gsize                  text_len,
                                                gpointer               user_data,
                                                GError               **error);
static void xml_start_element_handler_content  (GMarkupParseContext   *context,
                                                const gchar           *element_name,
                                                const gchar          **attribute_names,
                                                const gchar          **attribute_values,
                                                gpointer               user_data,
                                                GError               **error);
static void xml_end_element_handler_content    (GMarkupParseContext   *context,
                                                const gchar           *element_name,
                                                gpointer               user_data,
                                                GError               **error);
static void xml_text_handler_content           (GMarkupParseContext   *context,
                                                const gchar           *text,
                                                gsize                  text_len,
                                                gpointer               user_data,
                                                GError               **error);
static void extract_oasis_content              (const gchar           *uri,
                                                gulong                 total_bytes,
                                                ODTFileType            file_type,
                                                TrackerResource       *metadata);

static void
extract_oasis_content (const gchar     *uri,
                       gulong           total_bytes,
                       ODTFileType      file_type,
                       TrackerResource *metadata)
{
	gchar *content = NULL;
	ODTContentParseInfo info;
	GMarkupParseContext *context;
	GError *error = NULL;
	GMarkupParser parser = {
		xml_start_element_handler_content,
		xml_end_element_handler_content,
		xml_text_handler_content,
		NULL,
		NULL
	};

	/* If no content requested, return */
	if (total_bytes == 0) {
		return;
	}

	/* Create parse info */
	info.tag_stack = g_queue_new ();
	info.file_type = file_type;
	info.content = g_string_new ("");
	info.bytes_pending = total_bytes;

	/* Create parsing context */
	context = g_markup_parse_context_new (&parser, 0, &info, NULL);

	/* Load the internal XML file from the Zip archive, and parse it
	 * using the given context */
	tracker_gsf_parse_xml_in_zip (uri, "content.xml", context, &error);

	if (!error || g_error_matches (error, maximum_size_error_quark, 0)) {
		content = g_string_free (info.content, FALSE);
		tracker_resource_set_string (metadata, "nie:plainTextContent", content);
	} else {
		g_warning ("Got error parsing XML file: %s\n", error->message);
		g_string_free (info.content, TRUE);
	}

	if (error) {
		g_error_free (error);
	}

	g_free (content);
	g_markup_parse_context_free (context);
	g_queue_free (info.tag_stack);
}

G_MODULE_EXPORT gboolean
tracker_extract_get_metadata (TrackerExtractInfo *extract_info)
{
	TrackerResource *metadata;
	TrackerConfig *config;
	ODTMetadataParseInfo info = { 0 };
	ODTFileType file_type;
	GFile *file;
	gchar *uri;
	const gchar *mime_used;
	GMarkupParseContext *context;
	GMarkupParser parser = {
		xml_start_element_handler_metadata,
		xml_end_element_handler_metadata,
		xml_text_handler_metadata,
		NULL,
		NULL
	};

	if (G_UNLIKELY (maximum_size_error_quark == 0)) {
		maximum_size_error_quark = g_quark_from_static_string ("maximum_size_error");
	}

	metadata = tracker_resource_new (NULL);
	mime_used = tracker_extract_info_get_mimetype (extract_info);

	file = tracker_extract_info_get_file (extract_info);
	uri = g_file_get_uri (file);

	/* Setup conf */
	config = tracker_main_get_config ();

	g_debug ("Extracting OASIS metadata and contents from '%s'", uri);

	/* First, parse metadata */

	tracker_resource_add_uri (metadata, "rdf:type", "nfo:PaginatedTextDocument");

	/* Create parse info */
	info.metadata = metadata;
	info.tag_stack = g_queue_new ();
	info.uri = uri;

	/* Create parsing context */
	context = g_markup_parse_context_new (&parser, 0, &info, NULL);

	/* Load the internal XML file from the Zip archive, and parse it
	 * using the given context */
	tracker_gsf_parse_xml_in_zip (uri, "meta.xml", context, NULL);
	g_markup_parse_context_free (context);

	if (g_ascii_strcasecmp (mime_used, "application/vnd.oasis.opendocument.text") == 0) {
		file_type = FILE_TYPE_ODT;
	} else if (g_ascii_strcasecmp (mime_used, "application/vnd.oasis.opendocument.presentation") == 0) {
		file_type = FILE_TYPE_ODP;
	} else if (g_ascii_strcasecmp (mime_used, "application/vnd.oasis.opendocument.spreadsheet") == 0) {
		file_type = FILE_TYPE_ODS;
	} else if (g_ascii_strcasecmp (mime_used, "application/vnd.oasis.opendocument.graphics") == 0) {
		file_type = FILE_TYPE_ODG;
	} else {
		g_message ("Mime type was not recognised:'%s'", mime_used);
		file_type = FILE_TYPE_INVALID;
	}

	/* Extract content with the given limitations */
	extract_oasis_content (uri,
	                       tracker_config_get_max_bytes (config),
	                       file_type,
	                       metadata);

	g_queue_free (info.tag_stack);

	g_free (uri);

	tracker_extract_info_set_resource (extract_info, metadata);
	g_object_unref (metadata);

	return TRUE;
}

static void
xml_start_element_handler_metadata (GMarkupParseContext  *context,
                                    const gchar          *element_name,
                                    const gchar         **attribute_names,
                                    const gchar         **attribute_values,
                                    gpointer              user_data,
                                    GError              **error)
{
	ODTMetadataParseInfo *data = user_data;

	#define push_tag(id)                                                             \
		g_queue_push_head (data->tag_stack, GINT_TO_POINTER(id));

	#define handle_tag_and_return(name, id)                                          \
		if (g_ascii_strcasecmp (element_name, name) == 0) {                          \
			push_tag (id);                                                           \
			return;                                                                  \
		};

	handle_tag_and_return ("dc:title", ODT_TAG_TYPE_TITLE);
	handle_tag_and_return ("dc:subject", ODT_TAG_TYPE_SUBJECT);
	handle_tag_and_return ("dc:creator", ODT_TAG_TYPE_AUTHOR);
	handle_tag_and_return ("meta:keyword", ODT_TAG_TYPE_KEYWORDS);
	handle_tag_and_return ("dc:description", ODT_TAG_TYPE_COMMENTS);
	handle_tag_and_return ("meta:creation-date", ODT_TAG_TYPE_CREATED);
	handle_tag_and_return ("meta:generator", ODT_TAG_TYPE_GENERATOR);

	if (g_ascii_strcasecmp (element_name, "meta:document-statistic") == 0) {
		TrackerResource *metadata;
		const gchar **a, **v;

		metadata = data->metadata;

		for (a = attribute_names, v = attribute_values; *a; ++a, ++v) {
			if (g_ascii_strcasecmp (*a, "meta:word-count") == 0) {
				if (data->has_word_count) {
					g_warning ("Avoiding additional word count (%s) in OASIS document '%s'",
					           *v, data->uri);
				} else {
					data->has_word_count = TRUE;
					tracker_resource_set_string (metadata, "nfo:wordCount", *v);
				}
			} else if (g_ascii_strcasecmp (*a, "meta:page-count") == 0) {
				if (data->has_page_count) {
					g_warning ("Avoiding additional page count (%s) in OASIS document '%s'",
					           *v, data->uri);
				} else {
					data->has_page_count = TRUE;
					tracker_resource_set_string (metadata, "nfo:pageCount", *v);
				}
			}
		}

		push_tag (ODT_TAG_TYPE_STATS);

		return;
	}

	push_tag (ODT_TAG_TYPE_UNKNOWN);

	#undef push_tag
	#undef handle_tag
}

static void
xml_end_element_handler_metadata (GMarkupParseContext  *context,
                                  const gchar          *element_name,
                                  gpointer              user_data,
                                  GError              **error)
{
	ODTMetadataParseInfo *data = user_data;

	g_queue_pop_head (data->tag_stack);
}

static void
xml_text_handler_metadata (GMarkupParseContext  *context,
                           const gchar          *text,
                           gsize                 text_len,
                           gpointer              user_data,
                           GError              **error)
{
	ODTMetadataParseInfo *data;
	ODTTagType current;
	TrackerResource *metadata;
	gchar *date;

	data = user_data;
	metadata = data->metadata;

	if (text_len == 0) {
		/* ignore empty values */
		return;
	}

	current = GPOINTER_TO_INT (g_queue_peek_head (data->tag_stack));
	switch (current) {
	case ODT_TAG_TYPE_TITLE:
		if (data->has_title) {
			g_warning ("Avoiding additional title (%s) in OASIS document '%s'",
			           text, data->uri);
		} else {
			data->has_title = TRUE;
			tracker_resource_set_string (metadata, "nie:title", text);
		}
		break;

	case ODT_TAG_TYPE_SUBJECT:
		if (data->has_subject) {
			g_warning ("Avoiding additional subject (%s) in OASIS document '%s'",
			           text, data->uri);
		} else {
			data->has_subject = TRUE;
			tracker_resource_set_string (metadata, "nie:subject", text);
		}
		break;

	case ODT_TAG_TYPE_AUTHOR:
		if (data->has_publisher) {
			g_warning ("Avoiding additional publisher (%s) in OASIS document '%s'",
			           text, data->uri);
		} else {
			TrackerResource *publisher = tracker_extract_new_contact (text);

			data->has_publisher = TRUE;
			tracker_resource_set_relation (metadata, "nco:publisher", publisher);

			g_object_unref (publisher);
		}
		break;

	case ODT_TAG_TYPE_KEYWORDS: {
		gchar *keywords;
		gchar *lasts, *keyw;

		keywords = g_strdup (text);

		for (keyw = strtok_r (keywords, ",; ", &lasts);
		     keyw;
		     keyw = strtok_r (NULL, ",; ", &lasts)) {
			tracker_resource_add_string (metadata, "nie:keyword", keyw);
		}

		g_free (keywords);

		break;
	}

	case ODT_TAG_TYPE_COMMENTS:
		if (data->has_comment) {
			g_warning ("Avoiding additional comment (%s) in OASIS document '%s'",
			           text, data->uri);
		} else {
			data->has_comment = TRUE;
			tracker_resource_set_string (metadata, "nie:comment", text);
		}
		break;

	case ODT_TAG_TYPE_CREATED:
		if (data->has_content_created) {
			g_warning ("Avoiding additional creation time (%s) in OASIS document '%s'",
			           text, data->uri);
		} else {
			date = tracker_date_guess (text);
			if (date) {
				data->has_content_created = TRUE;
				tracker_resource_set_string (metadata, "nie:contentCreated", date);
				g_free (date);
			} else {
				g_warning ("Could not parse creation time (%s) in OASIS document '%s'",
				           text, data->uri);
			}
		}
		break;

	case ODT_TAG_TYPE_GENERATOR:
		if (data->has_generator) {
			g_warning ("Avoiding additional creation time (%s) in OASIS document '%s'",
			           text, data->uri);
		} else {
			data->has_generator = TRUE;
			tracker_resource_set_string (metadata, "nie:generator", text);
		}
		break;

	default:
	case ODT_TAG_TYPE_STATS:
		break;
	}
}

static void
xml_start_element_handler_content (GMarkupParseContext  *context,
                                   const gchar          *element_name,
                                   const gchar         **attribute_names,
                                   const gchar         **attribute_values,
                                   gpointer              user_data,
                                   GError              **error)
{
	ODTContentParseInfo *data = user_data;

	#define push_tag(id)                                                             \
		g_queue_push_head (data->tag_stack, GINT_TO_POINTER(id));

	#define handle_tag_and_return(name, id)                                          \
		if (g_ascii_strcasecmp (element_name, name) == 0) {                          \
			push_tag (id);                                                           \
			return;                                                                  \
		};

	switch (data->file_type) {
	case FILE_TYPE_ODT:
		handle_tag_and_return ("text:p", ODT_TAG_TYPE_WORD_TEXT);
		handle_tag_and_return ("text:h", ODT_TAG_TYPE_WORD_TEXT);
		handle_tag_and_return ("text:a", ODT_TAG_TYPE_WORD_TEXT);
		handle_tag_and_return ("text:span", ODT_TAG_TYPE_WORD_TEXT);
		handle_tag_and_return ("text:s", ODT_TAG_TYPE_WORD_TEXT);
		handle_tag_and_return ("text:tab", ODT_TAG_TYPE_WORD_TEXT);
		handle_tag_and_return ("text:line-break", ODT_TAG_TYPE_WORD_TEXT);
		handle_tag_and_return ("table:table-cell", ODT_TAG_TYPE_WORD_TABLE_CELL);

		push_tag (ODT_TAG_TYPE_UNKNOWN);
		return;

	case FILE_TYPE_ODP:
		push_tag (ODT_TAG_TYPE_SLIDE_TEXT);
		return;

	case FILE_TYPE_ODS:
		handle_tag_and_return ("text", ODT_TAG_TYPE_SPREADSHEET_TEXT);
		push_tag (ODT_TAG_TYPE_UNKNOWN);
		return;

	case FILE_TYPE_ODG:
		handle_tag_and_return ("text", ODT_TAG_TYPE_GRAPHICS_TEXT);
		push_tag (ODT_TAG_TYPE_UNKNOWN);
		return;

	case FILE_TYPE_INVALID:
		g_message ("Open Office Document type: %d invalid", data->file_type);
		push_tag (ODT_TAG_TYPE_UNKNOWN);
		return;
	}

	#undef push_tag
	#undef handle_tag
}

static void
xml_end_element_handler_content (GMarkupParseContext  *context,
                                 const gchar          *element_name,
                                 gpointer              user_data,
                                 GError              **error)
{
	ODTContentParseInfo *data = user_data;

	g_queue_pop_head (data->tag_stack);
}

static void
xml_text_handler_content (GMarkupParseContext  *context,
                          const gchar          *text,
                          gsize                 text_len,
                          gpointer              user_data,
                          GError              **error)
{
	ODTContentParseInfo *data = user_data;
	ODTTagType current;
	gsize written_bytes = 0;

	current = GPOINTER_TO_INT (g_queue_peek_head (data->tag_stack));
	switch (current) {
	case ODT_TAG_TYPE_WORD_TEXT:
	case ODT_TAG_TYPE_WORD_TABLE_CELL:
	case ODT_TAG_TYPE_SLIDE_TEXT:
	case ODT_TAG_TYPE_SPREADSHEET_TEXT:
	case ODT_TAG_TYPE_GRAPHICS_TEXT:
		if (data->bytes_pending == 0) {
			g_set_error_literal (error,
			                     maximum_size_error_quark, 0,
			                     "Maximum text limit reached");
			break;
		}

		/* Look for valid UTF-8 text */
		if (tracker_text_validate_utf8 (text,
		                                MIN (text_len, data->bytes_pending),
		                                &data->content,
		                                &written_bytes)) {
			/* We found valid text! */

			if (data->content->str[data->content->len - 1] != ' ') {
				if (current == ODT_TAG_TYPE_WORD_TEXT) {
					/* We're inside a text field in a word document, so trust
					 * the spacing given. Tag boundries mark things like bold
					 * and italic spans.
					 */
				} else {
					/* We don't know the context, we may be combining text from
					 * multiple spreadsheet cells for example, so make sure the
					 * text ends with a space.
					 */
					g_string_append_c (data->content, ' ');
				}
			}
		}

		data->bytes_pending -= written_bytes;
		break;

	default:
		break;
	}
}
