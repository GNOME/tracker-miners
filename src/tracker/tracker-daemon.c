/*
 * Copyright (C) 2010, Nokia <ivan.frade@nokia.com>
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

#include <errno.h>

#ifdef __sun
#include <procfs.h>
#endif

#include <glib.h>
#include <glib-unix.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <locale.h>

#include <libtracker-miners-common/tracker-common.h>
#include <libtracker-sparql/tracker-sparql.h>

#include "tracker-process.h"
#include "tracker-dbus.h"
#include "tracker-miner-manager.h"

typedef struct {
	TrackerSparqlConnection *connection;
	GHashTable *prefixes;
	GStrv filter;
} WatchData;

static GMainLoop *main_loop;
static GHashTable *miners_progress;
static GHashTable *miners_status;
static gint longest_miner_name_length = 0;
static gint paused_length = 0;

static gboolean full_namespaces = FALSE; /* Can be turned on if needed, or made cmd line option */

/* Note:
 * Every time a new option is added, make sure it is considered in the
 * 'STATUS_OPTIONS_ENABLED' macro below
 */
static gboolean status;
static gboolean follow;
static gboolean watch;
static gboolean list_common_statuses;

static gchar *miner_name;
static gchar *pause_reason;
static gchar *pause_for_process_reason;
static gint resume_cookie = -1;
static gboolean list_miners_running;
static gboolean list_miners_available;
static gboolean pause_details;

static gboolean list_processes;
static gboolean start;
static gboolean kill_miners;
static gboolean terminate_miners;
static gchar *backup;
static gchar *restore;

#define DAEMON_OPTIONS_ENABLED() \
	((status || follow || watch || list_common_statuses) || \
	 (miner_name || \
	  pause_reason || \
	  pause_for_process_reason || \
	  resume_cookie != -1 || \
	  list_miners_running || \
	  list_miners_available || \
	  pause_details) || \
	 (list_processes || \
	  start || \
	  kill_miners || \
	  terminate_miners || \
	  backup || \
	  restore));

/* Make sure our statuses are translated (most from libtracker-miner) */
static const gchar *statuses[8] = {
	N_("Unavailable"), /* generic */
	N_("Initializing"),
	N_("Processing…"),
	N_("Fetching…"), /* miner/rss */
	N_("Crawling single directory “%s”"),
	N_("Crawling recursively directory “%s”"),
	N_("Paused"),
	N_("Idle")
};

