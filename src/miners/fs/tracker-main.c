/*
 * Copyright (C) 2008, Nokia <ivan.frade@nokia.com>

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

#include <glib.h>
#include <glib-unix.h>
#include <glib-object.h>
#include <glib/gi18n.h>

#include <libtracker-miners-common/tracker-common.h>
#include <libtracker-sparql/tracker-sparql.h>
#include <libtracker-miner/tracker-miner.h>

#include "tracker-config.h"
#include "tracker-miner-files.h"
#include "tracker-miner-files-index.h"
#include "tracker-writeback.h"

#define ABOUT	  \
	"Tracker " PACKAGE_VERSION "\n"

#define LICENSE	  \
	"This program is free software and comes without any warranty.\n" \
	"It is licensed under version 2 or later of the General Public " \
	"License which can be viewed at:\n" \
	"\n" \
	"  http://www.gnu.org/licenses/gpl.txt\n"

#define SECONDS_PER_DAY 60 * 60 * 24

#define DBUS_NAME_SUFFIX "Tracker1.Miner.Files"
#define DBUS_PATH "/org/freedesktop/Tracker1/Miner/Files"
#define LOCALE_FILENAME "locale-for-miner-apps.txt"

static GMainLoop *main_loop;

static gint verbosity = -1;
static gint initial_sleep = -1;
static gboolean no_daemon;
static gchar *eligible;
static gboolean version;
static guint miners_timeout_id = 0;
static gboolean do_crawling = FALSE;
static gchar *domain_ontology_name = NULL;

static GOptionEntry entries[] = {
	{ "verbosity", 'v', 0,
	  G_OPTION_ARG_INT, &verbosity,
	  N_("Logging, 0 = errors only, "
	     "1 = minimal, 2 = detailed and 3 = debug (default=0)"),
	  NULL },
	{ "initial-sleep", 's', 0,
	  G_OPTION_ARG_INT, &initial_sleep,
	  N_("Initial sleep time in seconds, "
	     "0->1000 (default=15)"),
	  NULL },
	{ "no-daemon", 'n', 0,
	  G_OPTION_ARG_NONE, &no_daemon,
	  N_("Runs until all configured locations are indexed and then exits"),
	  NULL },
	{ "eligible", 'e', 0,
	  G_OPTION_ARG_FILENAME, &eligible,
	  N_("Checks if FILE is eligible for being mined based on configuration"),
	  N_("FILE") },
	{ "domain-ontology", 'd', 0,
	  G_OPTION_ARG_STRING, &domain_ontology_name,
	  N_("Runs for a specific domain ontology"),
	  NULL },
	{ "version", 'V', 0,
	  G_OPTION_ARG_NONE, &version,
	  N_("Displays version information"),
	  NULL },
	{ NULL }
};

static void
sanity_check_option_values (TrackerConfig *config)
{
	g_message ("General options:");
	g_message ("  Verbosity  ............................  %d",
	           tracker_config_get_verbosity (config));
	g_message ("  Sched Idle  ...........................  %d",
	           tracker_config_get_sched_idle (config));
	g_message ("  Initial Sleep  ........................  %d",
	           tracker_config_get_initial_sleep (config));
	g_message ("  Writeback  ............................  %s",
	           tracker_config_get_enable_writeback (config) ? "yes" : "no");

	g_message ("Indexer options:");
	g_message ("  Throttle level  .......................  %d",
	           tracker_config_get_throttle (config));
	g_message ("  Indexing while on battery  ............  %s (first time only = %s)",
	           tracker_config_get_index_on_battery (config) ? "yes" : "no",
	           tracker_config_get_index_on_battery_first_time (config) ? "yes" : "no");

	if (tracker_config_get_low_disk_space_limit (config) == -1) {
		g_message ("  Low disk space limit  .................  Disabled");
	} else {
		g_message ("  Low disk space limit  .................  %d%%",
		           tracker_config_get_low_disk_space_limit (config));
	}
}

static void
miner_reset_applications (TrackerMiner *miner)
{
	const gchar *sparql;
	GError *error = NULL;

	sparql =
		"DELETE { GRAPH <" TRACKER_OWN_GRAPH_URN "> { ?icon a rdfs:Resource } } "
		"WHERE { GRAPH <" TRACKER_OWN_GRAPH_URN "> { ?app a nfo:SoftwareApplication; nfo:softwareIcon ?icon } } "
		"DELETE { GRAPH <" TRACKER_OWN_GRAPH_URN "> { ?app nie:dataSource <" TRACKER_PREFIX_TRACKER "extractor-data-source> } } "
		"WHERE { GRAPH <" TRACKER_OWN_GRAPH_URN "> { ?app a nfo:SoftwareApplication } }";

	/* Execute a sync update, we don't want the apps miner to start before
	 * we finish this. */
	tracker_sparql_connection_update (tracker_miner_get_connection (miner),
	                                  sparql, G_PRIORITY_HIGH,
	                                  NULL, &error);

	if (error) {
		/* Some error happened performing the query, not good */
		g_critical ("Couldn't reset indexed applications: %s",
		            error ? error->message : "unknown error");
		g_error_free (error);
	}
}

