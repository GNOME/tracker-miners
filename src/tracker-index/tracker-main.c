/*
 * Copyright (C) 2020, Red Hat Inc

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

#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#include <gio/gio.h>
#include <glib-unix.h>
#include <glib-object.h>
#include <glib/gi18n.h>

#include <libtracker-miners-common/tracker-common.h>

#include "tracker-miner-files-index.h"

#define ABOUT	  \
	"Tracker " PACKAGE_VERSION "\n"

#define LICENSE	  \
	"This program is free software and comes without any warranty.\n" \
	"It is licensed under version 2 or later of the General Public " \
	"License which can be viewed at:\n" \
	"\n" \
	"  http://www.gnu.org/licenses/gpl.txt\n"

#define SECONDS_PER_DAY 60 * 60 * 24

#define DBUS_NAME_SUFFIX "Tracker3.Miner.Files"
#define DBUS_PATH "/org/freedesktop/Tracker3/Miner/Files"
#define LOCALE_FILENAME "locale-for-miner-apps.txt"

static GMainLoop *main_loop;

static gboolean version;

static GOptionEntry entries[] = {
	{ "version", 'V', 0,
	  G_OPTION_ARG_NONE, &version,
	  N_("Displays version information"),
	  NULL },
	{ NULL }
};

static gboolean
signal_handler (gpointer user_data)
{
	int signo = GPOINTER_TO_INT (user_data);

	static gboolean in_loop = FALSE;

	/* Die if we get re-entrant signals handler calls */
	if (in_loop) {
		_exit (EXIT_FAILURE);
	}

	switch (signo) {
	case SIGTERM:
	case SIGINT:
		in_loop = TRUE;
		g_main_loop_quit (main_loop);

		/* Fall through */
	default:
		if (g_strsignal (signo)) {
			g_message ("Received signal:%d->'%s'",
			           signo,
			           g_strsignal (signo));
		}
		break;
	}

	return G_SOURCE_CONTINUE;
}

static void
initialize_signal_handler (void)
{
#ifndef G_OS_WIN32
	g_unix_signal_add (SIGTERM, signal_handler, GINT_TO_POINTER (SIGTERM));
	g_unix_signal_add (SIGINT, signal_handler, GINT_TO_POINTER (SIGINT));
#endif /* G_OS_WIN32 */
}

static void
files_index_close_cb (GMainLoop *main_loop)
{
	g_debug ("No further watched folders, closing");
	g_main_loop_quit (main_loop);
}

int
main (gint argc, gchar *argv[])
{
	GOptionContext *context;
	GError *error = NULL;
	GDBusConnection *connection;
	TrackerMinerFilesIndex *index;

	main_loop = NULL;

	setlocale (LC_ALL, "");

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	/* Set timezone info */
	tzset ();

	/* Translators: this messagge will apper immediately after the
	 * usage string - Usage: COMMAND <THIS_MESSAGE>
	 */
	context = g_option_context_new (_("— start the tracker index proxy"));

	g_option_context_add_main_entries (context, entries, NULL);
	g_option_context_parse (context, &argc, &argv, &error);
	g_option_context_free (context);

	if (error) {
		g_printerr ("%s\n", error->message);
		g_error_free (error);
		return EXIT_FAILURE;
	}

	if (version) {
		g_print ("\n" ABOUT "\n" LICENSE "\n");
		return EXIT_SUCCESS;
	}

	connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
	if (error) {
		g_critical ("Could not create DBus connection: %s\n",
		            error->message);
		g_error_free (error);
		return EXIT_FAILURE;
	}

	main_loop = g_main_loop_new (NULL, FALSE);

	index = tracker_miner_files_index_new ();
	g_signal_connect (index, "close",
	                  G_CALLBACK (files_index_close_cb), main_loop);

	/* Request DBus name */
	if (!tracker_dbus_request_name (connection,
	                                "org.freedesktop.Tracker3.Miner.Files.Index",
	                                &error)) {
		g_critical ("Could not request DBus name: %s",
		            error->message);
		g_error_free (error);
		return EXIT_FAILURE;
	}

	initialize_signal_handler ();

	/* Go, go, go! */
	g_main_loop_run (main_loop);

	g_debug ("Shutdown started");

	g_main_loop_unref (main_loop);

	g_object_unref (connection);
	g_object_unref (index);

	g_print ("\nOK\n\n");

	return EXIT_SUCCESS;
}
