/*
 * Copyright (C) 2019, Saiful B. Khan <saifulbkhan.gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "config-miners.h"

#include <stdio.h>
#include <string.h>

#include <gst/gst.h>
#include <gst/tag/tag.h>

#include <glib/gi18n.h>
#include <gio/gio.h>

#include <libtracker-sparql/tracker-ontologies.h>
#include <libtracker-sparql/tracker-sparql.h>

#include "tracker-writeback-file.h"

#define TRACKER_TYPE_WRITEBACK_GSTREAMER (tracker_writeback_gstreamer_get_type ())

typedef struct TrackerWritebackGstreamer         TrackerWritebackGstreamer;
typedef struct TrackerWritebackGstreamerClass    TrackerWritebackGstreamerClass;
typedef struct TrackerWritebackGstreamerElements TagElements;

typedef enum {
	GST_AUTOPLUG_SELECT_TRY,
	GST_AUTOPLUG_SELECT_EXPOSE,
	GST_AUTOPLUG_SELECT_SKIP
} GstAutoplugSelectResult;

typedef GstElement *(*TrackerWritebackGstAddTaggerElem) (GstElement *pipeline,
                                                         GstPad     *srcpad,
                                                         GstTagList *tags);

struct TrackerWritebackGstreamerElements {
	GstElement *pipeline;
	GstElement *sink;
	GHashTable *taggers;
	GstTagList *tags;
	gboolean sink_linked;
};

struct TrackerWritebackGstreamer {
	TrackerWritebackFile parent_instance;
};

struct TrackerWritebackGstreamerClass {
	TrackerWritebackFileClass parent_class;
};

static GType               tracker_writeback_gstreamer_get_type     (void) G_GNUC_CONST;
static gboolean            writeback_gstreamer_write_file_metadata  (TrackerWritebackFile    *writeback_file,
                                                                     GFile                   *file,
                                                                     TrackerResource         *resource,
                                                                     GCancellable            *cancellable,
                                                                     GError                 **error);
static const gchar* const *writeback_gstreamer_content_types        (TrackerWritebackFile    *writeback_file);

G_DEFINE_DYNAMIC_TYPE (TrackerWritebackGstreamer, tracker_writeback_gstreamer, TRACKER_TYPE_WRITEBACK_FILE);

static void
tracker_writeback_gstreamer_class_init (TrackerWritebackGstreamerClass *klass)
{
	TrackerWritebackFileClass *writeback_file_class = TRACKER_WRITEBACK_FILE_CLASS (klass);

	gst_init (NULL, NULL);

	writeback_file_class->write_file_metadata = writeback_gstreamer_write_file_metadata;
	writeback_file_class->content_types = writeback_gstreamer_content_types;
}

static void
tracker_writeback_gstreamer_class_finalize (TrackerWritebackGstreamerClass *klass)
{
}

static void
tracker_writeback_gstreamer_init (TrackerWritebackGstreamer *wbg)
{
}

static const gchar* const *
writeback_gstreamer_content_types (TrackerWritebackFile *writeback_file)
{
	static const gchar *content_types[] = {
		"audio/flac",
		"audio/x-flac",
		"audio/mpeg",
		"audio/x-mpeg",
		"audio/mp3",
		"audio/x-mp3",
		"audio/mpeg3",
		"audio/x-mpeg3",
		"audio/x-ac3",
		"audio/ogg",
		"audio/x-ogg",
		"audio/x-vorbis+ogg",
		NULL
	};

	return content_types;
}

static gboolean
link_named_pad (GstPad      *srcpad,
                GstElement  *element,
                const gchar *sinkpadname)
{
	GstPad *sinkpad;
	GstPadLinkReturn result;

	sinkpad = gst_element_get_static_pad (element, sinkpadname);
	if (sinkpad == NULL) {
		sinkpad = gst_element_get_request_pad (element, sinkpadname);
	}
	result = gst_pad_link (srcpad, sinkpad);
	gst_object_unref (sinkpad);

	if (GST_PAD_LINK_SUCCESSFUL (result)) {
		return TRUE;
	} else {
		gchar *srcname = gst_pad_get_name (srcpad);
		gchar *sinkname = gst_pad_get_name (sinkpad);
		g_warning ("couldn't link %s to %s: %d", srcname, sinkname, result);
		return FALSE;
	}
}

static GstElement *
flac_tagger (GstElement *pipeline,
             GstPad     *srcpad,
             GstTagList *tags)
{
	GstElement *tagger = NULL;

	tagger = gst_element_factory_make ("flactag", NULL);
	if (tagger == NULL)
		return NULL;

	gst_bin_add (GST_BIN (pipeline), tagger);
	if (!link_named_pad (srcpad, tagger, "sink"))
		return NULL;

	gst_element_set_state (tagger, GST_STATE_PAUSED);
	if (tags != NULL) {
		gst_tag_setter_merge_tags (GST_TAG_SETTER (tagger), tags, GST_TAG_MERGE_REPLACE_ALL);
	}
	return tagger;
}

static GstElement *
mp3_tagger (GstElement *pipeline,
            GstPad     *srcpad,
            GstTagList *tags)
{
	GstElement *mux = NULL;

	/* try id3mux first, since it's more supported and
	 * writes id3v2.3 tags rather than v2.4. */
	mux = gst_element_factory_make ("id3mux", NULL);
	if (mux == NULL)
		mux = gst_element_factory_make ("id3v2mux", NULL);

	if (mux == NULL)
		return NULL;

	gst_bin_add (GST_BIN (pipeline), mux);
	if (!link_named_pad (srcpad, mux, "sink")) {
		g_warning ("couldn't link decoded pad to id3 muxer");
		return NULL;
	}

	gst_element_set_state (mux, GST_STATE_PAUSED);
	if (tags != NULL) {
		gst_tag_setter_merge_tags (GST_TAG_SETTER (mux), tags, GST_TAG_MERGE_REPLACE_ALL);
	}
	g_debug ("id3 tagger created");
	return mux;
}

