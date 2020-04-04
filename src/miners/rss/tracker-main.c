/*
 * Copyright (C) 2009/2010, Roberto Guido <madbob@users.barberaware.org>
 *                          Michele Tameni <michele@amdplanet.it>
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

#include <libtracker-miners-common/tracker-common.h>

#include "tracker-miner-rss.h"

#define DBUS_NAME_SUFFIX "Tracker1.Miner.RSS"
#define DBUS_PATH "/org/freedesktop/Tracker1/Miner/RSS"

static gint verbosity = -1;
static gchar *add_feed;
static gchar *title;
static gchar *domain_ontology_name = NULL;

static GOptionEntry entries[] = {
	{ "verbosity", 'v', 0,
	  G_OPTION_ARG_INT, &verbosity,
	  N_("Logging, 0 = errors only, "
	  "1 = minimal, 2 = detailed and 3 = debug (default=0)"),
	  NULL },
	{ "add-feed", 'a', 0,
	  G_OPTION_ARG_STRING, &add_feed,
	  /* Translators: this is a "feed" as in RSS */
	  N_("Add feed"),
	  N_("URL") },
	{ "title", 't', 0,
	  G_OPTION_ARG_STRING, &title,
	  N_("Title to use (must be used with --add-feed)"),
	  NULL },
	{ "domain-ontology", 'd', 0,
	  G_OPTION_ARG_STRING, &domain_ontology_name,
	  N_("Runs for a specific domain ontology"),
	  NULL },
	{ NULL }
};

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

	store = tracker_domain_ontology_get_cache (domain);
	*sparql_conn = tracker_sparql_connection_new (get_fts_connection_flags (),
	                                              store,
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
main (int argc, char **argv)
{
	gchar *log_filename;
	GMainLoop *loop;
	GOptionContext *context;
	TrackerMinerRSS *miner;
	GError *error = NULL;
	GDBusConnection *connection;
	TrackerSparqlConnection *sparql_conn;
	TrackerEndpointDBus *endpoint;
	TrackerMinerProxy *proxy;
	TrackerDomainOntology *domain_ontology;
	gchar *domain_name, *dbus_name;

	setlocale (LC_ALL, "");

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
	tzset ();

	/* Translators: this messagge will apper immediately after the
	 * usage string - Usage: COMMAND <THIS_MESSAGE>
	 */
	context = g_option_context_new (_("— start the feeds indexer"));
	g_option_context_add_main_entries (context, entries, NULL);
	g_option_context_parse (context, &argc, &argv, NULL);

	if (title && !add_feed) {
		gchar *help;

		help = g_option_context_get_help (context, TRUE, NULL);
		g_option_context_free (context);
		g_printerr ("%s", help);
		g_free (help);

		return EXIT_FAILURE;
	}

	g_option_context_free (context);

	/* Command line stuff doesn't use logging, so we're using g_print*() */
	if (add_feed) {
		TrackerSparqlConnection *connection;
		GString *query;

		g_print ("Adding feed:\n"
		         "  title:'%s'\n"
		         "  url:'%s'\n",
		         title,
		         add_feed);

		connection = tracker_sparql_connection_bus_new ("org.freedesktop.Tracker1.Miner.RSS",
		                                                NULL, NULL, &error);

		if (!connection) {
			g_printerr ("%s: %s\n",
			            _("Could not establish a connection to Tracker"),
			            error ? error->message : _("No error given"));
			g_clear_error (&error);
			return EXIT_FAILURE;
		}

		/* FIXME: Make interval configurable */
		query = g_string_new ("INSERT {"
		                      "  _:FeedSettings a mfo:FeedSettings ;"
		                      "                   mfo:updateInterval 20 ."
		                      "  _:Feed a nie:DataObject, mfo:FeedChannel ;"
		                      "           mfo:feedSettings _:FeedSettings ;");

		if (title) {
			g_string_append_printf (query, "nie:title \"%s\";", title);
		}

		g_string_append_printf (query, " nie:url \"%s\" }", add_feed);

		tracker_sparql_connection_update (connection,
		                                  query->str,
		                                  G_PRIORITY_DEFAULT,
		                                  NULL,
		                                  &error);
		g_string_free (query, TRUE);

		if (error) {
			g_printerr ("%s, %s\n",
			            _("Could not add feed"),
			            error->message);
			g_error_free (error);
			g_object_unref (connection);

			return EXIT_FAILURE;
		}

		g_print ("Done\n");

		return EXIT_SUCCESS;
	}

	tracker_log_init (verbosity, &log_filename);
	if (log_filename != NULL) {
		g_message ("Using log file:'%s'", log_filename);
		g_free (log_filename);
	}

	domain_ontology = tracker_domain_ontology_new (domain_ontology_name, NULL, &error);
	if (error) {
		g_critical ("Could not load domain ontology '%s': %s",
		            domain_ontology_name, error->message);
		g_error_free (error);
		return EXIT_FAILURE;
	}

	connection = g_bus_get_sync (TRACKER_IPC_BUS, NULL, &error);
	if (error) {
		g_critical ("Could not create DBus connection: %s\n",
		            error->message);
		g_error_free (error);
		return EXIT_FAILURE;
	}

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

	miner = tracker_miner_rss_new (sparql_conn, &error);
	if (!miner) {
		g_critical ("Could not create new RSS miner: '%s', exiting...\n",
		            error ? error->message : "unknown error");
		return EXIT_FAILURE;
	}

	tracker_miner_start (TRACKER_MINER (miner));
	proxy = tracker_miner_proxy_new (TRACKER_MINER (miner), connection, DBUS_PATH, NULL, &error);
	if (error) {
		g_critical ("Could not create miner DBus proxy: %s\n", error->message);
		g_error_free (error);
		return EXIT_FAILURE;
	}

	dbus_name = tracker_domain_ontology_get_domain (domain_ontology, DBUS_NAME_SUFFIX);

	if (!tracker_dbus_request_name (connection, dbus_name, &error)) {
		g_critical ("Could not request DBus name '%s': %s",
		            dbus_name, error->message);
		g_error_free (error);
		g_free (dbus_name);
		return EXIT_FAILURE;
	}

	g_free (dbus_name);

	loop = g_main_loop_new (NULL, FALSE);

	if (domain_ontology && domain_ontology_name) {
		/* If we are running for a specific domain, we tie the lifetime of this
		 * process to the domain. For example, if the domain name is
		 * org.example.MyApp then this tracker-miner-rss process will exit as
		 * soon as org.example.MyApp exits.
		 */
		domain_name = tracker_domain_ontology_get_domain (domain_ontology, NULL);
		g_bus_watch_name_on_connection (connection, domain_name,
		                                G_BUS_NAME_WATCHER_FLAGS_NONE,
		                                NULL, on_domain_vanished,
		                                loop, NULL);
		g_free (domain_name);
	}

	g_main_loop_run (loop);

	tracker_log_shutdown ();
	g_main_loop_unref (loop);
	g_object_unref (sparql_conn);
	g_object_unref (endpoint);
	g_object_unref (miner);
	g_object_unref (connection);
	g_object_unref (proxy);
	tracker_domain_ontology_unref (domain_ontology);

	return EXIT_SUCCESS;
}