static void
save_current_locale (void)
{
	GError *error = NULL;
	gchar *locale = tracker_locale_get (TRACKER_LOCALE_LANGUAGE);
	gchar *locale_file = g_build_filename (g_get_user_cache_dir (), "tracker", LOCALE_FILENAME, NULL);

	g_message ("Saving locale used to index applications");
	g_message ("  Creating locale file '%s'", locale_file);

	if (locale == NULL) {
		locale = g_strdup ("");
	}

	if (!g_file_set_contents (locale_file, locale, -1, &error)) {
		g_message ("  Could not set file contents, %s",
		           error ? error->message : "no error given");
		g_clear_error (&error);
	}

	g_free (locale);
	g_free (locale_file);
}

static gboolean
detect_locale_changed (TrackerMiner *miner)
{
	gchar *locale_file;
	gchar *previous_locale = NULL;
	gchar *current_locale;
	gboolean changed;

	locale_file = g_build_filename (g_get_user_cache_dir (), "tracker", LOCALE_FILENAME, NULL);

	if (G_LIKELY (g_file_test (locale_file, G_FILE_TEST_EXISTS))) {
		gchar *contents;

		/* Check locale is correct */
		if (G_LIKELY (g_file_get_contents (locale_file, &contents, NULL, NULL))) {
			if (contents &&
			    contents[0] == '\0') {
				g_critical ("  Empty locale file found at '%s'", locale_file);
				g_free (contents);
			} else {
				/* Re-use contents */
				previous_locale = contents;
			}
		} else {
			g_critical ("  Could not get content of file '%s'", locale_file);
		}
	} else {
		g_message ("  Could not find locale file:'%s'", locale_file);
	}

	g_free (locale_file);

	current_locale = tracker_locale_get (TRACKER_LOCALE_LANGUAGE);

	/* Note that having both to NULL is actually valid, they would default
	 * to the unicode collation without locale-specific stuff. */
	if (g_strcmp0 (previous_locale, current_locale) != 0) {
		g_message ("Locale change detected from '%s' to '%s'...",
		           previous_locale, current_locale);
		changed = TRUE;
	} else {
		g_message ("Current and previous locales match: '%s'", previous_locale);
		changed = FALSE;
	}

	g_free (current_locale);
	g_free (previous_locale);

	if (changed) {
		g_message ("Resetting nfo:Software due to locale change...");
		miner_reset_applications (miner);
	}

	return changed;
}

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
initialize_priority_and_scheduling (void)
{
	/* Set CPU priority */
	tracker_sched_idle ();

	/* Set disk IO priority and scheduling */
	tracker_ioprio_init ();

	/* Set process priority:
	 * The nice() function uses attribute "warn_unused_result" and
	 * so complains if we do not check its returned value. But it
	 * seems that since glibc 2.2.4, nice() can return -1 on a
	 * successful call so we have to check value of errno too.
	 * Stupid...
	 */

	g_message ("Setting priority nice level to 19");

	errno = 0;
	if (nice (19) == -1 && errno != 0) {
		const gchar *str = g_strerror (errno);

		g_message ("Couldn't set nice value to 19, %s",
		           str ? str : "no error given");
	}
}

