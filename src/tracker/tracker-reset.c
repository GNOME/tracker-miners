/*
 * Copyright (C) 2014, Lanedo <martyn@lanedo.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include "config-miners.h"

#include <stdlib.h>
#include <stdio.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <gio/gio.h>
#include <locale.h>

#include <libtracker-miners-common/tracker-common.h>
#include <libtracker-sparql/tracker-sparql.h>

#include "tracker-process.h"
#include "tracker-color.h"
#include "tracker-miner-manager.h"

static gboolean files = FALSE;
static gboolean rss = FALSE;
static gchar *filename = NULL;

#define RESET_OPTIONS_ENABLED() \
	(files || \
	 rss || \
	 filename)

static GOptionEntry entries[] = {
	{ "filesystem", 's', 0, G_OPTION_ARG_NONE, &files,
	  N_("Remove filesystem indexer database"),
	  NULL },
	{ "rss", 'r', 0, G_OPTION_ARG_NONE, &rss,
	  N_("Remove RSS indexer database"),
	  NULL },
	{ "file", 'f', 0, G_OPTION_ARG_FILENAME, &filename,
	  N_("Erase indexed information about a file, works recursively for directories"),
	  N_("FILE") },
	{ NULL }
};

static int
delete_info_recursively (GFile *file)
{
	TrackerSparqlConnection *connection;
	TrackerMinerManager *miner_manager;
	TrackerSparqlCursor *cursor;
	gchar *query, *uri;
	GError *error = NULL;

	connection = tracker_sparql_connection_bus_new ("org.freedesktop.Tracker3.Miner.Files",
	                                                NULL, NULL, &error);

	if (error)
		goto error;

	uri = g_file_get_uri (file);

	/* First, query whether the item exists */
	query = g_strdup_printf ("SELECT ?u { ?u nie:url '%s' }", uri);
	cursor = tracker_sparql_connection_query (connection, query,
	                                          NULL, &error);

	/* If the item doesn't exist, bail out. */
	if (!cursor || !tracker_sparql_cursor_next (cursor, NULL, &error)) {
		g_clear_object (&cursor);

		if (error)
			goto error;

		return EXIT_SUCCESS;
	}

	g_object_unref (cursor);

	/* Now, delete the element recursively */
	g_print ("%s\n", _("Deleting…"));
	query = g_strdup_printf ("DELETE { "
	                         "  ?f a rdfs:Resource . "
	                         "  ?ie a rdfs:Resource "
	                         "} WHERE {"
	                         "  ?f nie:url ?url . "
	                         "  ?ie nie:isStoredAs ?f . "
	                         "  FILTER (?url = '%s' ||"
	                         "          STRSTARTS (?url, '%s/'))"
	                         "}", uri, uri);
	g_free (uri);

	tracker_sparql_connection_update (connection, query, NULL, &error);
	g_free (query);

	if (error)
		goto error;

	g_object_unref (connection);

	g_print ("%s\n", _("The indexed data for this file has been deleted "
	                   "and will be reindexed again."));

	/* Request reindexing of this data, it was previously in the store. */
	miner_manager = tracker_miner_manager_new_full (FALSE, NULL);
	tracker_miner_manager_index_location (miner_manager, file, TRACKER_INDEX_LOCATION_FLAGS_NONE, NULL, &error);
	g_object_unref (miner_manager);

	if (error)
		goto error;

	return EXIT_SUCCESS;

error:
	g_warning ("%s", error->message);
	g_error_free (error);
	return EXIT_FAILURE;
}

static void
delete_databases (GFile *dir)
{
	GFileEnumerator *enumerator;
	GError *error = NULL;
	GFileInfo *info;

	enumerator = g_file_enumerate_children (dir,
	                                        G_FILE_ATTRIBUTE_STANDARD_NAME,
	                                        G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
	                                        NULL, &error);
	if (error) {
		g_critical ("Location does not have a Tracker DB: %s",
		            error->message);
		g_error_free (error);
		return;
	}

	while ((info = g_file_enumerator_next_file (enumerator, NULL, NULL)) != NULL) {
		const gchar *name;
		GFile *child;

		name = g_file_info_get_name (info);

		/* Delete sqlite files in the location */
		if (g_str_has_suffix (name, ".db") ||
		    g_str_has_suffix (name, ".db-wal") ||
		    g_str_has_suffix (name, ".db-shm")) {
			child = g_file_enumerator_get_child (enumerator, info);

			if (!g_file_delete (child, NULL, &error)) {
				g_critical ("Failed to delete '%s': %s",
				            g_file_info_get_name (info),
				            error->message);
				g_error_free (error);
			}

			g_object_unref (child);
		}
	}

	g_object_unref (enumerator);
}

static gint
reset_run (void)
{
	if (filename) {
		GFile *file;
		gint retval;

		file = g_file_new_for_commandline_arg (filename);
		retval = delete_info_recursively (file);
		g_object_unref (file);
		return retval;
	}

	/* KILL processes first... */
	if (files || rss) {
		/* FIXME: we might selectively kill affected miners */
		tracker_process_stop (TRACKER_PROCESS_TYPE_NONE, TRACKER_PROCESS_TYPE_MINERS);
	}

	if (files) {
		GFile *cache_location;
		gchar *dir;

		dir = g_build_filename (g_get_user_cache_dir (), "tracker3", "files", NULL);
		cache_location = g_file_new_for_path (dir);
		delete_databases (cache_location);
		g_object_unref (cache_location);
		g_free (dir);
	}

	if (rss) {
		GFile *cache_location;
		gchar *dir;

		dir = g_build_filename (g_get_user_cache_dir (), "tracker3", "rss", NULL);
		cache_location = g_file_new_for_path (dir);
		delete_databases (cache_location);
		g_object_unref (cache_location);
		g_free (dir);
	}

	return EXIT_SUCCESS;
}

static int
reset_run_default (void)
{
	GOptionContext *context;
	gchar *help;

	context = g_option_context_new (NULL);
	g_option_context_add_main_entries (context, entries, NULL);
	help = g_option_context_get_help (context, FALSE, NULL);
	g_option_context_free (context);
	g_printerr ("%s\n", help);
	g_free (help);

	return EXIT_FAILURE;
}

static gboolean
reset_options_enabled (void)
{
	return RESET_OPTIONS_ENABLED ();
}

int
main (int argc, const char **argv)
{
	GOptionContext *context;
	GError *error = NULL;

	setlocale (LC_ALL, "");

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	context = g_option_context_new (NULL);
	g_option_context_add_main_entries (context, entries, NULL);

	argv[0] = "tracker reset";

	if (!g_option_context_parse (context, &argc, (char***) &argv, &error)) {
		g_printerr ("%s, %s\n", _("Unrecognized options"), error->message);
		g_error_free (error);
		g_option_context_free (context);
		return EXIT_FAILURE;
	}

	g_option_context_free (context);

	if (reset_options_enabled ()) {
		return reset_run ();
	}

	return reset_run_default ();
}
