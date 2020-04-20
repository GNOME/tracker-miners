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
 */

#include "config-miners.h"

#include <stdlib.h>
#include <locale.h>

#include <glib/gi18n.h>

#include "tracker-writeback.h"

#define ABOUT	  \
	"Tracker " PACKAGE_VERSION "\n"

#define LICENSE	  \
	"This program is free software and comes without any warranty.\n" \
	"It is licensed under version 2 or later of the General Public " \
	"License which can be viewed at:\n" \
	"\n" \
	"  http://www.gnu.org/licenses/gpl.txt\n"

#define QUIT_TIMEOUT 30 /* 1/2 minutes worth of seconds */

static gboolean      version;
static gint          verbosity = -1;
static gboolean      disable_shutdown;

static GOptionEntry  entries[] = {
	{ "version", 'V', 0,
	  G_OPTION_ARG_NONE, &version,
	  N_("Displays version information"),
	  NULL },
	{ "verbosity", 'v', 0,
	  G_OPTION_ARG_INT, &verbosity,
	  N_("Logging, 0 = errors only, "
	     "1 = minimal, 2 = detailed and 3 = debug (default=0)"),
	  NULL },
	/* Debug run is used to avoid that the mainloop exits, so that
	 * as a developer you can be relax when running the tool in gdb */
	{ "disable-shutdown", 'd', 0,
	  G_OPTION_ARG_NONE, &disable_shutdown,
	  N_("Disable shutting down after 30 seconds of inactivity"),
	  NULL },
	{ NULL }
};


int
main (int   argc,
      char *argv[])
{
	TrackerController *controller;
	GOptionContext *context;
	GMainLoop *loop;
	GError *error = NULL;
	guint shutdown_timeout;

	/* Set up locale */
	setlocale (LC_ALL, "");

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	/* Translators: this message will appear immediately after the
	 * usage string - Usage: COMMAND <THIS_MESSAGE>
	 */
	context = g_option_context_new (_("— start the tracker writeback service"));

	g_option_context_add_main_entries (context, entries, NULL);
	g_option_context_parse (context, &argc, &argv, &error);
	g_option_context_free (context);

	if (version) {
		g_print ("\n" ABOUT "\n" LICENSE "\n");
		return EXIT_SUCCESS;
	}

	if (disable_shutdown) {
		shutdown_timeout = 0;
	} else {
		shutdown_timeout = QUIT_TIMEOUT;
	}

	controller = tracker_controller_new (shutdown_timeout, &error);

	if (error) {
		g_critical ("Error creating controller: %s", error->message);
		g_error_free (error);
		return EXIT_FAILURE;
	}

	g_message ("Main thread is: %p", g_thread_self ());

	loop = g_main_loop_new (NULL, FALSE);
	g_main_loop_run (loop);

	g_object_unref (controller);

	g_main_loop_unref (loop);

	return EXIT_SUCCESS;
}