static gboolean
should_crawl (TrackerConfig *config,
              gboolean      *forced)
{
	gint crawling_interval;

	crawling_interval = tracker_config_get_crawling_interval (config);

	g_message ("Checking whether to crawl file system based on configured crawling interval:");

	if (crawling_interval == -2) {
		g_message ("  Disabled");
		return FALSE;
	} else if (crawling_interval == -1) {
		g_message ("  Maybe (depends on a clean last shutdown)");
		return TRUE;
	} else if (crawling_interval == 0) {
		g_message ("  Forced");

		if (forced) {
			*forced = TRUE;
		}

		return TRUE;
	} else {
		guint64 then, now;

		then = tracker_miner_files_get_last_crawl_done ();

		if (then < 1) {
			return TRUE;
		}

		now = (guint64) time (NULL);

		if (now < then + (crawling_interval * SECONDS_PER_DAY)) {
			g_message ("  Postponed");
			return FALSE;
		} else {
			g_message ("  (More than) %d days after last crawling, enabled", crawling_interval);
			return TRUE;
		}
	}
}

static void
miner_do_start (TrackerMiner *miner)
{
	if (!tracker_miner_is_started (miner)) {
		g_message ("Starting filesystem miner...");
		tracker_miner_start (miner);
	}
}

static gboolean
miner_start_idle_cb (gpointer data)
{
	TrackerMiner *miner = data;

	miners_timeout_id = 0;
	miner_do_start (miner);
	return G_SOURCE_REMOVE;
}

static void
miner_start (TrackerMiner  *miner,
	     TrackerConfig *config,
	     gboolean       do_mtime_checking)
{
	gint initial_sleep;

	if (!do_mtime_checking) {
		g_debug ("Avoiding initial sleep, no mtime check needed");
		miner_do_start (miner);
		return;
	}

	/* If requesting to run as no-daemon, start right away */
	if (no_daemon) {
		miner_do_start (miner);
		return;
	}

	/* If no need to initially sleep, start right away */
	initial_sleep = tracker_config_get_initial_sleep (config);

	if (initial_sleep <= 0) {
		miner_do_start (miner);
		return;
	}

	g_debug ("Performing initial sleep of %d seconds",
	         initial_sleep);
	miners_timeout_id = g_timeout_add_seconds (initial_sleep,
	                                           miner_start_idle_cb,
	                                           miner);
}

static void
miner_finished_cb (TrackerMinerFS *fs,
                   gdouble         seconds_elapsed,
                   guint           total_directories_found,
                   guint           total_directories_ignored,
                   guint           total_files_found,
                   guint           total_files_ignored,
                   gpointer        user_data)
{
	g_info ("Finished mining in seconds:%f, total directories:%d, total files:%d",
	        seconds_elapsed,
	        total_directories_found,
	        total_files_found);

	if (do_crawling) {
		tracker_miner_files_set_last_crawl_done (TRUE);
	}

	/* We're not sticking around for file updates, so stop
	 * the mainloop and exit.
	 */
	if (no_daemon && main_loop) {
		// FIXME: wait for extractor to finish
/*		g_main_loop_quit (main_loop);*/
	}
}

static GList *
get_dir_children_as_gfiles (const gchar *path)
{
	GList *children = NULL;
	GDir *dir;

	dir = g_dir_open (path, 0, NULL);

	if (dir) {
		const gchar *basename;

		while ((basename = g_dir_read_name (dir)) != NULL) {
			GFile *child;
			gchar *str;

			str = g_build_filename (path, basename, NULL);
			child = g_file_new_for_path (str);
			g_free (str);

			children = g_list_prepend (children, child);
		}

		g_dir_close (dir);
	}

	return children;
}

static void
dummy_log_handler (const gchar    *domain,
                   GLogLevelFlags  log_level,
                   const gchar    *message,
                   gpointer        user_data)
{
	return;
}

