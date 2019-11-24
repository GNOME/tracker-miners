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
static gboolean            writeback_gstreamer_update_file_metadata (TrackerWritebackFile    *writeback_file,
                                                                     GFile                   *file,
                                                                     GPtrArray               *values,
                                                                     TrackerSparqlConnection *connection,
                                                                     GCancellable            *cancellable,
                                                                     GError                 **error);
static const gchar* const *writeback_gstreamer_content_types        (TrackerWritebackFile    *writeback_file);
static gchar              *writeback_gstreamer_get_artist_name      (TrackerSparqlConnection *connection,
                                                                     const gchar             *urn);
static gchar              *writeback_gstreamer_get_album_name       (TrackerSparqlConnection *connection,
                                                                     const gchar             *urn);
static gchar              *writeback_gstreamer_get_album_artist     (TrackerSparqlConnection *connection,
                                                                     const gchar             *urn);
static gchar              *writeback_gstreamer_get_disc_number      (TrackerSparqlConnection *connection,
                                                                     const gchar             *urn);
static gchar              *writeback_gstreamer_get_publisher_name   (TrackerSparqlConnection *connection,
                                                                     const gchar             *urn);
static gchar              *writeback_gstreamer_get_artwork_url      (TrackerSparqlConnection *connection,
                                                                     const gchar             *urn);

G_DEFINE_DYNAMIC_TYPE (TrackerWritebackGstreamer, tracker_writeback_gstreamer, TRACKER_TYPE_WRITEBACK_FILE);