static GOptionEntry entries[] = {
	/* Status */
	{ "follow", 'f', 0, G_OPTION_ARG_NONE, &follow,
	  N_("Follow status changes as they happen"),
	  NULL
	},
	{ "watch", 'w', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &watch,
	  N_("Watch changes to the database in real time (e.g. resources or files being added)"),
	  NULL
	},
	{ "list-common-statuses", 0, 0, G_OPTION_ARG_NONE, &list_common_statuses,
	  N_("List common statuses for miners"),
	  NULL
	},
	/* Miners */
	{ "pause", 0 , 0, G_OPTION_ARG_STRING, &pause_reason,
	  N_("Pause a miner (you must use this with --miner)"),
	  N_("REASON")
	},
	{ "pause-for-process", 0 , 0, G_OPTION_ARG_STRING, &pause_for_process_reason,
	  N_("Pause a miner while the calling process is alive or until resumed (you must use this with --miner)"),
	  N_("REASON")
	},
	{ "resume", 0 , 0, G_OPTION_ARG_INT, &resume_cookie,
	  N_("Resume a miner (you must use this with --miner)"),
	  N_("COOKIE")
	},
	{ "miner", 0 , 0, G_OPTION_ARG_STRING, &miner_name,
	  N_("Miner to use with --resume or --pause (you can use suffixes, e.g. Files or Applications)"),
	  N_("MINER")
	},
	{ "list-miners-running", 0, 0, G_OPTION_ARG_NONE, &list_miners_running,
	  N_("List all miners currently running"),
	  NULL
	},
	{ "list-miners-available", 0, 0, G_OPTION_ARG_NONE, &list_miners_available,
	  N_("List all miners installed"),
	  NULL
	},
	{ "pause-details", 0, 0, G_OPTION_ARG_NONE, &pause_details,
	  N_("List pause reasons"),
	  NULL
	},
	/* Processes */
	{ "list-processes", 'p', 0, G_OPTION_ARG_NONE, &list_processes,
	  N_("List all Tracker processes") },
	{ "kill", 'k', 0, G_OPTION_ARG_NONE, &kill_miners,
	  N_("Use SIGKILL to stop all miners"),
	  N_("APPS") },
	{ "terminate", 't', 0, G_OPTION_ARG_NONE, &terminate_miners,
	  N_("Use SIGTERM to stop all miners"),
	  N_("APPS") },
	{ "start", 's', 0, G_OPTION_ARG_NONE, &start,
	  N_("Starts miners"),
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
		exit (EXIT_FAILURE);
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
	g_unix_signal_add (SIGTERM, signal_handler, GINT_TO_POINTER (SIGTERM));
	g_unix_signal_add (SIGINT, signal_handler, GINT_TO_POINTER (SIGINT));
}

static gboolean
miner_get_details (TrackerMinerManager  *manager,
                   const gchar          *miner,
                   gchar               **status,
                   gdouble              *progress,
                   gint                 *remaining_time,
                   GStrv                *pause_applications,
                   GStrv                *pause_reasons)
{
	if ((status || progress || remaining_time) &&
	    !tracker_miner_manager_get_status (manager,
	                                       miner,
	                                       status,
	                                       progress,
	                                       remaining_time)) {
		g_printerr (_("Could not get status from miner: %s"), miner);
		return FALSE;
	}

	tracker_miner_manager_is_paused (manager, miner,
	                                 pause_applications,
	                                 pause_reasons);

	if (!(*pause_applications) || !(*pause_reasons)) {
		/* unable to get pause details,
		   already logged by tracker_miner_manager_is_paused */
		return FALSE;
	}

	return TRUE;
}

static void
miner_print_state (TrackerMinerManager *manager,
                   const gchar         *miner_name,
                   const gchar         *status,
                   gdouble              progress,
                   gint                 remaining_time,
                   gboolean             is_running,
                   gboolean             is_paused)
{
	const gchar *name;
	time_t now;
	gchar time_str[64];
	size_t len;
	struct tm *local_time;

	now = time ((time_t *) NULL);
	local_time = localtime (&now);
	len = strftime (time_str,
	                sizeof (time_str) - 1,
	                "%d %b %Y, %H:%M:%S:",
	                local_time);
	time_str[len] = '\0';

	name = tracker_miner_manager_get_display_name (manager, miner_name);

	if (is_running) {
		gchar *progress_str = NULL;
		gchar *remaining_time_str = NULL;

		if (progress >= 0.0 && progress < 1.0) {
			progress_str = g_strdup_printf ("%3u%%", (guint)(progress * 100));
		}

		/* Progress > 0.01 here because we want to avoid any message
		 * during crawling, as we don't have the remaining time in that
		 * case and it would just print "unknown time left" */
		if (progress > 0.01 &&
		    progress < 1.0 &&
		    remaining_time >= 0) {
			/* 0 means that we couldn't properly compute the remaining
			 * time. */
			if (remaining_time > 0) {
				gchar *seconds_str = tracker_seconds_to_string (remaining_time, TRUE);

				/* Translators: %s is a time string */
				remaining_time_str = g_strdup_printf (_("%s remaining"), seconds_str);
				g_free (seconds_str);
			} else {
				remaining_time_str = g_strdup (_("unknown time left"));
			}
		}

		g_print ("%s  %s  %-*.*s %s%-*.*s%s %s %s %s\n",
		         time_str,
		         progress_str ? progress_str : "✓   ",
		         longest_miner_name_length,
		         longest_miner_name_length,
		         name,
		         is_paused ? "(" : " ",
		         paused_length,
		         paused_length,
		         is_paused ? _("PAUSED") : " ",
		         is_paused ? ")" : " ",
		         status ? "-" : "",
		         status ? _(status) : "",
		         remaining_time_str ? remaining_time_str : "");

		g_free (progress_str);
		g_free (remaining_time_str);
	} else {
		g_print ("%s  ✗     %-*.*s  %-*.*s  - %s\n",
		         time_str,
		         longest_miner_name_length,
		         longest_miner_name_length,
		         name,
		         paused_length,
		         paused_length,
		         " ",
		         _("Not running or is a disabled plugin"));
	}
}

static void
manager_miner_progress_cb (TrackerMinerManager *manager,
                           const gchar         *miner_name,
                           const gchar         *status,
                           gdouble              progress,
                           gint                 remaining_time)
{
	GValue *gvalue;

	gvalue = g_slice_new0 (GValue);

	g_value_init (gvalue, G_TYPE_DOUBLE);
	g_value_set_double (gvalue, progress);

	miner_print_state (manager, miner_name, status, progress, remaining_time, TRUE, FALSE);

	g_hash_table_replace (miners_status,
	                      g_strdup (miner_name),
	                      g_strdup (status));
	g_hash_table_replace (miners_progress,
	                      g_strdup (miner_name),
	                      gvalue);
}

static void
manager_miner_paused_cb (TrackerMinerManager *manager,
                         const gchar         *miner_name)
{
	GValue *gvalue;

	gvalue = g_hash_table_lookup (miners_progress, miner_name);

	miner_print_state (manager, miner_name,
	                   g_hash_table_lookup (miners_status, miner_name),
	                   gvalue ? g_value_get_double (gvalue) : 0.0,
	                   -1,
	                   TRUE,
	                   TRUE);
}

static void
manager_miner_resumed_cb (TrackerMinerManager *manager,
                          const gchar         *miner_name)
{
	GValue *gvalue;

	gvalue = g_hash_table_lookup (miners_progress, miner_name);

	miner_print_state (manager, miner_name,
	                   g_hash_table_lookup (miners_status, miner_name),
	                   gvalue ? g_value_get_double (gvalue) : 0.0,
	                   0,
	                   TRUE,
	                   FALSE);
}

static void
miners_progress_destroy_notify (gpointer data)
{
	GValue *value;

	value = data;
	g_value_unset (value);
	g_slice_free (GValue, value);
}

static gchar *
get_shorthand (GHashTable  *prefixes,
               const gchar *namespace)
{
	gchar *hash;

	hash = strrchr (namespace, '#');

	if (hash) {
		gchar *property;
		const gchar *prefix;

		property = hash + 1;
		*hash = '\0';

		prefix = g_hash_table_lookup (prefixes, namespace);

		return g_strdup_printf ("%s:%s", prefix, property);
	}

	return g_strdup (namespace);
}

static inline void
print_key (GHashTable  *prefixes,
           const gchar *key)
{
	if (G_UNLIKELY (full_namespaces)) {
		g_print ("'%s'\n", key);
	} else {
		gchar *shorthand;

		shorthand = get_shorthand (prefixes, key);
		g_print ("'%s'\n", shorthand);
		g_free (shorthand);
	}
}

static void
notifier_events_cb (TrackerNotifier         *notifier,
		    const gchar             *service,
		    const gchar             *graph,
		    GPtrArray               *events,
		    TrackerSparqlConnection *conn)
{
	gint i;

	for (i = 0; i < events->len; i++) {
		TrackerNotifierEvent *event;

		event = g_ptr_array_index (events, i);
		g_print ("  '%s' => '%s'\n", graph,
			 tracker_notifier_event_get_urn (event));
	}
}

static gint
miner_pause (const gchar *miner,
             const gchar *reason,
             gboolean     for_process)
{
	TrackerMinerManager *manager;
	GError *error = NULL;
	gchar *str;
	guint32 cookie;

	/* Don't auto-start the miners here */
	manager = tracker_miner_manager_new_full (FALSE, &error);
	if (!manager) {
		g_printerr (_("Could not pause miner, manager could not be created, %s"),
		            error ? error->message : _("No error given"));
		g_printerr ("\n");
		g_clear_error (&error);
		return EXIT_FAILURE;
	}

	str = g_strdup_printf (_("Attempting to pause miner “%s” with reason “%s”"),
	                       miner,
	                       reason);
	g_print ("%s\n", str);
	g_free (str);

	if (for_process) {
		if (!tracker_miner_manager_pause_for_process (manager, miner, reason, &cookie)) {
			g_printerr (_("Could not pause miner: %s"), miner);
			g_printerr ("\n");
			return EXIT_FAILURE;
		}
	} else {
		if (!tracker_miner_manager_pause (manager, miner, reason, &cookie)) {
			g_printerr (_("Could not pause miner: %s"), miner);
			g_printerr ("\n");
			return EXIT_FAILURE;
		}
	}

	str = g_strdup_printf (_("Cookie is %d"), cookie);
	g_print ("  %s\n", str);
	g_free (str);

	if (for_process) {
		GMainLoop *main_loop;

		g_print ("%s\n", _("Press Ctrl+C to stop"));

		main_loop = g_main_loop_new (NULL, FALSE);
		/* Block until Ctrl+C */
		g_main_loop_run (main_loop);
		g_object_unref (main_loop);
	}

	g_object_unref (manager);

	return EXIT_SUCCESS;
}

static gint
miner_resume (const gchar *miner,
              gint         cookie)
{
	TrackerMinerManager *manager;
	GError *error = NULL;
	gchar *str;

	/* Don't auto-start the miners here */
	manager = tracker_miner_manager_new_full (FALSE, &error);
	if (!manager) {
		g_printerr (_("Could not resume miner, manager could not be created, %s"),
		            error ? error->message : _("No error given"));
		g_printerr ("\n");
		g_clear_error (&error);
		return EXIT_FAILURE;
	}

	str = g_strdup_printf (_("Attempting to resume miner %s with cookie %d"),
	                       miner,
	                       cookie);
	g_print ("%s\n", str);
	g_free (str);

	if (!tracker_miner_manager_resume (manager, miner, cookie)) {
		g_printerr (_("Could not resume miner: %s"), miner);
		return EXIT_FAILURE;
	}

	g_print ("  %s\n", _("Done"));

	g_object_unref (manager);

	return EXIT_SUCCESS;
}

static gint
miner_list (gboolean available,
            gboolean running)
{
	TrackerMinerManager *manager;
	GError *error = NULL;

	/* Don't auto-start the miners here */
	manager = tracker_miner_manager_new_full (FALSE, &error);
	if (!manager) {
		g_printerr (_("Could not list miners, manager could not be created, %s"),
		            error ? error->message : _("No error given"));
		g_printerr ("\n");
		g_clear_error (&error);
		return EXIT_FAILURE;
	}

	if (available) {
		GSList *miners_available;
		gchar *str;
		GSList *l;

		miners_available = tracker_miner_manager_get_available (manager);

		str = g_strdup_printf (ngettext ("Found %d miner installed",
		                                 "Found %d miners installed",
		                                 g_slist_length (miners_available)),
		                       g_slist_length (miners_available));

		g_print ("%s%s\n", str, g_slist_length (miners_available) > 0 ? ":" : "");
		g_free (str);

		for (l = miners_available; l; l = l->next) {
			g_print ("  %s\n", (gchar*) l->data);
		}

		g_slist_foreach (miners_available, (GFunc) g_free, NULL);
		g_slist_free (miners_available);
	}

	if (running) {
		GSList *miners_running;
		gchar *str;
		GSList *l;

		miners_running = tracker_miner_manager_get_running (manager);

		str = g_strdup_printf (ngettext ("Found %d miner running",
		                                 "Found %d miners running",
		                                 g_slist_length (miners_running)),
		                       g_slist_length (miners_running));

		g_print ("%s%s\n", str, g_slist_length (miners_running) > 0 ? ":" : "");
		g_free (str);

		for (l = miners_running; l; l = l->next) {
			g_print ("  %s\n", (gchar*) l->data);
		}

		g_slist_foreach (miners_running, (GFunc) g_free, NULL);
		g_slist_free (miners_running);
	}

	g_object_unref (manager);

	return EXIT_SUCCESS;
}

static gint
miner_pause_details (void)
{
	TrackerMinerManager *manager;
	GError *error = NULL;
	GSList *miners_running, *l;
	gint paused_miners = 0;

	/* Don't auto-start the miners here */
	manager = tracker_miner_manager_new_full (FALSE, &error);
	if (!manager) {
		g_printerr (_("Could not get pause details, manager could not be created, %s"),
		            error ? error->message : _("No error given"));
		g_printerr ("\n");
		g_clear_error (&error);
		return EXIT_FAILURE;
	}

	miners_running = tracker_miner_manager_get_running (manager);

	if (!miners_running) {
		g_print ("%s\n", _("No miners are running"));

		g_slist_foreach (miners_running, (GFunc) g_free, NULL);
		g_slist_free (miners_running);
		g_object_unref (manager);

		return EXIT_SUCCESS;
	}

	for (l = miners_running; l; l = l->next) {
		const gchar *name;
		GStrv pause_applications, pause_reasons;
		gint i;

		name = tracker_miner_manager_get_display_name (manager, l->data);

		if (!name) {
			g_critical ("Could not get name for '%s'", (gchar *) l->data);
			continue;
		}

		tracker_miner_manager_is_paused (manager,
		                                 l->data,
		                                 &pause_applications,
		                                 &pause_reasons);

		if (!pause_applications || !pause_reasons) {
			/* unable to get pause details,
			   already logged by tracker_miner_manager_is_paused */
			continue;
		}

		if (!(*pause_applications) || !(*pause_reasons)) {
			g_strfreev (pause_applications);
			g_strfreev (pause_reasons);
			continue;
		}

		paused_miners++;
		if (paused_miners == 1) {
			g_print ("%s:\n", _("Miners"));
		}

		g_print ("  %s:\n", name);

		for (i = 0; pause_applications[i] != NULL; i++) {
			g_print ("    %s: '%s', %s: '%s'\n",
			         _("Application"),
			         pause_applications[i],
			         _("Reason"),
			         pause_reasons[i]);
		}

		g_strfreev (pause_applications);
		g_strfreev (pause_reasons);
	}

	if (paused_miners < 1) {
		g_print ("%s\n", _("No miners are paused"));
	}

	g_slist_foreach (miners_running, (GFunc) g_free, NULL);
	g_slist_free (miners_running);

	g_object_unref (manager);

	return EXIT_SUCCESS;
}

static gint
daemon_run (void)
{
	TrackerMinerManager *manager;

	/* --follow implies --status */
	if (follow) {
		status = TRUE;
	}

	if (watch) {
		TrackerSparqlConnection *sparql_connection;
		TrackerNotifier *notifier;
		GError *error = NULL;

		sparql_connection = tracker_sparql_connection_bus_new ("org.freedesktop.Tracker3.Miner.Files",
		                                                       NULL, NULL, &error);

		if (!sparql_connection) {
			g_critical ("%s, %s",
			            _("Could not get SPARQL connection"),
			            error ? error->message : _("No error given"));
			g_clear_error (&error);
			return EXIT_FAILURE;
		}

		notifier = tracker_sparql_connection_create_notifier (sparql_connection);
		g_signal_connect (notifier, "events",
				  G_CALLBACK (notifier_events_cb), sparql_connection);
		g_object_unref (sparql_connection);

		g_print ("%s\n", _("Now listening for resource updates to the database"));
		g_print ("%s\n\n", _("All nie:plainTextContent properties are omitted"));
		g_print ("%s\n", _("Press Ctrl+C to stop"));

		main_loop = g_main_loop_new (NULL, FALSE);
		g_main_loop_run (main_loop);
		g_main_loop_unref (main_loop);
		g_object_unref (notifier);

		return EXIT_SUCCESS;
	}

	if (list_common_statuses) {
		gint i;

		g_print ("%s:\n", _("Common statuses include"));

		for (i = 0; i < G_N_ELEMENTS (statuses); i++) {
			g_print ("  %s\n", _(statuses[i]));
		}

		return EXIT_SUCCESS;
	}

	if (status) {
		GError *error = NULL;
		GSList *miners_available;
		GSList *miners_running;
		GSList *l;

		/* Don't auto-start the miners here */
		manager = tracker_miner_manager_new_full (FALSE, &error);
		if (!manager) {
			g_printerr (_("Could not get status, manager could not be created, %s"),
			            error ? error->message : _("No error given"));
			g_printerr ("\n");
			g_clear_error (&error);
			return EXIT_FAILURE;
		}

		miners_available = tracker_miner_manager_get_available (manager);
		miners_running = tracker_miner_manager_get_running (manager);

		/* Work out lengths for output spacing */
		paused_length = strlen (_("PAUSED"));

		for (l = miners_available; l; l = l->next) {
			const gchar *name;

			name = tracker_miner_manager_get_display_name (manager, l->data);
			longest_miner_name_length = MAX (longest_miner_name_length, strlen (name));
		}

		/* Display states */
		g_print ("%s:\n", _("Miners"));

		for (l = miners_available; l; l = l->next) {
			const gchar *name;
			gboolean is_running;

			name = tracker_miner_manager_get_display_name (manager, l->data);
			if (!name) {
				g_critical (_("Could not get display name for miner “%s”"),
				            (const gchar*) l->data);
				continue;
			}

			is_running = tracker_string_in_gslist (l->data, miners_running);

			if (is_running) {
				GStrv pause_applications, pause_reasons;
				gchar *status = NULL;
				gdouble progress;
				gint remaining_time;
				gboolean is_paused;

				if (!miner_get_details (manager,
				                        l->data,
				                        &status,
				                        &progress,
				                        &remaining_time,
				                        &pause_applications,
				                        &pause_reasons)) {
					continue;
				}

				is_paused = *pause_applications || *pause_reasons;

				miner_print_state (manager,
				                   l->data,
				                   status,
				                   progress,
				                   remaining_time,
				                   TRUE,
				                   is_paused);

				g_strfreev (pause_applications);
				g_strfreev (pause_reasons);
				g_free (status);
			} else {
				miner_print_state (manager, l->data, NULL, 0.0, -1, FALSE, FALSE);
			}
		}

		g_slist_foreach (miners_available, (GFunc) g_free, NULL);
		g_slist_free (miners_available);

		g_slist_foreach (miners_running, (GFunc) g_free, NULL);
		g_slist_free (miners_running);

		if (!follow) {
			/* Do nothing further */
			g_print ("\n");
			return EXIT_SUCCESS;
		}

		g_print ("%s\n", _("Press Ctrl+C to stop"));

		g_signal_connect (manager, "miner-progress",
		                  G_CALLBACK (manager_miner_progress_cb), NULL);
		g_signal_connect (manager, "miner-paused",
		                  G_CALLBACK (manager_miner_paused_cb), NULL);
		g_signal_connect (manager, "miner-resumed",
		                  G_CALLBACK (manager_miner_resumed_cb), NULL);

		initialize_signal_handler ();

		miners_progress = g_hash_table_new_full (g_str_hash,
		                                         g_str_equal,
		                                         (GDestroyNotify) g_free,
		                                         (GDestroyNotify) miners_progress_destroy_notify);
		miners_status = g_hash_table_new_full (g_str_hash,
		                                       g_str_equal,
		                                       (GDestroyNotify) g_free,
		                                       (GDestroyNotify) g_free);

		main_loop = g_main_loop_new (NULL, FALSE);
		g_main_loop_run (main_loop);
		g_main_loop_unref (main_loop);

		g_hash_table_unref (miners_progress);
		g_hash_table_unref (miners_status);

		if (manager) {
			g_object_unref (manager);
		}

		return EXIT_SUCCESS;
	}

	/* Miners */
	if (pause_reason && resume_cookie != -1) {
		g_printerr ("%s\n",
		            _("You can not use miner pause and resume switches together"));
		return EXIT_FAILURE;
	}

	if ((pause_reason || pause_for_process_reason  || resume_cookie != -1) && !miner_name) {
		g_printerr ("%s\n",
		            _("You must provide the miner for pause or resume commands"));
		return EXIT_FAILURE;
	}

	if ((!pause_reason && !pause_for_process_reason && resume_cookie == -1) && miner_name) {
		g_printerr ("%s\n",
		            _("You must provide a pause or resume command for the miner"));
		return EXIT_FAILURE;
	}

	/* Known actions */

	if (list_miners_running || list_miners_available) {
		return miner_list (list_miners_available,
		                   list_miners_running);
	}

	if (pause_reason) {
		return miner_pause (miner_name, pause_reason, FALSE);
	}

	if (pause_for_process_reason) {
		return miner_pause (miner_name, pause_for_process_reason, TRUE);
	}

	if (resume_cookie != -1) {
		return miner_resume (miner_name, resume_cookie);
	}

	if (pause_details) {
		return miner_pause_details ();
	}

	/* Processes */
	GError *error = NULL;

	/* Constraints */

	if (kill_miners && terminate_miners) {
		g_printerr ("%s\n",
		            _("You can not use the --kill and --terminate arguments together"));
		return EXIT_FAILURE;
	}

	if (list_processes) {
		GSList *pids, *l;
		gchar *str;

		pids = tracker_process_find_all ();

		str = g_strdup_printf (g_dngettext (NULL,
		                                    "Found %d PID…",
		                                    "Found %d PIDs…",
		                                    g_slist_length (pids)),
		                       g_slist_length (pids));
		g_print ("%s\n", str);
		g_free (str);

		for (l = pids; l; l = l->next) {
			TrackerProcessData *pd = l->data;

			str = g_strdup_printf (_("Found process ID %d for “%s”"), pd->pid, pd->cmd);
			g_print ("%s\n", str);
			g_free (str);
		}

		g_slist_foreach (pids, (GFunc) tracker_process_data_free, NULL);
		g_slist_free (pids);

		return EXIT_SUCCESS;
	}

	if (kill_miners || terminate_miners) {
		gint retval = 0;

		if (kill_miners)
			retval = tracker_process_stop (TRACKER_PROCESS_TYPE_NONE, TRACKER_PROCESS_TYPE_MINERS);
		else if (terminate_miners)
			retval = tracker_process_stop (TRACKER_PROCESS_TYPE_MINERS, TRACKER_PROCESS_TYPE_NONE);

		return retval;
	}

	if (start) {
		TrackerMinerManager *manager;
		GSList *miners, *l;

		g_print ("%s\n", _("Starting miners…"));

		/* Auto-start the miners here */
		manager = tracker_miner_manager_new_full (TRUE, &error);
		if (!manager) {
			g_printerr (_("Could not start miners, manager could not be created, %s"),
			            error ? error->message : _("No error given"));
			g_printerr ("\n");
			g_clear_error (&error);
			return EXIT_FAILURE;
		}

		miners = tracker_miner_manager_get_available (manager);

		/* Get the status of all miners, this will start all
		 * miners not already running.
		 */
		for (l = miners; l; l = l->next) {
			const gchar *display_name;
			gdouble progress = 0.0;

			display_name = tracker_miner_manager_get_display_name (manager, l->data);

			if (!tracker_miner_manager_get_status (manager,
			                                       l->data,
			                                       NULL,
			                                       &progress,
			                                       NULL)) {
				g_printerr ("  ✗ %s (%s)\n",
				            display_name,
				            _("perhaps a disabled plugin?"));
			} else {
				g_print ("  ✓ %s\n",
				         display_name);
			}

			g_free (l->data);
		}

		g_slist_free (miners);
		g_object_unref (manager);

		return EXIT_SUCCESS;
	}

	/* All known options have their own exit points */
	g_warn_if_reached ();

	return EXIT_FAILURE;
}

static int
daemon_run_default (void)
{
	/* Enable status output in the default run */
	status = TRUE;

	return daemon_run ();
}

static gboolean
daemon_options_enabled (void)
{
	return DAEMON_OPTIONS_ENABLED ();
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
	g_option_context_set_summary (context, _("If no arguments are given, the status of the data miners is shown"));

	argv[0] = "tracker daemon";

	if (!g_option_context_parse (context, &argc, (char***) &argv, &error)) {
		g_printerr ("%s, %s\n", _("Unrecognized options"), error->message);
		g_error_free (error);
		g_option_context_free (context);
		return EXIT_FAILURE;
	}

	g_option_context_free (context);

	if (daemon_options_enabled ()) {
		return daemon_run ();
	}

	return daemon_run_default ();
}