static void
check_eligible (void)
{
	TrackerConfig *config;
	GFile *file;
	GFileInfo *info;
	GError *error = NULL;
	gchar *path;
	guint log_handler_id;
	gboolean exists = TRUE;
	gboolean is_dir;
	gboolean print_dir_check;
	gboolean print_dir_check_with_content;
	gboolean print_file_check;
	gboolean print_monitor_check;
	gboolean would_index = TRUE;
	gboolean would_notice = TRUE;

	/* Set log handler for library messages */
	log_handler_id = g_log_set_handler (NULL,
	                                    G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL,
	                                    dummy_log_handler,
	                                    NULL);

	g_log_set_default_handler (dummy_log_handler, NULL);

	/* Start check */
	file = g_file_new_for_commandline_arg (eligible);
	info = g_file_query_info (file,
	                          G_FILE_ATTRIBUTE_STANDARD_TYPE,
	                          G_FILE_QUERY_INFO_NONE,
	                          NULL,
	                          &error);

	if (error) {
		if (error->code == G_IO_ERROR_NOT_FOUND) {
			exists = FALSE;
		}

		g_error_free (error);
	}

	if (info) {
		is_dir = g_file_info_get_file_type (info) == G_FILE_TYPE_DIRECTORY;
		g_object_unref (info);
	} else {
		/* Assume not a dir */
		is_dir = FALSE;
	}

	config = tracker_config_new ();
	path = g_file_get_path (file);

	if (exists) {
		if (is_dir) {
			print_dir_check = TRUE;
			print_dir_check_with_content = TRUE;
			print_file_check = FALSE;
			print_monitor_check = TRUE;
		} else {
			print_dir_check = FALSE;
			print_dir_check_with_content = FALSE;
			print_file_check = TRUE;
			print_monitor_check = TRUE;
		}
	} else {
		print_dir_check = TRUE;
		print_dir_check_with_content = FALSE;
		print_file_check = TRUE;
		print_monitor_check = TRUE;
	}

	g_print (exists ?
	         _("Data object “%s” currently exists") :
	         _("Data object “%s” currently does not exist"),
	         path);

	g_print ("\n");

	if (print_dir_check) {
		gboolean check;

		check = tracker_miner_files_check_directory (file,
		                                             tracker_config_get_index_recursive_directories (config),
			                                     tracker_config_get_index_single_directories (config),
			                                     tracker_config_get_ignored_directory_paths (config),
			                                     tracker_config_get_ignored_directory_patterns (config));
		g_print ("  %s\n",
		         check ?
		         _("Directory is eligible to be mined (based on rules)") :
		         _("Directory is NOT eligible to be mined (based on rules)"));

		would_index &= check;
	}

	if (print_dir_check_with_content) {
		GList *children;
		gboolean check;

		children = get_dir_children_as_gfiles (path);

		check = tracker_miner_files_check_directory_contents (file,
			                                              children,
			                                              tracker_config_get_ignored_directories_with_content (config));

		g_list_foreach (children, (GFunc) g_object_unref, NULL);
		g_list_free (children);

		g_print ("  %s\n",
		         check ?
		         _("Directory is eligible to be mined (based on contents)") :
		         _("Directory is NOT eligible to be mined (based on contents)"));

		would_index &= check;
	}

	if (print_monitor_check) {
		gboolean check = TRUE;

		check &= tracker_config_get_enable_monitors (config);

		if (check) {
			GSList *dirs_to_check, *l;
			gboolean is_covered_single;
			gboolean is_covered_recursive;

			is_covered_single = FALSE;
			dirs_to_check = tracker_config_get_index_single_directories (config);

			for (l = dirs_to_check; l && !is_covered_single; l = l->next) {
				GFile *dir;
				GFile *parent;

				parent = g_file_get_parent (file);
				dir = g_file_new_for_path (l->data);
				is_covered_single = g_file_equal (parent, dir) || g_file_equal (file, dir);

				g_object_unref (dir);
				g_object_unref (parent);
			}

			is_covered_recursive = FALSE;
			dirs_to_check = tracker_config_get_index_recursive_directories (config);

			for (l = dirs_to_check; l && !is_covered_recursive; l = l->next) {
				GFile *dir;

				dir = g_file_new_for_path (l->data);
				is_covered_recursive = g_file_has_prefix (file, dir) || g_file_equal (file, dir);
				g_object_unref (dir);
			}

			check &= is_covered_single || is_covered_recursive;
		}

		if (exists && is_dir) {
			g_print ("  %s\n",
			         check ?
			         _("Directory is eligible to be monitored (based on config)") :
			         _("Directory is NOT eligible to be monitored (based on config)"));
		} else if (exists && !is_dir) {
			g_print ("  %s\n",
			         check ?
			         _("File is eligible to be monitored (based on config)") :
			         _("File is NOT eligible to be monitored (based on config)"));
		} else {
			g_print ("  %s\n",
			         check ?
			         _("File or Directory is eligible to be monitored (based on config)") :
			         _("File or Directory is NOT eligible to be monitored (based on config)"));
		}

		would_notice &= check;
	}

	if (print_file_check) {
		gboolean check;

		check = tracker_miner_files_check_file (file,
		                                        tracker_config_get_ignored_file_paths (config),
		                                        tracker_config_get_ignored_file_patterns (config));

		g_print ("  %s\n",
		         check ?
		         _("File is eligible to be mined (based on rules)") :
		         _("File is NOT eligible to be mined (based on rules)"));

		would_index &= check;
	}

	g_print ("\n"
	         "%s: %s\n"
	         "%s: %s\n"
	         "\n",
	         _("Would be indexed"),
	         would_index ? _("Yes") : _("No"),
	         _("Would be monitored"),
	         would_notice ? _("Yes") : _("No"));

	if (log_handler_id != 0) {
		/* Unset log handler */
		g_log_remove_handler (NULL, log_handler_id);
	}

	g_free (path);
	g_object_unref (config);
	g_object_unref (file);
}

