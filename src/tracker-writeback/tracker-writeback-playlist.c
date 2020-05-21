/*
 * Copyright (C) 2009, Nokia <ivan.frade@nokia.com>
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
 *
 * Authors: Philip Van Hoof <philip@codeminded.be>
 */

#include "config-miners.h"

#include <string.h>

#include <totem-pl-parser.h>

#include <libtracker-sparql/tracker-ontologies.h>

#include "tracker-writeback-file.h"

#define TRACKER_TYPE_WRITEBACK_PLAYLIST (tracker_writeback_playlist_get_type ())

typedef struct TrackerWritebackPlaylist TrackerWritebackPlaylist;
typedef struct TrackerWritebackPlaylistClass TrackerWritebackPlaylistClass;
typedef struct PlaylistMap PlaylistMap;

struct TrackerWritebackPlaylist {
	TrackerWritebackFile parent_instance;
};

struct TrackerWritebackPlaylistClass {
	TrackerWritebackFileClass parent_class;
};

struct PlaylistMap {
	const gchar *mime_type;
	TotemPlParserType playlist_type;
};

static GType                tracker_writeback_playlist_get_type     (void) G_GNUC_CONST;
static gboolean             writeback_playlist_write_file_metadata  (TrackerWritebackFile  *wbf,
                                                                     GFile                 *file,
                                                                     TrackerResource       *resource,
                                                                     GCancellable          *cancellable,
                                                                     GError               **error);
static const gchar * const *writeback_playlist_content_types        (TrackerWritebackFile  *wbf);

G_DEFINE_DYNAMIC_TYPE (TrackerWritebackPlaylist, tracker_writeback_playlist, TRACKER_TYPE_WRITEBACK_FILE);

static void
tracker_writeback_playlist_class_init (TrackerWritebackPlaylistClass *klass)
{
	TrackerWritebackFileClass *writeback_file_class = TRACKER_WRITEBACK_FILE_CLASS (klass);

	writeback_file_class->write_file_metadata = writeback_playlist_write_file_metadata;
	writeback_file_class->content_types = writeback_playlist_content_types;
}

static void
tracker_writeback_playlist_class_finalize (TrackerWritebackPlaylistClass *klass)
{
}

static void
tracker_writeback_playlist_init (TrackerWritebackPlaylist *wbm)
{
}

static const gchar * const *
writeback_playlist_content_types (TrackerWritebackFile *wbf)
{
	static const gchar *content_types[] = {
		"audio/x-mpegurl",
		"audio/mpegurl",
		"audio/x-scpls",
		"application/xspf+xml",
		"audio/x-iriver-pla",
#if 0
		"audio/x-pn-realaudio",
		"application/ram",
		"application/vnd.ms-wpl",
		"application/smil",
		"audio/x-ms-asx",
#endif
		NULL
	};

	return content_types;
}

static gboolean
get_playlist_type (GFile             *file,
                   TotemPlParserType *type)
{
	GFileInfo *file_info;
	const gchar *mime_type;
	gint i;
	PlaylistMap playlist_map[] = {
		{ "audio/x-mpegurl", TOTEM_PL_PARSER_M3U },
		{ "audio/mpegurl", TOTEM_PL_PARSER_M3U },
		{ "audio/x-scpls", TOTEM_PL_PARSER_PLS },
		{ "application/xspf+xml", TOTEM_PL_PARSER_XSPF },
		{ "audio/x-iriver-pla", TOTEM_PL_PARSER_IRIVER_PLA }
	};

	file_info = g_file_query_info (file,
	                               G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
	                               G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
	                               NULL, NULL);

	if (!file_info) {
		return FALSE;
	}

	mime_type = g_file_info_get_content_type (file_info);
	g_object_unref (file_info);

	if (!mime_type) {
		return FALSE;
	}

	for (i = 0; i < G_N_ELEMENTS (playlist_map); i++) {
		if (strcmp (mime_type, playlist_map[i].mime_type) == 0) {
			*type = playlist_map[i].playlist_type;
			return TRUE;
		}
	}

	return FALSE;
}

static gboolean
writeback_playlist_write_file_metadata (TrackerWritebackFile  *writeback_file,
                                        GFile                 *file,
                                        TrackerResource       *resource,
                                        GCancellable          *cancellable,
                                        GError               **error)
{
	GList *properties, *l;
	TotemPlParser *parser;
	TotemPlPlaylist *playlist;
	TotemPlPlaylistIter iter;
	TotemPlParserType type;
	GPtrArray *entries;
	guint amount = 0;
	gchar *path;

	if (!get_playlist_type (file, &type)) {
		g_set_error (G_IO_ERROR,
		             G_IO_ERROR_FAILED,
		             "Unhandled playlist type");
		return FALSE;
	}

	entries = g_ptr_array_new ();

	for (l = properties; l; l = l->next) {
		const gchar *prop = l->data;

		if (g_strcmp0 (prop, "nfo:hasMediaFileListEntry") == 0) {
			GList *resources;

			resources = tracker_resource_get_values (resource, prop);

			for (l = resources; l; l = l->next) {
				TrackerResource *entry;
				const gchar *url;
				GValue *value;
				gpointer ptr;
				gint pos;

				value = l->data;
				entry = g_value_get_object (value);
				pos = tracker_resource_get_first_integer (entry,
				                                          "nfo:listPosition");
				url = tracker_resource_get_first_string (entry,
				                                         "nfo:entryUrl");

				if (entries->len < pos)
					g_array_set_size (entries, pos);

				ptr = &g_ptr_array_index (entries, pos) = url;
				*ptr = url;
			}
		}
	}

	parser = totem_pl_parser_new ();
	playlist = totem_pl_playlist_new ();

	for (i = 0; entries->len; i++) {
		const gchar *uri;

		uri = g_ptr_array_index (entries, i);

		if (uri) {
			totem_pl_playlist_append  (playlist, &iter);
			totem_pl_playlist_set (playlist, &iter,
			                       TOTEM_PL_PARSER_FIELD_URI,
			                       uri, NULL);
		}
	}

	if (entries->len > 0) {
		totem_pl_parser_save (parser, playlist, file, NULL, type, &error);
	} else {
		/* TODO: Empty the file in @path */
	}

	if (error) {
		g_critical ("Could not save playlist: %s\n", error->message);
		g_error_free (error);
	}

	g_ptr_array_unref (entries);
	g_list_free (properties);
	g_object_unref (playlist);
	g_object_unref (parser);

	return TRUE;
}

TrackerWriteback *
writeback_module_create (GTypeModule *module)
{
	tracker_writeback_playlist_register_type (module);

	return g_object_new (TRACKER_TYPE_WRITEBACK_PLAYLIST, NULL);
}

const gchar * const *
writeback_module_get_rdf_types (void)
{
	static const gchar *rdftypes[] = {
		TRACKER_PREFIX_NFO "MediaList",
		TRACKER_PREFIX_NFO "MediaFileListEntry",
		NULL
	};

	return rdftypes;
}