static void
tracker_writeback_gstreamer_class_init (TrackerWritebackGstreamerClass *klass)
{
	TrackerWritebackFileClass *writeback_file_class = TRACKER_WRITEBACK_FILE_CLASS (klass);

	gst_init (NULL, NULL);

	writeback_file_class->update_file_metadata = writeback_gstreamer_update_file_metadata;
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

static gboolean
writeback_gstreamer_update_file_metadata (TrackerWritebackFile    *writeback,
                                          GFile                   *file,
                                          GPtrArray               *values,
                                          TrackerSparqlConnection *connection,
                                          GCancellable            *cancellable,
                                          GError                 **error)
{
	guint n;
	gboolean ret = FALSE;
	TagElements *element = (TagElements *) g_malloc (sizeof (TagElements));

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

	for (n = 0; n < values->len; n++) {
		const GStrv row = g_ptr_array_index (values, n);
		GValue val = G_VALUE_INIT;

		if (g_strcmp0 (row[2], TRACKER_PREFIX_NIE "title") == 0) {
			g_value_init (&val, G_TYPE_STRING);
			g_value_set_string (&val, row[3]);
			writeback_gstreamer_set (element, GST_TAG_TITLE, &val);
		}

		if (g_strcmp0 (row[2], TRACKER_PREFIX_NMM "performer") == 0) {
			gchar *artist_name = writeback_gstreamer_get_artist_name (connection, row[3]);

			if (artist_name) {
				g_value_init (&val, G_TYPE_STRING);
				g_value_set_string (&val, artist_name);
				writeback_gstreamer_set (element, GST_TAG_ARTIST, &val);
				g_free (artist_name);
			}
		}

		if (g_strcmp0 (row[2], TRACKER_PREFIX_NMM "musicAlbum") == 0) {
			gchar *album_name = writeback_gstreamer_get_album_name (connection, row[3]);
			gchar *album_artist = writeback_gstreamer_get_album_artist (connection, row[3]);

			if (album_name) {
				g_value_init (&val, G_TYPE_STRING);
				g_value_set_string (&val, album_name);
				writeback_gstreamer_set (element, GST_TAG_ALBUM, &val);
				g_free (album_name);
			}
			g_value_unset(&val);
			if (album_artist) {
				g_value_init (&val, G_TYPE_STRING);
				g_value_set_string (&val, album_artist);
				writeback_gstreamer_set (element, GST_TAG_ALBUM_ARTIST, &val);
				g_free (album_artist);
			}
		}

		if (g_strcmp0 (row[2], TRACKER_PREFIX_NIE "comment") == 0) {
			g_value_init (&val, G_TYPE_STRING);
			g_value_set_string (&val, row[3]);
			writeback_gstreamer_set (element, GST_TAG_COMMENT, &val);
		}

		if (g_strcmp0 (row[2], TRACKER_PREFIX_NMM "genre") == 0) {
			g_value_init (&val, G_TYPE_STRING);
			g_value_set_string (&val, row[3]);
			writeback_gstreamer_set (element, GST_TAG_GENRE, &val);
		}

		if (g_strcmp0 (row[2], TRACKER_PREFIX_NMM "trackNumber") == 0) {
			g_value_init (&val, G_TYPE_INT);
			g_value_set_int (&val, atoi(row[3]));
			writeback_gstreamer_set (element, GST_TAG_TRACK_NUMBER, &val);
		}

		if (g_strcmp0 (row[2], TRACKER_PREFIX_NMM "artwork") == 0) {
			gchar *artwork_url = writeback_gstreamer_get_artwork_url(connection, row[3]);

			if (artwork_url) {
				g_value_init (&val, G_TYPE_STRING);
				g_value_set_string (&val, artwork_url);
				writeback_gstreamer_set (element, GST_TAG_IMAGE, &val);
				g_free (artwork_url);
			}
		}

		if (g_strcmp0 (row[2], TRACKER_PREFIX_NIE "contentCreated") == 0) {
			g_value_init (&val, G_TYPE_STRING);
			g_value_set_string (&val, row[3]);
			writeback_gstreamer_set (element, GST_TAG_DATE_TIME, &val);
		}

		if (g_strcmp0 (row[2], TRACKER_PREFIX_NMM "internationalStandardRecordingCode") == 0) {
			g_value_init (&val, G_TYPE_STRING);
			g_value_set_string (&val, row[3]);
			writeback_gstreamer_set (element, GST_TAG_ISRC, &val);
		}

		if (g_strcmp0 (row[2], TRACKER_PREFIX_NMM "lyrics") == 0) {
			g_value_init (&val, G_TYPE_STRING);
			g_value_set_string (&val, row[3]);
			writeback_gstreamer_set (element, GST_TAG_LYRICS, &val);
		}

		if (g_strcmp0 (row[2], TRACKER_PREFIX_NMM "composer") == 0) {
			gchar *composer_name = writeback_gstreamer_get_artist_name (connection, row[3]);

			if (composer_name) {
				g_value_init (&val, G_TYPE_STRING);
				g_value_set_string (&val, composer_name);
				writeback_gstreamer_set (element, GST_TAG_COMPOSER, &val);
				g_free (composer_name);
			}
		}

		if (g_strcmp0 (row[2], TRACKER_PREFIX_NMM "musicAlbumDisc") == 0) {
			gchar *disc_number = writeback_gstreamer_get_disc_number (connection, row[3]);

			if (disc_number) {
				g_value_init (&val, G_TYPE_INT);
				g_value_set_int (&val, atoi(disc_number));
				writeback_gstreamer_set (element, GST_TAG_ALBUM_VOLUME_NUMBER, &val);
			}
		}

		if (g_strcmp0 (row[2], TRACKER_PREFIX_NCO "publisher") == 0) {
			gchar *publisher = writeback_gstreamer_get_publisher_name (connection, row[3]);

			if (publisher) {
				g_value_init (&val, G_TYPE_STRING);
				g_value_set_string (&val, publisher);
				writeback_gstreamer_set (element, GST_TAG_PUBLISHER, &val);
			}
		}

		if (g_strcmp0 (row[2], TRACKER_PREFIX_NIE "description") == 0) {
			g_value_init (&val, G_TYPE_STRING);
			g_value_set_string (&val, row[3]);
			writeback_gstreamer_set (element, GST_TAG_DESCRIPTION, &val);
		}

		if (g_strcmp0 (row[2], TRACKER_PREFIX_NIE "keyword") == 0) {
			g_value_init (&val, G_TYPE_STRING);
			g_value_set_string (&val, row[3]);
			writeback_gstreamer_set (element, GST_TAG_KEYWORDS, &val);
		}

		g_value_unset (&val);
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
	g_free (element);

	return ret;
}

static gchar *
writeback_gstreamer_get_from_query (TrackerSparqlConnection *connection,
                                    const gchar             *urn,
                                    const gchar             *query,
                                    const gchar             *field)
{
	TrackerSparqlCursor *cursor;
	GError *error = NULL;
	gchar *value = NULL;

	cursor = tracker_sparql_connection_query (connection, query, NULL, &error);

	if (error || !cursor || !tracker_sparql_cursor_next (cursor, NULL, NULL)) {
		g_warning ("Couldn't find %s for entity with urn '%s', %s",
		           field,
		           urn,
		           error ? error->message : "no such value was found");

		if (error) {
			g_error_free (error);
		}
	} else {
		value = g_strdup (tracker_sparql_cursor_get_string (cursor, 0, NULL));
	}

	g_clear_object (&cursor);

	return value;
}

static gchar *
writeback_gstreamer_get_artist_name (TrackerSparqlConnection *connection,
                                     const gchar             *urn)
{
	gchar *val, *query;

	query = g_strdup_printf ("SELECT ?artistName WHERE {<%s> nmm:artistName ?artistName}", urn);
	val = writeback_gstreamer_get_from_query (connection, urn, query, "artist name");
	g_free (query);

	return val;
}

static gchar *
writeback_gstreamer_get_album_name (TrackerSparqlConnection *connection,
                                    const gchar             *urn)
{
	gchar *val, *query;

	query = g_strdup_printf ("SELECT ?albumName WHERE {<%s> dc:title ?albumName}", urn);
	val = writeback_gstreamer_get_from_query (connection, urn, query, "album name");
	g_free (query);

	return val;
}

static gchar *
writeback_gstreamer_get_album_artist (TrackerSparqlConnection *connection,
                                      const gchar             *urn)
{
	gchar *artist_urn, *val, *query;

	query = g_strdup_printf ("SELECT ?albumArtist WHERE {<%s> nmm:albumArtist ?albumArtist}", urn);
	artist_urn = writeback_gstreamer_get_from_query (connection, urn, query, "album artist");
	val = writeback_gstreamer_get_artist_name (connection, artist_urn);
	g_free(query);
	g_free(artist_urn);

	return val;
}

static gchar *
writeback_gstreamer_get_disc_number (TrackerSparqlConnection *connection,
                                     const gchar             *urn)
{
	gchar *val, *query;

	query = g_strdup_printf ("SELECT ?setNumber WHERE {<%s> nmm:setNumber ?setNumber}", urn);
	val = writeback_gstreamer_get_from_query (connection, urn, query, "set number");
	g_free (query);

	return val;
}

static gchar *
writeback_gstreamer_get_publisher_name (TrackerSparqlConnection *connection,
                                        const gchar             *urn)
{
	gchar *val, *query;

	query = g_strdup_printf ("SELECT ?name WHERE {<%s> nco:fullname ?name}", urn);
	val = writeback_gstreamer_get_from_query (connection, urn, query, "fullname");
	g_free (query);

	return val;
}

static gchar *
writeback_gstreamer_get_artwork_url (TrackerSparqlConnection *connection,
                                     const gchar             *urn)
{
	gchar *val, *query;

	query = g_strdup_printf ("SELECT ?url WHERE {<%s> nie:url ?url}", urn);
	val = writeback_gstreamer_get_from_query (connection, urn, query, "image URL");
	g_free (query);

	return val;
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