static gboolean
miner_needs_check (TrackerMiner *miner)
{
	/* Reasons to not mark ourselves as cleanly shutdown include:
	 *
	 * 1. Still crawling or with files to process in our queues.
	 * 2. We crash (out of our control usually anyway).
	 * 3. At least one of the miners is PAUSED, we have
	 *    to exclude the situations where the miner is actually done.
	 */
	if (!tracker_miner_is_paused (miner)) {
		if (tracker_miner_fs_has_items_to_process (TRACKER_MINER_FS (miner))) {
			/* There are items left to process */
			return TRUE;
		}

		/* FIXME: We currently don't check the applications
		 *  miner if we are finished before returning TRUE/FALSE here, should
		 *  we?
		 */

		/* We consider the miner finished */
		return FALSE;
	} else {
		/* Paused for other reasons, so probably not done */
		return TRUE;
	}
}

static void
on_domain_vanished (GDBusConnection *connection,
                    const gchar     *name,
                    gpointer         user_data)
{
	GMainLoop *loop = user_data;
	g_main_loop_quit (loop);
}

TrackerSparqlConnectionFlags
get_fts_connection_flags (void)
{
	TrackerSparqlConnectionFlags flags = 0;
	TrackerFTSConfig *fts_config;

	fts_config = tracker_fts_config_new ();

	if (tracker_fts_config_get_enable_stemmer (fts_config))
		flags |= TRACKER_SPARQL_CONNECTION_FLAGS_FTS_ENABLE_STEMMER;
	if (tracker_fts_config_get_enable_unaccent (fts_config))
		flags |= TRACKER_SPARQL_CONNECTION_FLAGS_FTS_ENABLE_UNACCENT;
	if (tracker_fts_config_get_ignore_numbers (fts_config))
		flags |= TRACKER_SPARQL_CONNECTION_FLAGS_FTS_IGNORE_NUMBERS;
	if (tracker_fts_config_get_ignore_stop_words (fts_config))
		flags |= TRACKER_SPARQL_CONNECTION_FLAGS_FTS_ENABLE_STOP_WORDS;

	g_object_unref (fts_config);

	return flags;
}