static GstElement *
vorbis_tagger (GstElement *pipeline,
               GstPad     *srcpad,
               GstTagList *tags)
{
	GstElement *mux;
	GstElement *tagger;
	GstElement *parser;

	mux = gst_element_factory_make ("oggmux", NULL);
	parser = gst_element_factory_make ("vorbisparse", NULL);
	tagger = gst_element_factory_make ("vorbistag", NULL);
	if (mux == NULL || parser == NULL || tagger == NULL)
		goto error;

	gst_bin_add_many (GST_BIN (pipeline), parser, tagger, mux, NULL);
	if (!link_named_pad (srcpad, parser, "sink"))
		return NULL;
	if (!gst_element_link_many (parser, tagger, mux, NULL))
		return NULL;

	gst_element_set_state (parser, GST_STATE_PAUSED);
	gst_element_set_state (tagger, GST_STATE_PAUSED);
	gst_element_set_state (mux, GST_STATE_PAUSED);
	if (tags != NULL) {
		gst_tag_setter_merge_tags (GST_TAG_SETTER (tagger), tags, GST_TAG_MERGE_REPLACE_ALL);
	}
	return mux;

 error:
	if (parser != NULL)
		g_object_unref (parser);
	if (tagger != NULL)
		g_object_unref (tagger);
	if (mux != NULL)
		g_object_unref (mux);
	return NULL;
}

static GstElement *
mp4_tagger (GstElement *pipeline,
            GstPad     *srcpad,
            GstTagList *tags)
{
	GstElement *mux;

	mux = gst_element_factory_make ("mp4mux", NULL);
	if (mux == NULL)
		return NULL;

	gst_bin_add (GST_BIN (pipeline), mux);
	if (!link_named_pad (srcpad, mux, "audio_%u"))
		return NULL;

	gst_element_set_state (mux, GST_STATE_PAUSED);
	if (tags != NULL) {
		gst_tag_setter_merge_tags (GST_TAG_SETTER (mux), tags, GST_TAG_MERGE_REPLACE_ALL);
	}
	return mux;
}

