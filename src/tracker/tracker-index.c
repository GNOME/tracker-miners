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
#include <errno.h>

#ifdef __sun
#include <procfs.h>
#endif

#include <glib.h>
#include <glib-unix.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <locale.h>

#include <libtracker-sparql/tracker-sparql.h>

#include "tracker-dbus.h"
#include "tracker-miner-manager.h"

static gboolean file_arg;
static gboolean monitor_mode;
static gchar **filenames;

static GOptionEntry entries[] = {
	{ "file", 'f', 0, G_OPTION_ARG_NONE, &file_arg,
	 N_("Does nothing, provided for compatibility with Tracker 2.0"),
	 NULL },
	{ "monitor", 'm', 0, G_OPTION_ARG_NONE, &monitor_mode,
	  N_("Trigger indexing and wait for CTRL+C, removing the data again on exit."),
	  NULL },
	{ G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &filenames,
	  N_("FILE"),
	  N_("FILE") },
	{ NULL }
};

static gint
index_or_reindex_file (void)
{
	TrackerMinerManager *manager;
	GError *error = NULL;
	gchar **p;

	/* Auto-start the miners here if we need to */
	manager = tracker_miner_manager_new_full (TRUE, &error);
	if (!manager) {
		g_printerr (_("Could not (re)index file, manager could not be created, %s"),
		            error ? error->message : _("No error given"));
		g_printerr ("\n");
		g_clear_error (&error);
		return EXIT_FAILURE;
	}

	for (p = filenames; *p; p++) {
		GFile *file;

		file = g_file_new_for_commandline_arg (*p);
		tracker_miner_manager_index_file (manager, file, NULL, &error);

		if (error) {
			g_printerr ("%s: %s\n",
			            _("Could not (re)index file"),
			            error->message);
			g_error_free (error);
			return EXIT_FAILURE;
		}

		g_print ("%s\n", _("(Re)indexing file was successful"));
		g_object_unref (file);
	}

	g_object_unref (manager);

	return EXIT_SUCCESS;
}

static void
miner_file_processed (TrackerMinerManager *miner_manager,
                      const gchar         *miner,
                      const gchar         *uri,
                      const gboolean       status,
                      const gchar         *message,
                      GFile               *root)
{
	g_autoptr(GFile) file = NULL;
	g_autoptr(GError) error = NULL;
	g_autofree gchar *relative_path = NULL;

	file = g_file_new_for_uri (uri);

	relative_path = g_file_get_relative_path (root, file);

	if (g_file_equal (root, file) || relative_path != NULL) {
		/* File is a child of the root. */
		g_debug ("File processed: %s: %s, %i, %s\n", miner, uri, status, message);
	}
}

static gboolean
sigterm_cb (gpointer user_data)
{
	g_message ("Received signal");

	g_main_loop_quit (user_data);

	return G_SOURCE_REMOVE;
}

static gint
index_or_reindex_file_monitor_mode (void)
{
	g_autoptr(TrackerMinerManager) manager = NULL;
	g_autoptr(GMainLoop) main_loop = NULL;
	g_autoptr(GError) error = NULL;
	gchar **p;

	/* Check we were only passed directories. IndexFileForProcess doesn't work
	 * for files. */
	for (p = filenames; *p; p++) {
		g_autoptr(GFile) path = NULL;
		g_autoptr(GFileInfo) info = NULL;

		path = g_file_new_for_commandline_arg (*p);
		info = g_file_query_info (path,
		                          G_FILE_ATTRIBUTE_STANDARD_TYPE,
		                          G_FILE_QUERY_INFO_NONE,
		                          NULL,
		                          &error);

		if (error) {
			g_printerr ("%s: %s\n",
			            _("Error determining file type"),
			            error->message);
			return EXIT_FAILURE;
		}

		if (g_file_info_get_file_type (info) != G_FILE_TYPE_DIRECTORY) {
			g_printerr (_("Could not index file %s: in `--monitor` mode, "
			              "only directories can be indexed.\n"),
			            g_file_peek_path (path));
			return EXIT_FAILURE;
		}
	}

	/* Auto-start the miners here if we need to */
	manager = tracker_miner_manager_new_full (TRUE, &error);
	if (!manager) {
		g_printerr (_("Could not (re)index file, manager could not be created, %s"),
		            error ? error->message : _("No error given"));
		g_printerr ("\n");
		return EXIT_FAILURE;
	}

	for (p = filenames; *p; p++) {
		g_autoptr(GFile) dir = NULL;

		dir = g_file_new_for_commandline_arg (*p);

		g_signal_connect_data (G_OBJECT (manager),
		                       "miner-file-processed",
		                       G_CALLBACK (miner_file_processed),
		                       g_object_ref (dir),
		                       (GClosureNotify)g_object_unref,
		                       0);

		tracker_miner_manager_index_file_for_process (manager, dir, NULL, &error);

		if (error) {
			g_printerr ("%s: %s\n",
			            _("Could not (re)index directory"),
			            error->message);
			return EXIT_FAILURE;
		}

		g_print ("%s\n", _("(Re)indexing directory was successful"));
	}

	main_loop = g_main_loop_new (NULL, 0);

	g_unix_signal_add (SIGINT, sigterm_cb, main_loop);
	g_unix_signal_add (SIGTERM, sigterm_cb, main_loop);

	g_main_loop_run (main_loop);

	return EXIT_SUCCESS;
}

static int
index_run (void)
{
	if (monitor_mode) {
		return index_or_reindex_file_monitor_mode ();
	} else {
		return index_or_reindex_file ();
	}
}

int
main (int argc, const char **argv)
{
	GOptionContext *context;
	GError *error = NULL;
	const gchar *failed;

	setlocale (LC_ALL, "");

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	context = g_option_context_new (NULL);
	g_option_context_add_main_entries (context, entries, NULL);

	argv[0] = "tracker index";

	if (!g_option_context_parse (context, &argc, (char***) &argv, &error)) {
		g_printerr ("%s, %s\n", _("Unrecognized options"), error->message);
		g_error_free (error);
		g_option_context_free (context);
		return EXIT_FAILURE;
	}

	g_option_context_free (context);

	if (!filenames || g_strv_length (filenames) < 1) {
		failed = _("Please specify one or more locations to index.");
	} else {
		failed = NULL;
	}

	if (failed) {
		g_printerr ("%s\n", failed);
		return EXIT_FAILURE;
	}

	if (file_arg) {
		g_message ("The --file arg is no longer needed.");
	}

	return index_run ();
}
