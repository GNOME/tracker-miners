/*
 * Copyright (C) 2007, Jamie McCracken <jamiemcc@gnome.org>
 * Copyright (C) 2008-2009, Nokia <ivan.frade@nokia.com>
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

#include <glib.h>
#include <glib/gstdio.h>

#include <libtracker-miners-common/tracker-common.h>
#include <libtracker-extract/tracker-extract.h>

static gchar *
hour_day_str_day (const gchar *date)
{
	/* From: ex. date: "(18:07 Tuesday 22 May 2007)"
	 * To  : ex. ISO8601 date: "2007-05-22T18:07:10-0600"
	 */
	return tracker_date_format_to_iso8601 (date, "(%H:%M %A %d %B %Y)");
}

static gchar *
day_str_month_day (const gchar *date)
{
	/* From: ex. date: "Tue May 22 18:07:10 2007"
	 * To  : ex. ISO8601 date: "2007-05-22T18:07:10-0600"
	 */
	return tracker_date_format_to_iso8601 (date, "%A %B %d %H:%M:%S %Y");
}

static gchar *
day_month_year_date (const gchar *date)
{
	/* From: ex. date: "22 May 1997 18:07:10 -0600"
	 * To  : ex. ISO8601 date: "2007-05-22T18:07:10-0600"
	 */
	return tracker_date_format_to_iso8601 (date, "%d %B %Y %H:%M:%S %z");
}

static gchar *
hour_month_day_date (const gchar *date)
{
	/* From: ex. date: "6:07 PM May 22, 2007"
	 * To  : ex. ISO8601 date: "2007-05-22T18:07:10-0600"
	 */
	return tracker_date_format_to_iso8601 (date, "%I:%M %p %B %d, %Y");
}

static gchar *
date_to_iso8601 (const gchar *date)
{
	if (date && date[1] && date[2]) {
		if (date[0] == '(') {
			/* we have probably a date like
			 * "(18:07 Tuesday 22 May 2007)"
			 */
			return hour_day_str_day (date);
		} else if (g_ascii_isalpha (date[0])) {
			/* we have probably a date like
			 * "Tue May 22 18:07:10 2007"
			 */
			return day_str_month_day (date);

		} else if (date[1] == ' ' || date[2] == ' ') {
			/* we have probably a date like
			 * "22 May 1997 18:07:10 -0600"
			 */
			return day_month_year_date (date);

		} else if (date[1] == ':' || date[2] == ':') {
			/* we have probably a date like
			 * "6:07 PM May 22, 2007"
			 */
			return hour_month_day_date (date);
		}
	}

	return NULL;
}

static TrackerResource *
extract_ps_from_inputstream (GInputStream *stream)
{
	TrackerResource *metadata;
	g_autoptr(GDataInputStream) data_stream = NULL;
	gchar *line;
	gsize length, accum, max_bytes;
	gboolean pageno_atend = FALSE;
	gboolean header_finished = FALSE;
	g_autoptr(GError) error = NULL;

	metadata = tracker_resource_new (NULL);
	tracker_resource_add_uri (metadata, "rdf:type", "nfo:PaginatedTextDocument");

	data_stream = g_data_input_stream_new (stream);

	/* 20 MiB should be enough! (original safe limit) */
	accum = 0;
	max_bytes = 20u << 20;

	while ((accum < max_bytes) &&
	       (line = g_data_input_stream_read_line (data_stream, &length, NULL, &error)) != NULL) {
		/* Update accumulated bytes read */
		accum += length;

		if (!header_finished && strncmp (line, "%%Copyright:", 12) == 0) {
			tracker_resource_set_string (metadata, "nie:copyright", line + 13);
		} else if (!header_finished && strncmp (line, "%%Title:", 8) == 0) {
			tracker_resource_set_string (metadata, "nie:title", line + 9);
		} else if (!header_finished && strncmp (line, "%%Creator:", 10) == 0) {
			TrackerResource *creator = tracker_extract_new_contact (line + 11);
			tracker_resource_set_relation (metadata, "nco:creator", creator);
			g_object_unref (creator);
		} else if (!header_finished && strncmp (line, "%%CreationDate:", 15) == 0) {
			g_autofree gchar *date = NULL;

			date = date_to_iso8601 (line + 16);
			if (date)
				tracker_resource_set_string (metadata, "nie:contentCreated", date);
		} else if (strncmp (line, "%%Pages:", 8) == 0) {
			if (strcmp (line + 9, "(atend)") == 0) {
				pageno_atend = TRUE;
			} else {
				gint64 page_count;

				page_count = g_ascii_strtoll (line + 9, NULL, 10);
				tracker_resource_set_int (metadata, "nfo:pageCount", page_count);
			}
		} else if (strncmp (line, "%%EndComments", 14) == 0) {
			header_finished = TRUE;

			if (!pageno_atend) {
				g_free (line);
				break;
			}
		}

		g_free (line);
	}

	if (error != NULL)
		g_warning ("Unexpected lack of content trying to read a line: %s", error->message);

	return metadata;
}

static TrackerResource *
extract_ps (const gchar *uri)
{
	g_autoptr(GFile) file = NULL;
	g_autoptr(GInputStream) stream = NULL;
	g_autoptr(GError) error = NULL;

	g_debug ("Extracting PS '%s'...", uri);

	file = g_file_new_for_uri (uri);

	stream = G_INPUT_STREAM (g_file_read (file, NULL, &error));
	if (stream == NULL) {
		g_warning ("Could't not read file %s: %s", uri, error->message);
		return NULL;
	}

	return extract_ps_from_inputstream (stream);
}

#ifdef USING_UNZIPPSFILES

static TrackerResource *
extract_ps_gz (const gchar *uri)
{
	g_autoptr(GFile) file = NULL;
	g_autoptr(GInputStream) stream, cstream = NULL;
	g_autoptr(GConverter) converter = NULL;
	g_autoptr(GError) error = NULL;

	g_debug ("Extracting PS '%s'...", uri);

	file = g_file_new_for_uri (uri);

	stream = G_INPUT_STREAM (g_file_read (file, NULL, &error));
	if (stream == NULL) {
		g_warning ("Could't not read file %s: %s", uri, error->message);
		return NULL;
	}

	converter = G_CONVERTER (g_zlib_decompressor_new (G_ZLIB_COMPRESSOR_FORMAT_GZIP));
	cstream = g_converter_input_stream_new (stream, converter);

	return extract_ps_from_inputstream (cstream);
}

#endif /* USING_UNZIPPSFILES */

G_MODULE_EXPORT gboolean
tracker_extract_get_metadata (TrackerExtractInfo *info)
{
	TrackerResource *metadata;
	GFile *file;
	g_autofree gchar *uri = NULL;
	const char *mimetype;

	file = tracker_extract_info_get_file (info);
	uri = g_file_get_uri (file);
	mimetype = tracker_extract_info_get_mimetype (info);

	if (strcmp (mimetype, "application/x-gzpostscript") == 0) {
#ifdef USING_UNZIPPSFILES
		metadata = extract_ps_gz (uri);
#else
		metadata = NULL;
#endif /* USING_UNZIPPSFILES */
	} else {
		metadata = extract_ps (uri);
	}

	if (metadata) {
		tracker_extract_info_set_resource (info, metadata);
		g_object_unref (metadata);
	}

	return TRUE;
}