static void
pad_added_cb (GstElement  *decodebin,
              GstPad      *pad,
              TagElements *element)
{
	TrackerWritebackGstAddTaggerElem add_tagger_func = NULL;
	GstElement *retag_end;
	GstCaps *caps;
	gchar *caps_str = NULL;
	GHashTableIter iter;
	gpointer key;
	gpointer value;

	if (element->sink_linked) {
		GError *error;
		error = g_error_new (GST_STREAM_ERROR,
		                     GST_STREAM_ERROR_FORMAT,
		                     "Unable to write tags to this file as it contains multiple streams");
		gst_element_post_message (decodebin, gst_message_new_error (GST_OBJECT (decodebin), error, NULL));
		g_error_free (error);
		return;
	}

	/* find a tagger function that accepts the caps */
	caps = gst_pad_query_caps (pad, NULL);
	caps_str = gst_caps_to_string (caps);
	g_debug ("finding tagger for src caps %s", caps_str);
	g_free (caps_str);

	g_hash_table_iter_init (&iter, element->taggers);
	while (g_hash_table_iter_next (&iter, &key, &value)) {
		GstCaps *tagger_caps;
		const gchar *media_type = (const gchar *)key;

		if (strcmp (media_type, "audio/mpeg") == 0)
			tagger_caps = gst_caps_from_string ("audio/mpeg, mpegversion=(int)1");
		else if (strcmp (media_type, "audio/mp4") == 0)
			tagger_caps = gst_caps_from_string ("audio/mpeg, mpegversion=(int){ 2, 4 }");
		else if (strcmp (media_type, "audio/x-ac3") == 0)
			tagger_caps = gst_caps_from_string ("audio/x-ac3, channels=(int)[ 1, 6 ], rate=(int)[ 1, 2147483647 ]");
		else
			tagger_caps = gst_caps_from_string (media_type);

		if (gst_caps_is_always_compatible (caps, tagger_caps)) {
			caps_str = gst_caps_to_string (tagger_caps);
			g_debug ("matched sink caps %s", caps_str);
			g_free (caps_str);

			gst_caps_unref (tagger_caps);
			add_tagger_func = (TrackerWritebackGstAddTaggerElem) value;
			break;
		}
		gst_caps_unref (tagger_caps);
	}
	gst_caps_unref (caps);

	/* add retagging element(s) */
	if (add_tagger_func == NULL) {
		GError *error;
		error = g_error_new (GST_STREAM_ERROR,
		                     GST_STREAM_ERROR_FORMAT,
		                     "Unable to write tags to this file as it is not encoded in a supported format");
		gst_element_post_message (decodebin, gst_message_new_error (GST_OBJECT (decodebin), error, NULL));
		g_error_free (error);
		return;
	}
	retag_end = add_tagger_func (element->pipeline, pad, element->tags);

	/* link to the sink */
	gst_element_link (retag_end, element->sink);
	element->sink_linked = TRUE;
}

static gboolean
factory_src_caps_intersect (GstElementFactory *factory,
                            GstCaps           *caps)
{
	const GList *templates;
	const GList *l;

	templates = gst_element_factory_get_static_pad_templates (factory);
	for (l = templates; l != NULL; l = l->next) {
		GstStaticPadTemplate *t = l->data;
		GstCaps *tcaps;

		if (t->direction != GST_PAD_SRC) {
			continue;
		}

		tcaps = gst_static_pad_template_get_caps (t);
		if (gst_caps_can_intersect (tcaps, caps)) {
			gst_caps_unref (tcaps);
			return TRUE;
		}
		gst_caps_unref (tcaps);
	}
	return FALSE;
}

static GstAutoplugSelectResult
autoplug_select_cb (GstElement        *decodebin,
                    GstPad            *pad,
                    GstCaps           *caps,
                    GstElementFactory *factory,
                    TagElements       *element)
{
	GstCaps *src_caps;
	gboolean is_any;
	gboolean is_raw;
	gboolean is_demuxer;

	is_demuxer = (strstr (gst_element_factory_get_klass (factory), "Demuxer") != NULL);
	if (is_demuxer) {
		/* allow demuxers, since we're going to remux later */
		return GST_AUTOPLUG_SELECT_TRY;
	}

	src_caps = gst_caps_new_any ();
	is_any = gst_element_factory_can_src_all_caps (factory, src_caps);       /* or _any_caps? */
	gst_caps_unref (src_caps);
	if (is_any) {
		/* this is something like id3demux (though that will match the
		 * above check), allow it so we can get to the actual decoder later
		 */
		return GST_AUTOPLUG_SELECT_TRY;
	}

	src_caps = gst_caps_from_string ("audio/x-raw");
	is_raw = factory_src_caps_intersect (factory, src_caps);
	gst_caps_unref (src_caps);

	if (is_raw == FALSE) {
		/*this is probably a parser or something, allow it */
		return GST_AUTOPLUG_SELECT_TRY;
	}

	/* don't allow decoders */
	return GST_AUTOPLUG_SELECT_EXPOSE;
}