static gboolean
setup_connection_and_endpoint (TrackerDomainOntology    *domain,
                               GDBusConnection          *connection,
                               TrackerSparqlConnection **sparql_conn,
                               TrackerEndpointDBus     **endpoint,
                               GError                  **error)
{
	GFile *store;
	GFile *ontology;

	store = tracker_domain_ontology_get_cache (domain);
	ontology = tracker_domain_ontology_get_ontology (domain);
	*sparql_conn = tracker_sparql_connection_new (get_fts_connection_flags (),
	                                              store,
	                                              ontology,
	                                              NULL,
	                                              error);
	if (!*sparql_conn)
		return FALSE;

	*endpoint = tracker_endpoint_dbus_new (*sparql_conn,
	                                       connection,
	                                       NULL,
	                                       NULL,
	                                       error);
	if (!*endpoint)
		return FALSE;

	return TRUE;
}

int
main (gint argc, gchar *argv[])
{
	TrackerConfig *config;
	TrackerMiner *miner_files;
	TrackerMinerFilesIndex *miner_files_index;
	GOptionContext *context;
	GError *error = NULL;
	gchar *log_filename = NULL;
	gboolean do_mtime_checking;
	gboolean force_mtime_checking = FALSE;
	TrackerMinerProxy *proxy;
	GDBusConnection *connection;
	TrackerSparqlConnection *sparql_conn;
	TrackerEndpointDBus *endpoint;
	TrackerDomainOntology *domain_ontology;
	gchar *domain_name, *dbus_name;

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
	context = g_option_context_new (_("— start the tracker indexer"));

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

	if (eligible) {
		check_eligible ();
		return EXIT_SUCCESS;
	}

	domain_ontology = tracker_domain_ontology_new (domain_ontology_name, NULL, &error);
	if (error) {
		g_critical ("Could not load domain ontology '%s': %s",
		            domain_ontology_name, error->message);
		g_error_free (error);
		return EXIT_FAILURE;
	}

	/* This makes sure we don't steal all the system's resources */
	initialize_priority_and_scheduling ();

	connection = g_bus_get_sync (TRACKER_IPC_BUS, NULL, &error);
	if (error) {
		g_critical ("Could not create DBus connection: %s\n",
		            error->message);
		g_error_free (error);
		return EXIT_FAILURE;
	}

	/* Initialize logging */
	config = tracker_config_new ();

	if (verbosity > -1) {
		tracker_config_set_verbosity (config, verbosity);
	}

	if (initial_sleep > -1) {
		tracker_config_set_initial_sleep (config, initial_sleep);
	}

	tracker_log_init (tracker_config_get_verbosity (config),
	                  &log_filename);
	if (log_filename) {
		g_message ("Using log file:'%s'", log_filename);
		g_free (log_filename);
	}

	sanity_check_option_values (config);

	main_loop = g_main_loop_new (NULL, FALSE);

	if (domain_ontology && domain_ontology_name) {
		/* If we are running for a specific domain, we tie the lifetime of this
		 * process to the domain. For example, if the domain name is
		 * org.example.MyApp then this tracker-miner-fs process will exit as
		 * soon as org.example.MyApp exits.
		 */
		domain_name = tracker_domain_ontology_get_domain (domain_ontology, NULL);
		g_bus_watch_name_on_connection (connection, domain_name,
		                                G_BUS_NAME_WATCHER_FLAGS_NONE,
		                                NULL, on_domain_vanished,
		                                main_loop, NULL);
		g_free (domain_name);
	}

	g_message ("Checking if we're running as a daemon:");
	g_message ("  %s %s",
	           no_daemon ? "No" : "Yes",
	           no_daemon ? "(forced by command line)" : "");

	if (!setup_connection_and_endpoint (domain_ontology,
	                                    connection,
	                                    &sparql_conn,
	                                    &endpoint,
	                                    &error)) {

		g_critical ("Could not create store/endpoint: %s",
		            error->message);
		g_error_free (error);

		return EXIT_FAILURE;
	}

	/* Create new TrackerMinerFiles object */
	miner_files = tracker_miner_files_new (sparql_conn, config,
	                                       domain_ontology_name, &error);
	if (!miner_files) {
		g_critical ("Couldn't create new Files miner: '%s'",
		            error ? error->message : "unknown error");
		g_object_unref (config);
		tracker_log_shutdown ();
		return EXIT_FAILURE;
	}

	/* If the locales changed, we need to reset some things first */
	detect_locale_changed (TRACKER_MINER (miner_files));

	proxy = tracker_miner_proxy_new (miner_files, connection, DBUS_PATH, NULL, &error);
	if (error) {
		g_critical ("Couldn't create miner proxy: %s", error->message);
		g_error_free (error);
		g_object_unref (config);
		g_object_unref (miner_files);
		tracker_log_shutdown ();
		return EXIT_FAILURE;
	}

	tracker_writeback_init (TRACKER_MINER_FILES (miner_files),
	                        config,
	                        &error);

	if (error) {
		g_critical ("Couldn't create writeback handling: '%s'",
		            error ? error->message : "unknown error");
		g_object_unref (config);
		g_object_unref (miner_files);
		tracker_log_shutdown ();
		return EXIT_FAILURE;
	}

	/* Create new TrackerMinerFilesIndex object */
	miner_files_index = tracker_miner_files_index_new (TRACKER_MINER_FILES (miner_files));
	if (!miner_files_index) {
		g_object_unref (miner_files);
		tracker_writeback_shutdown ();
		g_object_unref (config);
		tracker_log_shutdown ();
		return EXIT_FAILURE;
	}

	/* Request DBus name */
	dbus_name = tracker_domain_ontology_get_domain (domain_ontology, DBUS_NAME_SUFFIX);

	if (!tracker_dbus_request_name (connection, dbus_name, &error)) {
		g_critical ("Could not request DBus name '%s': %s",
		            dbus_name, error->message);
		g_error_free (error);
		g_free (dbus_name);
		return EXIT_FAILURE;
	}

	g_free (dbus_name);

	/* Check if we should crawl and if we should force mtime
	 * checking based on the config.
	 */
	do_crawling = should_crawl (config, &force_mtime_checking);

	/* Get the last shutdown state to see if we need to perform a
	 * full mtime check against the db or not.
	 *
	 * Set to TRUE here in case we crash and miss file system
	 * events.
	 */
	g_message ("Checking whether to force mtime checking during crawling (based on last clean shutdown):");

	/* Override the shutdown state decision based on the config */
	if (force_mtime_checking) {
		do_mtime_checking = TRUE;
	} else {
		do_mtime_checking = tracker_miner_files_get_need_mtime_check ();
	}

	g_message ("  %s %s",
	           do_mtime_checking ? "Yes" : "No",
	           force_mtime_checking ? "(forced from config)" : "");

	/* Set the need for an mtime check to TRUE so we check in the
	 * event of a crash, this is changed back on shutdown if
	 * everything appears to be fine.
	 */
	tracker_miner_files_set_need_mtime_check (TRUE);

	/* Configure files miner */
	tracker_miner_files_set_mtime_checking (TRACKER_MINER_FILES (miner_files), do_mtime_checking);
	g_signal_connect (miner_files, "finished",
			  G_CALLBACK (miner_finished_cb),
			  NULL);

	if (do_crawling)
		miner_start (miner_files, config, do_mtime_checking);

	initialize_signal_handler ();

	/* Go, go, go! */
	g_main_loop_run (main_loop);

	g_message ("Shutdown started");

	if (miners_timeout_id == 0 && !miner_needs_check (miner_files)) {
		tracker_miner_files_set_need_mtime_check (FALSE);
		save_current_locale ();
	}

	g_main_loop_unref (main_loop);
	g_object_unref (config);
	g_object_unref (miner_files_index);

	g_object_unref (miner_files);

	g_object_unref (proxy);
	g_object_unref (connection);
	tracker_domain_ontology_unref (domain_ontology);

	tracker_writeback_shutdown ();
	tracker_log_shutdown ();

	g_print ("\nOK\n\n");

	return EXIT_SUCCESS;
}
