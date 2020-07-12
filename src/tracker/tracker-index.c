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
static gboolean no_wait;
static gchar **filenames;

static GOptionEntry entries[] = {
	{ "file", 'f', 0, G_OPTION_ARG_NONE, &file_arg,
	 N_("Does nothing, provided for compatibility with Tracker 2.0"),
	 NULL },
	{ "monitor", 'm', 0, G_OPTION_ARG_NONE, &monitor_mode,
	  N_("Trigger indexing and wait for CTRL+C, removing the data again on exit."),
	  NULL },
	{ "no-wait", 'w', 0, G_OPTION_ARG_NONE, &no_wait,
	  N_("Don't wait for processing to complete, exit immediately."),
	  NULL },
	{ G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &filenames,
	  N_("FILE"),
	  N_("FILE") },
	{ NULL }
};

GMainLoop *main_loop;

static void
print_indexing_status (GFile                 *root,
                       TrackerIndexingStatus *status)
{
	g_autofree gchar *uri = g_file_get_uri (root);
	g_autoptr(GList) error_list = NULL;

	error_list = tracker_indexing_status_get_errors (status);

	if (tracker_indexing_status_get_n_indexed_files (status) == 1) {
		if (error_list) {
			const gchar *message = error_list->data;
			g_print ("%s\n", message);
		} else {
			/* TRANSLATORS: %s is a URI. */
			g_print (_("File %s was processed successfully."), uri);
			g_print ("\n");
		}
	} else {
		GList *node;

		if (tracker_indexing_status_get_completed (status)) {
			if (error_list) {
				/* TRANSLATORS: %s is a URI. */
				g_print (_("Indexing of %s completed with errors."), uri);
			} else {
				/* TRANSLATORS: %s is a URI. */
				g_print (_("Indexing of %s completed."), uri);
			}
			g_print ("\n");
		}

		if (error_list) {
			for (node = error_list; node; node=node->next) {
				const gchar *message = node->data;
				g_print ("  * %s\n", message);
			}
		}

		g_print (_("%i files were added to the index."),
		         tracker_indexing_status_get_n_indexed_files (status));
		g_print ("\n");
	}

	g_list_free_full (error_list, g_free);
}

static void
index_file_cb (GObject      *source_object,
               GAsyncResult *res,
               gpointer      user_data)
{
	GMainLoop *loop = user_data;
	GError *error = NULL;

	tracker_miner_manager_index_file_finish (TRACKER_MINER_MANAGER (source_object), res, &error);

	if (error) {
		g_error ("Error starting indexing: %s", error->message);
	}

	g_main_loop_quit (loop);
}

static void
index_file_for_process_cb (GObject      *source_object,
               GAsyncResult *res,
               gpointer      user_data)
{
	GMainLoop *loop = user_data;
	GError *error = NULL;

	tracker_miner_manager_index_file_for_process_finish (TRACKER_MINER_MANAGER (source_object), res, NULL);

	if (error) {
		g_error ("Error starting indexing: %s", error->message);
	}

	g_main_loop_quit (loop);
}

static gboolean
g_file_is_directory (GFile *path)
{
	g_autoptr(GFileInfo) info = NULL;
	g_autoptr(GError) error = NULL;

	info = g_file_query_info (path,
	                          G_FILE_ATTRIBUTE_STANDARD_TYPE,
	                          G_FILE_QUERY_INFO_NONE,
	                          NULL,
	                          &error);

	if (error) {
		g_warning ("Error checking if %s is a directory: %s.",
		           g_file_peek_path(path), error->message);
		return FALSE;
	}

	return (g_file_info_get_file_type (info) == G_FILE_TYPE_DIRECTORY);
}

static gboolean
sigterm_cb (gpointer user_data)
{
	g_message ("Received signal");

	g_main_loop_quit (user_data);

	return G_SOURCE_REMOVE;
}

static gint
index_run (void)
{
	g_autoptr(TrackerMinerManager) manager;
	g_autoptr(GMainLoop) main_loop;
	g_autoptr(GError) error = NULL;
	gboolean success = TRUE;
	gchar **p;

	/* Check we were only passed directories. IndexFileForProcess doesn't work
	 * for files. */
	if (monitor_mode) {
		for (p = filenames; *p; p++) {
			g_autoptr(GFile) path = NULL;

			path = g_file_new_for_commandline_arg (*p);

			if (!g_file_is_directory (path)) {
				g_printerr (_("Could not index file %s: in `--monitor` mode, "
				              "only directories can be indexed.\n"),
				            g_file_peek_path (path));
				return EXIT_FAILURE;
			}
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

	main_loop = g_main_loop_new (NULL, 0);

	for (p = filenames; *p; p++) {
		g_autoptr(GFile) file;
		g_autoptr(TrackerIndexingStatus) status;

		file = g_file_new_for_commandline_arg (*p);

		if (monitor_mode) {
			status = tracker_miner_manager_index_file_for_process_async (manager, file, NULL, index_file_for_process_cb, main_loop);
		} else {
			status = tracker_miner_manager_index_file_async (manager, file, NULL, index_file_cb, main_loop);
		}

		if (no_wait) {
			/* We may detect an error straight away, even if we don't wait. */
			if (tracker_indexing_status_had_error (status)) {
				print_indexing_status (file, status);
				success = FALSE;
			} else {
				g_print ("Successfully enqueued %s for indexing.", *p);
				success &= TRUE;
			}
		} else {
			/* Run the main loop until the indexing completes, at which point
			 * index_file_cb() will quit this loop.
			 */
			g_main_loop_run (main_loop);

			print_indexing_status (file, status);

			success &= !(tracker_indexing_status_had_error (status));
		}
	}

	if (monitor_mode) {
		g_print (_("Press CTRL+C to exit and remove the files from the index."));
		g_print ("\n");

		g_unix_signal_add (SIGINT, sigterm_cb, main_loop);
		g_unix_signal_add (SIGTERM, sigterm_cb, main_loop);

		g_main_loop_run (main_loop);
	} else {
		g_print (_("Files may not be monitored for changes. Use --monitor mode to "
		           "avoid stale data being left in your Tracker index."));
		g_print ("\n");
	};

	return success ? EXIT_SUCCESS : EXIT_FAILURE;
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
	} else if (monitor_mode && no_wait) {
		failed = _("The --monitor-mode and --no-wait options are incompatible");
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