static void
writeback_gstreamer_save (TagElements *element,
                          GFile       *file,
                          GError     **error)
{
	GstElement *pipeline = NULL;
	GstElement *urisrc = NULL;
	GstElement *decodebin = NULL;
	GOutputStream *stream = NULL;
	GError *io_error = NULL;
	GstBus *bus;
	gboolean done;
	gchar *uri = g_file_get_uri (file);

	g_debug ("saving metadata for uri: %s", uri);

	stream = G_OUTPUT_STREAM (g_file_replace (file, NULL, FALSE, G_FILE_CREATE_NONE, NULL, &io_error));
	if (io_error != NULL) {
		goto gio_error;
	}

	/* set up pipeline */
	pipeline = gst_pipeline_new ("pipeline");
	element->pipeline = pipeline;
	element->sink_linked = FALSE;

	urisrc = gst_element_make_from_uri (GST_URI_SRC, uri, "urisrc", NULL);
	if (urisrc == NULL) {
		g_warning ("Failed to create gstreamer 'source' element from uri %s", uri);
		goto out;
	}
	decodebin = gst_element_factory_make ("decodebin", "decoder");
	if (decodebin == NULL) {
		g_warning ("Failed to create a 'decodebin' element");
		goto out;
	}

	element->sink = gst_element_factory_make ("giostreamsink", "sink");
	if (element->sink == NULL) {
		g_warning ("Failed to create a 'sink' element");
		goto out;
	}
	g_object_set (element->sink, "stream", stream, NULL);

	gst_bin_add_many (GST_BIN (pipeline), urisrc, decodebin, element->sink, NULL);
	gst_element_link (urisrc, decodebin);

	g_signal_connect_data (decodebin, "pad-added", G_CALLBACK (pad_added_cb), element, NULL, 0);
	g_signal_connect_data (decodebin, "autoplug-select", G_CALLBACK (autoplug_select_cb), element, NULL, 0);

	/* run pipeline .. */
	gst_element_set_state (pipeline, GST_STATE_PLAYING);
	bus = gst_element_get_bus (pipeline);
	done = FALSE;
	while (done == FALSE) {
		GstMessage *message;

		message = gst_bus_timed_pop (bus, GST_CLOCK_TIME_NONE);
		if (message == NULL) {
			g_debug ("breaking out of bus polling loop");
			break;
		}

		switch (GST_MESSAGE_TYPE (message)) {
		case GST_MESSAGE_ERROR:
			{
				GError *gerror;
				gchar *debug;

				gst_message_parse_error (message, &gerror, &debug);
				g_warning ("caught error: %s (%s)", gerror->message, debug);

				g_propagate_error (error, gerror);
				done = TRUE;

				g_free (debug);
			}
			break;

		case GST_MESSAGE_EOS:
			g_debug ("got eos message");
			done = TRUE;
			break;

		default:
			break;
		}

		gst_message_unref (message);
	}
	gst_element_set_state (pipeline, GST_STATE_NULL);

	if (g_output_stream_close (stream, NULL, &io_error) == FALSE) {
		goto gio_error;
	}
	g_object_unref (stream);
	stream = NULL;

	if (*error == NULL)
		goto out;

 gio_error:
	if (io_error != NULL)
		g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "%s", io_error->message);
 out:
	if (pipeline != NULL)
		gst_object_unref (GST_OBJECT (pipeline));
}

static GstSample *
generate_gst_sample_from_image (const GValue *val)
{
	GstSample *img_sample = NULL;
	GMappedFile *mapped_file = NULL;
	GError *err = NULL;
	GByteArray *byte_arr = NULL;
	gchar *filename;
	const gchar *image_url = g_value_get_string (val);

	filename = g_filename_from_uri (image_url, NULL, &err);
	if (err != NULL) {
		g_warning ("could not get filename for url (%s): %s", image_url, err->message);
		g_clear_error (&err);
		return img_sample;
	}

	mapped_file = g_mapped_file_new (filename, TRUE, &err);
	if (err != NULL) {
		g_warning ("encountered error reading image file (%s): %s", filename, err->message);
		g_error_free (err);
	} else {
		GBytes *bytes = g_mapped_file_get_bytes (mapped_file);
		byte_arr = g_bytes_unref_to_array (bytes);
		img_sample = gst_tag_image_data_to_image_sample (byte_arr->data,
		                                                 byte_arr->len, GST_TAG_IMAGE_TYPE_NONE);
	}

	g_byte_array_unref (byte_arr);
	g_mapped_file_unref (mapped_file);
	return img_sample;
}

static gboolean
writeback_gstreamer_set (TagElements  *element,
                         const gchar  *tag,
                         const GValue *val)
{
	GstSample *img_sample;
	GValue newval = {0, };

	if (element->tags == NULL) {
		element->tags = gst_tag_list_new_empty ();
	}

	g_value_init (&newval, gst_tag_get_type (tag));

	if (g_strcmp0 (tag, GST_TAG_DATE_TIME) == 0) {
		GstDateTime *datetime;

		/* assumes date-time in ISO8601 string format */
		datetime = gst_date_time_new_from_iso8601_string (g_value_get_string (val));
		g_value_take_boxed (&newval, datetime);
	} else if (g_strcmp0 (tag, GST_TAG_IMAGE) == 0) {
		img_sample = generate_gst_sample_from_image (val);
		if (img_sample == NULL) {
			g_warning ("failed to set image as tag");
			return FALSE;
		}
		g_value_take_boxed (&newval, img_sample);
	} else {
		g_value_transform (val, &newval);
	}

	g_debug ("Setting %s", tag);
	gst_tag_list_add_values (element->tags, GST_TAG_MERGE_APPEND, tag, &newval, NULL);
	g_value_unset (&newval);

	return TRUE;
}

static void
handle_musicbrainz_tags (TrackerResource     *resource,
			 const gchar         *prop,
			 TagElements         *element,
			 const gchar * const *allowed_tags)
{
	GList *references, *r;

	references = tracker_resource_get_values (resource, prop);

	for (r = references; r; r = r->next) {
		TrackerResource *ref;
		GValue *value, val = G_VALUE_INIT;
		const gchar *source, *identifier;

		value = r->data;

		if (!G_VALUE_HOLDS (value, TRACKER_TYPE_RESOURCE))
			continue;

		ref = g_value_get_object (value);

		source = tracker_resource_get_first_uri (ref, "tracker:referenceSource");
		identifier = tracker_resource_get_first_string (ref, "tracker:referenceIdentifier");

		if (!source || !g_strv_contains (allowed_tags, source))
			continue;

		if (g_strcmp0 (source, "https://musicbrainz.org/doc/Recording") == 0) {
			g_value_init (&val, G_TYPE_STRING);
			g_value_set_string (&val, identifier);
			writeback_gstreamer_set (element, GST_TAG_MUSICBRAINZ_TRACKID, &val);
			g_value_unset (&val);
		} else if (g_strcmp0 (source, "https://musicbrainz.org/doc/Release") == 0) {
			g_value_init (&val, G_TYPE_STRING);
			g_value_set_string (&val, identifier);
			writeback_gstreamer_set (element, GST_TAG_MUSICBRAINZ_ALBUMID, &val);
			g_value_unset (&val);
#ifdef GST_TAG_MUSICBRAINZ_RELEASETRACKID
		} else if (g_strcmp0 (source, "https://musicbrainz.org/doc/Track") == 0) {
			g_value_init (&val, G_TYPE_STRING);
			g_value_set_string (&val, identifier);
			writeback_gstreamer_set (element, GST_TAG_MUSICBRAINZ_RELEASETRACKID, &val);
			g_value_unset (&val);
#endif
#ifdef GST_TAG_MUSICBRAINZ_RELEASEGROUPID
		} else if (g_strcmp0 (source, "https://musicbrainz.org/doc/Release_Group") == 0) {
			g_value_init (&val, G_TYPE_STRING);
			g_value_set_string (&val, identifier);
			writeback_gstreamer_set (element, GST_TAG_MUSICBRAINZ_RELEASEGROUPID, &val);
			g_value_unset (&val);
#endif
		} else if (g_strcmp0 (source, "https://musicbrainz.org/doc/Artist") == 0) {
			g_value_init (&val, G_TYPE_STRING);
			g_value_set_string (&val, identifier);
			writeback_gstreamer_set (element, GST_TAG_MUSICBRAINZ_ARTISTID, &val);
			g_value_unset (&val);
		}
	}
}

static gboolean
writeback_gstreamer_write_file_metadata (TrackerWritebackFile  *writeback,
                                         GFile                 *file,
                                         TrackerResource       *resource,
                                         GCancellable          *cancellable,
                                         GError               **error)
{
	gboolean ret = FALSE;
	TagElements *element = (TagElements *) g_malloc (sizeof (TagElements));
	GList *l, *properties;

	element->tags = NULL;
	element->taggers = g_hash_table_new (g_str_hash, g_str_equal);

	if (gst_element_factory_find ("giostreamsink") == NULL) {
		g_warning ("giostreamsink not found, can't tag anything");
		g_hash_table_unref (element->taggers);
		g_free (element);
		return ret;
	} else {
		if (gst_element_factory_find ("vorbistag") &&
		    gst_element_factory_find ("vorbisparse") &&
		    gst_element_factory_find ("oggmux")) {
			g_debug ("ogg vorbis tagging available");
			g_hash_table_insert (element->taggers, "audio/x-vorbis", (gpointer) vorbis_tagger);
		}

		if (gst_element_factory_find ("flactag")) {
			g_debug ("flac tagging available");
			g_hash_table_insert (element->taggers, "audio/x-flac", flac_tagger);
		}

		if (gst_element_factory_find ("id3v2mux") ||
		    gst_element_factory_find ("id3mux")) {
			g_debug ("id3 tagging available");
			g_hash_table_insert (element->taggers, "audio/mpeg", mp3_tagger);
		}

		if (gst_element_factory_find ("mp4mux")) {
			g_debug ("mp4 tagging available");
			g_hash_table_insert (element->taggers, "audio/mp4", mp4_tagger);
			g_hash_table_insert (element->taggers, "audio/x-ac3", mp4_tagger);
		}
	}

	properties = tracker_resource_get_properties (resource);

	for (l = properties; l; l = l->next) {
		const gchar *prop = l->data;
		GValue val = G_VALUE_INIT;

		if (g_strcmp0 (prop, "nie:title") == 0) {
			const gchar *title;

			title = tracker_resource_get_first_string (resource, prop);
			g_value_init (&val, G_TYPE_STRING);
			g_value_set_string (&val, title);
			writeback_gstreamer_set (element, GST_TAG_TITLE, &val);
			g_value_unset (&val);
		}

		if (g_strcmp0 (prop, "nmm:performer") == 0) {
			TrackerResource *performer;
			const gchar *name = NULL;
			const gchar *mb_tags[] = {
				"https://musicbrainz.org/doc/Artist",
				NULL,
			};

			performer = tracker_resource_get_first_relation (resource, prop);

			if (performer) {
				name = tracker_resource_get_first_string (performer,
				                                          "nmm:artistName");

				handle_musicbrainz_tags (performer,
							 "tracker:hasExternalReference",
							 element, mb_tags);
			}

			if (name) {
				g_value_init (&val, G_TYPE_STRING);
				g_value_set_string (&val, name);
				writeback_gstreamer_set (element, GST_TAG_ARTIST, &val);
				g_value_unset (&val);
			}
		}

		if (g_strcmp0 (prop, "nmm:musicAlbum") == 0) {
			TrackerResource *album, *artist = NULL;
			const gchar *album_name = NULL, *album_artist = NULL;

			album = tracker_resource_get_first_relation (resource, prop);

			if (album) {
				album_name = tracker_resource_get_first_string (album, "nie:title");
				artist = tracker_resource_get_first_relation (album, "nmm:albumArtist");
			}

			if (artist) {
				album_artist = tracker_resource_get_first_string (artist, "nmm:artistName");
			}

			if (album_name) {
				g_value_init (&val, G_TYPE_STRING);
				g_value_set_string (&val, album_name);
				writeback_gstreamer_set (element, GST_TAG_ALBUM, &val);
				g_value_unset(&val);
			}

			if (album_artist) {
				g_value_init (&val, G_TYPE_STRING);
				g_value_set_string (&val, album_artist);
				writeback_gstreamer_set (element, GST_TAG_ALBUM_ARTIST, &val);
				g_value_unset(&val);
			}
		}

		if (g_strcmp0 (prop, "nie:comment") == 0) {
			const gchar *comment;

			comment = tracker_resource_get_first_string (resource, prop);
			g_value_init (&val, G_TYPE_STRING);
			g_value_set_string (&val, comment);
			writeback_gstreamer_set (element, GST_TAG_COMMENT, &val);
			g_value_unset (&val);
		}

		if (g_strcmp0 (prop, "nmm:genre") == 0) {
			const gchar *genre;

			genre = tracker_resource_get_first_string (resource, prop);
			g_value_init (&val, G_TYPE_STRING);
			g_value_set_string (&val, genre);
			writeback_gstreamer_set (element, GST_TAG_GENRE, &val);
			g_value_unset (&val);
		}

		if (g_strcmp0 (prop, "nmm:trackNumber") == 0) {
			gint number;

			number = tracker_resource_get_first_int (resource, prop);
			g_value_init (&val, G_TYPE_INT);
			g_value_set_int (&val, number);
			writeback_gstreamer_set (element, GST_TAG_TRACK_NUMBER, &val);
		}

		if (g_strcmp0 (prop, "nmm:artwork") == 0) {
			TrackerResource *image, *file = NULL;
			const gchar *artwork_url = NULL;

			image = tracker_resource_get_first_relation (resource, prop);

			if (image)
				file = tracker_resource_get_first_relation (image, "nie:isStoredAs");

			if (file)
				artwork_url = tracker_resource_get_first_string (image, "nie:url");

			if (artwork_url) {
				g_value_init (&val, G_TYPE_STRING);
				g_value_set_string (&val, artwork_url);
				writeback_gstreamer_set (element, GST_TAG_IMAGE, &val);
				g_value_unset (&val);
			}
		}

		if (g_strcmp0 (prop, "nie:contentCreated") == 0) {
			const gchar *created;

			created = tracker_resource_get_first_string (resource, prop);
			g_value_init (&val, G_TYPE_STRING);
			g_value_set_string (&val, created);
			writeback_gstreamer_set (element, GST_TAG_DATE_TIME, &val);
			g_value_unset (&val);
		}

		if (g_strcmp0 (prop, "nmm:internationalStandardRecordingCode") == 0) {
			const gchar *isrc;

			isrc = tracker_resource_get_first_string (resource, prop);
			g_value_init (&val, G_TYPE_STRING);
			g_value_set_string (&val, isrc);
			writeback_gstreamer_set (element, GST_TAG_ISRC, &val);
			g_value_unset (&val);
		}

		if (g_strcmp0 (prop, "nmm:lyrics") == 0) {
			const gchar *lyrics;

			lyrics = tracker_resource_get_first_string (resource, prop);
			g_value_init (&val, G_TYPE_STRING);
			g_value_set_string (&val, lyrics);
			writeback_gstreamer_set (element, GST_TAG_LYRICS, &val);
		}

		if (g_strcmp0 (prop, "nmm:composer") == 0) {
			TrackerResource *composer;
			const gchar *name = NULL;

			composer = tracker_resource_get_first_relation (resource, prop);

			if (composer) {
				name = tracker_resource_get_first_string (composer,
				                                          "nmm:artistName");
			}

			if (name) {
				g_value_init (&val, G_TYPE_STRING);
				g_value_set_string (&val, name);
				writeback_gstreamer_set (element, GST_TAG_COMPOSER, &val);
				g_value_unset (&val);
			}
		}

		if (g_strcmp0 (prop, "nmm:musicAlbumDisc") == 0) {
			TrackerResource *disc;
			gint number;

			disc = tracker_resource_get_first_relation (resource, prop);

			if (disc) {
				const gchar *mb_tags[] = {
					"https://musicbrainz.org/doc/Release",
					"https://musicbrainz.org/doc/Release_Group",
					NULL,
				};

				number = tracker_resource_get_first_int (disc,
				                                         "nmm:setNumber");
				g_value_init (&val, G_TYPE_INT);
				g_value_set_int (&val, number);
				writeback_gstreamer_set (element, GST_TAG_ALBUM_VOLUME_NUMBER, &val);
				g_value_unset (&val);

				handle_musicbrainz_tags (disc,
							 "tracker:hasExternalReference",
							 element, mb_tags);
			}
		}

		if (g_strcmp0 (prop, "nco:publisher") == 0) {
			TrackerResource *publisher;
			const gchar *name = NULL;

			publisher = tracker_resource_get_first_relation (resource, prop);

			if (publisher) {
				name = tracker_resource_get_first_string (publisher,
				                                          "nco:fullname");
			}

			if (name) {
				g_value_init (&val, G_TYPE_STRING);
				g_value_set_string (&val, name);
				writeback_gstreamer_set (element, GST_TAG_PUBLISHER, &val);
				g_value_unset (&val);
			}
		}

		if (g_strcmp0 (prop, "nie:description") == 0) {
			const gchar *description;

			description = tracker_resource_get_first_string (resource, prop);
			g_value_init (&val, G_TYPE_STRING);
			g_value_set_string (&val, description);
			writeback_gstreamer_set (element, GST_TAG_DESCRIPTION, &val);
			g_value_unset (&val);
		}

		if (g_strcmp0 (prop, "nie:keyword") == 0) {
			GList *keywords, *k;
			GString *keyword_str = NULL;

			keywords = tracker_resource_get_values (resource, prop);

			for (k = keywords; k; k = k->next) {
				GValue *value;

				value = k->data;

				if (G_VALUE_HOLDS_STRING (value)) {
					const gchar *str = g_value_get_string (value);

					if (!keywords)
						keyword_str = g_string_new (str);
					else
						g_string_append_printf (keyword_str, ",%s", str);
				}
			}

			if (keyword_str->len > 0) {
				g_value_init (&val, G_TYPE_STRING);
				g_value_set_string (&val, keyword_str->str);
				writeback_gstreamer_set (element, GST_TAG_KEYWORDS, &val);
				g_value_unset (&val);
			}

			g_string_free (keyword_str, TRUE);
			g_list_free (keywords);
		}

		if (g_strcmp0 (prop, "tracker:hasExternalReference") == 0) {
			const gchar *mb_tags[] = {
				"https://musicbrainz.org/doc/Recording",
				"https://musicbrainz.org/doc/Track",
				NULL,
			};

			handle_musicbrainz_tags (resource, prop, element, mb_tags);
		}

		if (g_strcmp0 (prop, "nfo:hasHash") == 0) {
			TrackerResource *hash;
			const gchar *value = NULL, *algorithm;

			hash = tracker_resource_get_first_relation (resource, prop);

			if (hash) {
				algorithm = tracker_resource_get_first_string (hash,
									       "nfo:hashAlgorithm");
				value = tracker_resource_get_first_string (hash,
									   "nfo:hashValue");
			}

#ifdef GST_TAG_ACOUSTID_FINGERPRINT
			if (value && algorithm && g_strcmp0 (algorithm, "chromaprint") == 0) {
				g_value_init (&val, G_TYPE_STRING);
				g_value_set_string (&val, value);
				writeback_gstreamer_set (element, GST_TAG_ACOUSTID_FINGERPRINT, &val);
				g_value_unset (&val);
			}
#endif
		}
	}

	writeback_gstreamer_save (element, file, error);

	if (*error != NULL) {
		g_warning ("Error (%s) occured while attempting to write tags", (*error)->message);
	} else {
		ret = TRUE;
	}

	if (element->tags != NULL)
		gst_tag_list_unref (element->tags);
	if (element->taggers != NULL)
		g_hash_table_unref (element->taggers);
	g_list_free (properties);
	g_free (element);

	return ret;
}

TrackerWriteback *
writeback_module_create (GTypeModule *module)
{
	tracker_writeback_gstreamer_register_type (module);
	return g_object_new (TRACKER_TYPE_WRITEBACK_GSTREAMER, NULL);
}

const gchar *
const *writeback_module_get_rdf_types (void)
{
	static const gchar *rdftypes[] = {
		TRACKER_PREFIX_NFO "Audio",
		NULL
	};

	return rdftypes;
}
