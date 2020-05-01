/*
 * Copyright (C) 2010, Nokia <ivan.frade@nokia.com>
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

#include <string.h>
#include <unistd.h>

#include <glib.h>
#include <glib/gstdio.h>

/* Special case, the monitor header is not normally exported */
#include <libtracker-miner/tracker-monitor.h>

/* -------------- COMMON FOR ALL FILE EVENT TESTS ----------------- */

#define TEST_TIMEOUT 5 /* seconds */

typedef enum {
	MONITOR_SIGNAL_NONE = 0,
	/* Short names used for code readability. */
	CREATED           = 1 << 0,
	UPDATED           = 1 << 1,
	ATTRIBUTE_UPDATED = 1 << 2,
	DELETED           = 1 << 3,
	MOVED_FROM        = 1 << 4,
	MOVED_TO          = 1 << 5
} MonitorSignal;

/* Fixture object type */
typedef struct {
	TrackerMonitor *monitor;
	GFile *monitored_directory_file;
	gchar *monitored_directory;
	gchar *not_monitored_directory;
	GHashTable *require_events;
	GHashTable *prohibit_events;
	GHashTable *events;
	GMainLoop *main_loop;
} TrackerMonitorTestFixture;

/* These two functions set up expections for the test. */

static void
require_events (TrackerMonitorTestFixture *fixture,
                GFile                     *file,
                MonitorSignal              event_mask)
{
	gboolean created;

	g_assert (G_IS_FILE (file));

	created = g_hash_table_insert (fixture->require_events, g_object_ref (file), GUINT_TO_POINTER (event_mask));

	if (!created) {
		g_error ("require_events() called twice for %s", g_file_peek_path (file));
	}
}

static void
prohibit_events (TrackerMonitorTestFixture *fixture,
                 GFile                     *file,
                 MonitorSignal              event_mask)
{
	gboolean created;

	g_assert (G_IS_FILE (file));

	created = g_hash_table_insert (fixture->prohibit_events, g_object_ref (file), GUINT_TO_POINTER (event_mask));

	if (!created) {
		g_error ("prohibit_events() called twice for %s", g_file_peek_path (file));
	}
}

static void
error_prohibited_events (GFile         *file,
                         MonitorSignal  events,
                         MonitorSignal  prohibit_event_mask)
{
	gchar *uri;
	uri = g_file_get_uri (file);

	g_message ("Prohibited event received for '%s': \n"
	           "   CREATED:           %s %s\n"
	           "   UPDATED:           %s %s\n"
	           "   ATTRIBUTE UPDATED: %s %s\n"
	           "   DELETED:           %s %s\n"
	           "   MOVED_FROM:        %s %s\n"
	           "   MOVED_TO:          %s %s\n",
	           uri,
	           events & CREATED ? "yes" : "no",
	           prohibit_event_mask & CREATED ? "(prohibited)" : "",
	           events & UPDATED ? "yes" : "no",
	           prohibit_event_mask & UPDATED ? "(prohibited)" : "",
	           events & ATTRIBUTE_UPDATED ? "yes" : "no",
	           prohibit_event_mask & ATTRIBUTE_UPDATED ? "(prohibited)" : "",
	           events & DELETED ? "yes" : "no",
	           prohibit_event_mask & DELETED ? "(prohibited)" : "",
	           events & MOVED_FROM ? "yes" : "no",
	           prohibit_event_mask & MOVED_FROM ? "(prohibited)" : "",
	           events & MOVED_TO ? "yes" : "no",
	           prohibit_event_mask & MOVED_TO ? "(prohibited)" : "");
}

static void
error_required_events (GFile         *file,
                       MonitorSignal  events,
                       MonitorSignal  require_event_mask)
{
	gchar *uri;
	uri = g_file_get_uri (file);

	g_message ("Required event was not received for '%s': \n"
	           "   CREATED:           %s %s\n"
	           "   UPDATED:           %s %s\n"
	           "   ATTRIBUTE UPDATED: %s %s\n"
	           "   DELETED:           %s %s\n"
	           "   MOVED_FROM:        %s %s\n"
	           "   MOVED_TO:          %s %s\n",
	           uri,
	           events & CREATED ? "yes" : "no",
	           require_event_mask & CREATED ? "(required)" : "",
	           events & UPDATED ? "yes" : "no",
	           require_event_mask & UPDATED ? "(required)" : "",
	           events & ATTRIBUTE_UPDATED ? "yes" : "no",
	           require_event_mask & ATTRIBUTE_UPDATED ? "(required)" : "",
	           events & DELETED ? "yes" : "no",
	           require_event_mask & DELETED ? "(required)" : "",
	           events & MOVED_FROM ? "yes" : "no",
	           require_event_mask & MOVED_FROM ? "(required)" : "",
	           events & MOVED_TO ? "yes" : "no",
	           require_event_mask & MOVED_TO ? "(required)" : "");
}


/* This function checks the received events against the 'require' and
 * 'prohibit' events set by the test.
 *
 * If a prohibited event occured, the test will abort with g_error().
 *
 * If 'test_running' is FALSE and not all required events occurred,
 * the test will abort with g_error().
 *
 * if 'test_running' is TRUE and all required events occured,
 * g_main_loop_quit() is called.
 */
static void
check_events (TrackerMonitorTestFixture *fixture,
              gboolean                   test_running)
{
	GFile *file = NULL;
	GList *keys, *l;
	MonitorSignal require_event_mask, prohibit_event_mask, events;
	GList *files_missing_required_events = NULL;
	GList *files_with_prohibited_events = NULL;

	keys = g_hash_table_get_keys (fixture->require_events);
	for (l = keys; l; l = l->next) {
		file = G_FILE (l->data);
		require_event_mask = GPOINTER_TO_UINT (g_hash_table_lookup (fixture->require_events, file));
		events = GPOINTER_TO_UINT (g_hash_table_lookup (fixture->events, file));
		if ((events & require_event_mask) != require_event_mask) {
			files_missing_required_events = g_list_prepend (files_missing_required_events, file);
		}
	}
	g_list_free (keys);

	keys = g_hash_table_get_keys (fixture->prohibit_events);
	for (l = keys; l; l = l->next) {
		file = G_FILE (l->data);
		prohibit_event_mask = GPOINTER_TO_UINT (g_hash_table_lookup (fixture->prohibit_events, file));
		events = GPOINTER_TO_UINT (g_hash_table_lookup (fixture->events, file));
		if (events & prohibit_event_mask) {
			files_with_prohibited_events = g_list_prepend (files_with_prohibited_events, file);
		}
	}
	g_list_free (keys);

	if (files_with_prohibited_events != NULL) {
		file = G_FILE (files_with_prohibited_events->data);

		events = GPOINTER_TO_UINT (g_hash_table_lookup (fixture->events, file));
		prohibit_event_mask = GPOINTER_TO_UINT (g_hash_table_lookup (fixture->prohibit_events, file));
		error_prohibited_events (file, events, prohibit_event_mask);

		g_test_fail ();
		g_main_loop_quit (fixture->main_loop);
	}

	if (files_missing_required_events) {
		if (test_running) {
			return;  /* Test should continue */
		} else {
			file = G_FILE (files_missing_required_events->data);

			events = GPOINTER_TO_UINT (g_hash_table_lookup (fixture->events, file));
			require_event_mask = GPOINTER_TO_UINT (g_hash_table_lookup (fixture->require_events, file));
			error_required_events (file, events, require_event_mask);

			g_test_fail ();
			g_main_loop_quit (fixture->main_loop);
		}
	}

	if (test_running) {
		g_main_loop_quit (fixture->main_loop);
	}
}

static void
add_event (GHashTable    *events,
           GFile         *file,
           MonitorSignal  new_event)
{
	gpointer previous_file;
	gpointer previous_mask;

	/* Lookup file in HT */
	if (g_hash_table_lookup_extended (events,
	                                  file,
	                                  &previous_file,
	                                  &previous_mask)) {
		guint mask;

		mask = GPOINTER_TO_UINT (previous_mask);
		mask |= new_event;
		g_hash_table_replace (events,
		                      g_object_ref (previous_file),
		                      GUINT_TO_POINTER (mask));
	}
}

static void
test_monitor_events_created_cb (TrackerMonitor *monitor,
                                GFile          *file,
                                gboolean        is_directory,
                                gpointer        user_data)
{
	gchar *path;
	TrackerMonitorTestFixture *fixture = user_data;

	g_assert (file != NULL);
	path = g_file_get_path (file);
	g_assert (path != NULL);

	g_debug ("***** '%s' (%s) (CREATED)",
	         path,
	         is_directory ? "DIR" : "FILE");

	g_free (path);

	add_event ((GHashTable *) fixture->events,
	           file,
	           CREATED);
	check_events (fixture, TRUE);
}

static void
test_monitor_events_updated_cb (TrackerMonitor *monitor,
                                GFile          *file,
                                gboolean        is_directory,
                                gpointer        user_data)
{
	TrackerMonitorTestFixture *fixture = user_data;
	gchar *path;

	g_assert (file != NULL);
	path = g_file_get_path (file);
	g_assert (path != NULL);

	g_debug ("***** '%s' (%s) (UPDATED)",
	         path,
	         is_directory ? "DIR" : "FILE");

	g_free (path);

	add_event ((GHashTable *) fixture->events,
	           file,
	           UPDATED);
	check_events (fixture, TRUE);
}

static void
test_monitor_events_attribute_updated_cb (TrackerMonitor *monitor,
                                          GFile          *file,
                                          gboolean        is_directory,
                                          gpointer        user_data)
{
	TrackerMonitorTestFixture *fixture = user_data;
	gchar *path;

	g_assert (file != NULL);
	path = g_file_get_path (file);
	g_assert (path != NULL);

	g_debug ("***** '%s' (%s) (ATRIBUTE UPDATED)",
	         path,
	         is_directory ? "DIR" : "FILE");

	g_free (path);

	add_event ((GHashTable *) fixture->events,
	           file,
	           ATTRIBUTE_UPDATED);
	check_events (fixture, TRUE);
}

static void
test_monitor_events_deleted_cb (TrackerMonitor *monitor,
                                GFile          *file,
                                gboolean        is_directory,
                                gpointer        user_data)
{
	TrackerMonitorTestFixture *fixture = user_data;
	gchar *path;

	g_assert (file != NULL);
	path = g_file_get_path (file);
	g_assert (path != NULL);

	g_debug ("***** '%s' (%s) (DELETED)",
	         path,
	         is_directory ? "DIR" : "FILE");

	g_free (path);

	add_event ((GHashTable *) fixture->events,
	           file,
	           DELETED);
	check_events (fixture, TRUE);
}

static void
test_monitor_events_moved_cb (TrackerMonitor *monitor,
                              GFile          *file,
                              GFile          *other_file,
                              gboolean        is_directory,
                              gboolean        is_source_monitored,
                              gpointer        user_data)
{
	TrackerMonitorTestFixture *fixture = user_data;
	gchar *path;
	gchar *other_path;

	g_assert (file != NULL);
	path = g_file_get_path (other_file);
	other_path = g_file_get_path (other_file);

	g_debug ("***** '%s'->'%s' (%s) (MOVED) (source %smonitored)",
	         path,
	         other_path,
	         is_directory ? "DIR" : "FILE",
	         is_source_monitored ? "" : "not ");

	g_free (other_path);
	g_free (path);

	/* Add event to the files */
	add_event ((GHashTable *) fixture->events,
	           file,
	           MOVED_FROM);
	add_event ((GHashTable *) fixture->events,
	           other_file,
	           MOVED_TO);
	check_events (fixture, TRUE);
}

static void
test_monitor_common_setup (TrackerMonitorTestFixture *fixture,
                           gconstpointer              data)
{
	gchar *basename;

	/* Create hash tables to store expected results */
	fixture->require_events = g_hash_table_new_full (g_file_hash,
	                                                 (GEqualFunc) g_file_equal,
	                                                 (GDestroyNotify) g_object_unref,
	                                                 NULL);
	fixture->prohibit_events = g_hash_table_new_full (g_file_hash,
	                                                  (GEqualFunc) g_file_equal,
	                                                  (GDestroyNotify) g_object_unref,
	                                                  NULL);

	/* Create hash table to store received events */
	fixture->events = g_hash_table_new_full (g_file_hash,
	                                         (GEqualFunc) g_file_equal,
	                                         NULL,
	                                         NULL);

	/* Create and setup the tracker monitor */
	fixture->monitor = tracker_monitor_new ();
	g_assert (fixture->monitor != NULL);

	g_signal_connect (fixture->monitor, "item-created",
	                  G_CALLBACK (test_monitor_events_created_cb),
	                  fixture);
	g_signal_connect (fixture->monitor, "item-updated",
	                  G_CALLBACK (test_monitor_events_updated_cb),
	                  fixture);
	g_signal_connect (fixture->monitor, "item-attribute-updated",
	                  G_CALLBACK (test_monitor_events_attribute_updated_cb),
	                  fixture);
	g_signal_connect (fixture->monitor, "item-deleted",
	                  G_CALLBACK (test_monitor_events_deleted_cb),
	                  fixture);
	g_signal_connect (fixture->monitor, "item-moved",
	                  G_CALLBACK (test_monitor_events_moved_cb),
	                  fixture);

	/* Initially, set it disabled */
	tracker_monitor_set_enabled (fixture->monitor, FALSE);

	/* Create a temp directory to monitor in the test */
	basename = g_dir_make_tmp ("tracker-monitor-test-XXXXXX", NULL);

	fixture->monitored_directory = basename;
	fixture->monitored_directory_file = g_file_new_for_path (fixture->monitored_directory);
	g_assert (fixture->monitored_directory_file != NULL);
	g_assert_cmpint (tracker_monitor_add (fixture->monitor, fixture->monitored_directory_file), ==, TRUE);
	g_assert_cmpint (tracker_monitor_get_count (fixture->monitor), ==, 1);

	/* Setup also not-monitored directory */
	fixture->not_monitored_directory = g_strdup (g_get_tmp_dir ());

	/* Create new main loop */
	fixture->main_loop = g_main_loop_new (NULL, FALSE);
	g_assert (fixture->main_loop != NULL);
}

static void
test_monitor_common_teardown (TrackerMonitorTestFixture *fixture,
                              gconstpointer              data)
{
	/* Remove the main loop */
	g_main_loop_unref (fixture->main_loop);

	/* Cleanup monitor */
	g_assert_cmpint (tracker_monitor_remove (fixture->monitor, fixture->monitored_directory_file), ==, TRUE);
	g_assert_cmpint (tracker_monitor_get_count (fixture->monitor), ==, 0);

	/* Destroy monitor */
	g_assert (fixture->monitor != NULL);
	g_object_unref (fixture->monitor);

	/* Remove the hash_tables of events */
	g_hash_table_destroy (fixture->events);
	g_hash_table_destroy (fixture->require_events);
	g_hash_table_destroy (fixture->prohibit_events);

	/* Remove base test directories */
	g_assert (fixture->monitored_directory_file != NULL);
	g_assert (fixture->monitored_directory != NULL);
	g_assert_cmpint (g_file_delete (fixture->monitored_directory_file, NULL, NULL), ==, TRUE);
	g_object_unref (fixture->monitored_directory_file);
	g_free (fixture->monitored_directory);

	g_assert (fixture->not_monitored_directory != NULL);
	g_free (fixture->not_monitored_directory);
}

static void
create_directory (const gchar  *parent,
                  const gchar  *directory_name,
                  GFile       **outfile)
{
	GFile *dirfile;
	gchar *path;

	path = g_build_path (G_DIR_SEPARATOR_S, parent, directory_name, NULL);
	dirfile = g_file_new_for_path (path);
	g_assert (dirfile != NULL);
	g_assert_cmpint (g_file_make_directory_with_parents (dirfile, NULL, NULL), ==, TRUE);
	if (outfile) {
		*outfile = dirfile;
	} else {
		g_object_unref (dirfile);
	}
	g_free (path);
}

static void
set_file_contents (const gchar  *directory,
                   const gchar  *filename,
                   const gchar  *contents,
                   GFile       **outfile)
{
	FILE *file;
	size_t length;
	gchar *file_path;

	g_assert (directory != NULL);
	g_assert (filename != NULL);
	g_assert (contents != NULL);

	file_path = g_build_filename (directory, filename, NULL);

	file = g_fopen (file_path, "wb");
	g_assert (file != NULL);
	length = strlen (contents);
	g_assert_cmpint (fwrite (contents, 1, length, file), >=, length);
	g_assert_cmpint (fflush (file), ==, 0);
	g_assert_cmpint (fclose (file), !=, EOF);

	if (outfile) {
		*outfile = g_file_new_for_path (file_path);
	}
	g_free (file_path);
}

static void
set_file_permissions (const gchar  *directory,
                      const gchar  *filename,
                      gint          permissions)
{
	gchar *file_path;

	g_assert (directory != NULL);
	g_assert (filename != NULL);

	file_path = g_build_filename (directory, filename, NULL);
	g_assert_cmpint (g_chmod (file_path, permissions), ==, 0);
	g_free (file_path);
}

static gboolean
timeout_cb (gpointer data)
{
	g_main_loop_quit ((GMainLoop *) data);
	return TRUE;
}

static void
events_wait (TrackerMonitorTestFixture *fixture)
{
	gint timeout_id;

	/* Setup timeout to stop the main loop after some seconds */
	timeout_id = g_timeout_add_seconds (TEST_TIMEOUT, timeout_cb, fixture->main_loop);
	g_debug ("Waiting up to %u seconds for monitor events...", TEST_TIMEOUT);
	g_main_loop_run (fixture->main_loop);

	/* Assert that we got only the required events. */
	if (! g_test_failed ()) {
		check_events (fixture, FALSE);
	}

	g_source_remove (timeout_id);
}

/* ----------------------------- FILE EVENT TESTS --------------------------------- */

static void
test_monitor_file_event_created (TrackerMonitorTestFixture *fixture,
                                 gconstpointer              data)
{
	GFile *test_file;

	/* Set up environment */
	tracker_monitor_set_enabled (fixture->monitor, TRUE);

	/* Create file to test with */
	set_file_contents (fixture->monitored_directory, "created.txt", "foo", &test_file);
	g_assert (test_file != NULL);
	g_hash_table_insert (fixture->events,
	                     g_object_ref (test_file),
	                     GUINT_TO_POINTER (MONITOR_SIGNAL_NONE));

	/* Run test */
	require_events (fixture, test_file, CREATED);
	prohibit_events (fixture, test_file, MOVED_FROM | MOVED_TO | DELETED | UPDATED);
	events_wait (fixture);

	/* Cleanup environment */
	tracker_monitor_set_enabled (fixture->monitor, FALSE);

	/* Remove the test file */
	g_assert_cmpint (g_file_delete (test_file, NULL, NULL), ==, TRUE);
	g_object_unref (test_file);
}

static void
test_monitor_file_event_updated (TrackerMonitorTestFixture *fixture,
                                 gconstpointer              data)
{
	GFile *test_file;

	/* Create file to test with, before setting up environment */
	set_file_contents (fixture->monitored_directory, "created.txt", "foo", NULL);

	/* Set up environment */
	tracker_monitor_set_enabled (fixture->monitor, TRUE);

	/* Now, trigger update of the already created file */
	set_file_contents (fixture->monitored_directory, "created.txt", "barrrr", &test_file);
	g_assert (test_file != NULL);
	g_hash_table_insert (fixture->events,
	                     g_object_ref (test_file),
	                     GUINT_TO_POINTER (MONITOR_SIGNAL_NONE));

	/* Wait for events */
	require_events (fixture, test_file, UPDATED);
	prohibit_events (fixture, test_file, CREATED | MOVED_FROM | MOVED_TO | DELETED);
	events_wait (fixture);

	/* Cleanup environment */
	tracker_monitor_set_enabled (fixture->monitor, FALSE);

	/* Remove the test file */
	g_assert_cmpint (g_file_delete (test_file, NULL, NULL), ==, TRUE);
	g_object_unref (test_file);
}

static void
test_monitor_file_event_attribute_updated (TrackerMonitorTestFixture *fixture,
                                           gconstpointer              data)
{
	GFile *test_file;

	/* Create file to test with, before setting up environment */
	set_file_contents (fixture->monitored_directory, "created.txt", "foo", &test_file);
	g_assert (test_file != NULL);

	/* Set up environment */
	tracker_monitor_set_enabled (fixture->monitor, TRUE);

	/* Now, trigger attribute update of the already created file */
	set_file_permissions (fixture->monitored_directory,
	                      "created.txt",
	                      S_IRWXU);

	g_hash_table_insert (fixture->events,
	                     g_object_ref (test_file),
	                     GUINT_TO_POINTER (MONITOR_SIGNAL_NONE));

	/* Wait for events */
	require_events (fixture, test_file, ATTRIBUTE_UPDATED);
	prohibit_events (fixture, test_file, UPDATED | CREATED | MOVED_FROM | MOVED_TO | DELETED);
	events_wait (fixture);

	/* Cleanup environment */
	tracker_monitor_set_enabled (fixture->monitor, FALSE);

	/* Remove the test file */
	g_assert_cmpint (g_file_delete (test_file, NULL, NULL), ==, TRUE);
	g_object_unref (test_file);
}

static void
test_monitor_file_event_deleted (TrackerMonitorTestFixture *fixture,
                                 gconstpointer              data)
{
	GFile *test_file;

	/* Create file to test with, before setting up environment */
	set_file_contents (fixture->monitored_directory, "created.txt", "foo", &test_file);
	g_assert (test_file != NULL);

	/* Set up environment */
	tracker_monitor_set_enabled (fixture->monitor, TRUE);

	/* Now, remove file */
	g_assert_cmpint (g_file_delete (test_file, NULL, NULL), ==, TRUE);
	g_hash_table_insert (fixture->events,
	                     g_object_ref (test_file),
	                     GUINT_TO_POINTER (MONITOR_SIGNAL_NONE));

	/* Wait for events */
	require_events (fixture, test_file, DELETED);
	prohibit_events (fixture, test_file, CREATED | UPDATED | ATTRIBUTE_UPDATED | MOVED_FROM | MOVED_TO);
	events_wait (fixture);

	/* Cleanup environment */
	tracker_monitor_set_enabled (fixture->monitor, FALSE);
	g_object_unref (test_file);
}

static void
test_monitor_file_event_moved_to_monitored (TrackerMonitorTestFixture *fixture,
                                            gconstpointer              data)
{
	GFile *source_file;
	gchar *source_path;
	GFile *dest_file;
	gchar *dest_path;

	/* Create file to test with, before setting up environment */
	set_file_contents (fixture->monitored_directory, "created.txt", "foo", &source_file);
	g_assert (source_file != NULL);

	/* Set up environment */
	tracker_monitor_set_enabled (fixture->monitor, TRUE);

	/* Now, rename the file */
	source_path = g_file_get_path (source_file);
	dest_path = g_build_filename (fixture->monitored_directory, "renamed.txt", NULL);
	dest_file = g_file_new_for_path (dest_path);
	g_assert (dest_file != NULL);

	g_assert_cmpint (g_rename (source_path, dest_path), ==, 0);

	g_hash_table_insert (fixture->events,
	                     g_object_ref (source_file),
	                     GUINT_TO_POINTER (MONITOR_SIGNAL_NONE));
	g_hash_table_insert (fixture->events,
	                     g_object_ref (dest_file),
	                     GUINT_TO_POINTER (MONITOR_SIGNAL_NONE));

	/* Wait for events */
	require_events (fixture, source_file, MOVED_FROM);
	require_events (fixture, dest_file, MOVED_TO);
	prohibit_events (fixture, source_file, CREATED | UPDATED | ATTRIBUTE_UPDATED | MOVED_TO | DELETED);
	prohibit_events (fixture, dest_file, CREATED | ATTRIBUTE_UPDATED | MOVED_FROM | DELETED);
	events_wait (fixture);

	/* Cleanup environment */
	tracker_monitor_set_enabled (fixture->monitor, FALSE);
	g_assert_cmpint (g_file_delete (dest_file, NULL, NULL), ==, TRUE);
	g_object_unref (source_file);
	g_object_unref (dest_file);
	g_free (source_path);
	g_free (dest_path);
}

static void
test_monitor_file_event_moved_to_not_monitored (TrackerMonitorTestFixture *fixture,
                                                gconstpointer              data)
{
	GFile *source_file;
	gchar *source_path;
	GFile *dest_file;
	gchar *dest_path;

	/* Create file to test with, before setting up environment */
	set_file_contents (fixture->monitored_directory, "created.txt", "foo", &source_file);
	g_assert (source_file != NULL);

	/* Set up environment */
	tracker_monitor_set_enabled (fixture->monitor, TRUE);

	/* Now, rename the file */
	source_path = g_file_get_path (source_file);
	dest_path = g_build_filename (fixture->not_monitored_directory, "out.txt", NULL);
	dest_file = g_file_new_for_path (dest_path);
	g_assert (dest_file != NULL);

	g_assert_cmpint (g_rename (source_path, dest_path), ==, 0);

	g_hash_table_insert (fixture->events,
	                     g_object_ref (source_file),
	                     GUINT_TO_POINTER (MONITOR_SIGNAL_NONE));
	g_hash_table_insert (fixture->events,
	                     g_object_ref (dest_file),
	                     GUINT_TO_POINTER (MONITOR_SIGNAL_NONE));

	/* Wait for events */
	require_events (fixture, source_file, DELETED);
	prohibit_events (fixture, source_file, CREATED | UPDATED | ATTRIBUTE_UPDATED | MOVED_TO | MOVED_FROM);
	prohibit_events (fixture, dest_file, CREATED | UPDATED | ATTRIBUTE_UPDATED | MOVED_TO | MOVED_FROM | DELETED);
	events_wait (fixture);

	/* Cleanup environment */
	tracker_monitor_set_enabled (fixture->monitor, FALSE);
	g_assert_cmpint (g_file_delete (dest_file, NULL, NULL), ==, TRUE);
	g_object_unref (source_file);
	g_object_unref (dest_file);
	g_free (source_path);
	g_free (dest_path);
}

static void
test_monitor_file_event_moved_from_not_monitored (TrackerMonitorTestFixture *fixture,
                                                  gconstpointer              data)
{
	GFile *source_file;
	gchar *source_path;
	GFile *dest_file;
	gchar *dest_path;

	/* Create file to test with, before setting up environment */
	set_file_contents (fixture->not_monitored_directory, "created.txt", "foo", &source_file);
	g_assert (source_file != NULL);

	/* Set up environment */
	tracker_monitor_set_enabled (fixture->monitor, TRUE);

	/* Now, rename the file */
	source_path = g_file_get_path (source_file);
	dest_path = g_build_filename (fixture->monitored_directory, "in.txt", NULL);
	dest_file = g_file_new_for_path (dest_path);
	g_assert (dest_file != NULL);

	g_assert_cmpint (g_rename (source_path, dest_path), ==, 0);

	g_hash_table_insert (fixture->events,
	                     g_object_ref (source_file),
	                     GUINT_TO_POINTER (MONITOR_SIGNAL_NONE));
	g_hash_table_insert (fixture->events,
	                     g_object_ref (dest_file),
	                     GUINT_TO_POINTER (MONITOR_SIGNAL_NONE));

	/* Wait for events */
	require_events (fixture, dest_file, CREATED);
	prohibit_events (fixture, source_file, CREATED | UPDATED | ATTRIBUTE_UPDATED | MOVED_TO | MOVED_FROM | DELETED);
	prohibit_events (fixture, dest_file, UPDATED | ATTRIBUTE_UPDATED | MOVED_TO | MOVED_FROM | DELETED);
	events_wait (fixture);

	/* Cleanup environment */
	tracker_monitor_set_enabled (fixture->monitor, FALSE);
	g_assert_cmpint (g_file_delete (dest_file, NULL, NULL), ==, TRUE);
	g_object_unref (source_file);
	g_object_unref (dest_file);
	g_free (source_path);
	g_free (dest_path);
}

/* ----------------------------- DIRECTORY EVENT TESTS --------------------------------- */

static void
test_monitor_directory_event_created (TrackerMonitorTestFixture *fixture,
                                      gconstpointer              data)
{
	GFile *test_dir;

	/* Set up environment */
	tracker_monitor_set_enabled (fixture->monitor, TRUE);

	/* Create directory to test with */
	create_directory (fixture->monitored_directory, "directory", &test_dir);
	g_assert (test_dir != NULL);
	g_hash_table_insert (fixture->events,
	                     g_object_ref (test_dir),
	                     GUINT_TO_POINTER (MONITOR_SIGNAL_NONE));

	/* Wait for events */
	require_events (fixture, test_dir, CREATED);
	prohibit_events (fixture, test_dir, MOVED_TO | MOVED_FROM | DELETED);
	events_wait (fixture);

	/* Cleanup environment */
	tracker_monitor_set_enabled (fixture->monitor, FALSE);

	/* Remove the test dir */
	g_assert_cmpint (g_file_delete (test_dir, NULL, NULL), ==, TRUE);
	g_object_unref (test_dir);
}

static void
test_monitor_directory_event_deleted (TrackerMonitorTestFixture *fixture,
				      gconstpointer              data)
{
	GFile *source_dir;
	gchar *source_path;

	/* Create directory to test with in a monitored place,
	 * before setting up the environment */
	create_directory (fixture->monitored_directory, "directory", &source_dir);
	source_path = g_file_get_path (source_dir);
	g_assert (source_dir != NULL);

	/* Set to monitor the new dir also */
	g_assert_cmpint (tracker_monitor_add (fixture->monitor, source_dir), ==, TRUE);

	/* Set up environment */
	tracker_monitor_set_enabled (fixture->monitor, TRUE);

	/* Now, delete the directory  */
	g_assert_cmpint (g_file_delete (source_dir, NULL, NULL), ==, TRUE);

	g_hash_table_insert (fixture->events,
	                     g_object_ref (source_dir),
	                     GUINT_TO_POINTER (MONITOR_SIGNAL_NONE));

	/* Wait for events */
	require_events (fixture, source_dir, DELETED);
	prohibit_events (fixture, source_dir, CREATED | UPDATED | ATTRIBUTE_UPDATED | MOVED_TO | MOVED_FROM);
	events_wait (fixture);

	/* Cleanup environment */
	tracker_monitor_set_enabled (fixture->monitor, FALSE);
	g_assert_cmpint (tracker_monitor_remove (fixture->monitor, source_dir), ==, TRUE);
	g_object_unref (source_dir);
	g_free (source_path);
}


static void
test_monitor_directory_event_moved_to_monitored (TrackerMonitorTestFixture *fixture,
                                                 gconstpointer              data)
{
	GFile *source_dir;
	gchar *source_path;
	GFile *dest_dir;
	gchar *dest_path;
	GFile *file_in_source_dir;
	GFile *file_in_dest_dir;
	gchar *file_in_dest_dir_path;

	/* Create directory to test with, before setting up the environment */
	create_directory (fixture->monitored_directory, "directory", &source_dir);
	source_path = g_file_get_path (source_dir);
	g_assert (source_dir != NULL);

	/* Add some file to the new dir */
	set_file_contents (source_path, "file.txt", "whatever", &file_in_source_dir);

	/* Set up environment */
	tracker_monitor_set_enabled (fixture->monitor, TRUE);

	/* Set to monitor the new dir also */
	g_assert_cmpint (tracker_monitor_add (fixture->monitor, source_dir), ==, TRUE);

	/* Get final path of the file */
	file_in_dest_dir_path = g_build_path (G_DIR_SEPARATOR_S,
	                                      fixture->monitored_directory,
	                                      "renamed",
	                                      "file.txt",
	                                      NULL);
	file_in_dest_dir = g_file_new_for_path (file_in_dest_dir_path);
	g_assert (file_in_dest_dir != NULL);

	/* Now, rename the directory */
	dest_dir = g_file_get_parent (file_in_dest_dir);
	dest_path = g_file_get_path (dest_dir);

	g_assert_cmpint (g_rename (source_path, dest_path), ==, 0);

	g_hash_table_insert (fixture->events,
	                     g_object_ref (source_dir),
	                     GUINT_TO_POINTER (MONITOR_SIGNAL_NONE));
	g_hash_table_insert (fixture->events,
	                     g_object_ref (dest_dir),
	                     GUINT_TO_POINTER (MONITOR_SIGNAL_NONE));
	g_hash_table_insert (fixture->events,
	                     g_object_ref (file_in_dest_dir),
	                     GUINT_TO_POINTER (MONITOR_SIGNAL_NONE));
	g_hash_table_insert (fixture->events,
	                     g_object_ref (file_in_source_dir),
	                     GUINT_TO_POINTER (MONITOR_SIGNAL_NONE));

	/* Wait for events */
	require_events (fixture, source_dir, MOVED_FROM);
	require_events (fixture, dest_dir, MOVED_TO);
	prohibit_events (fixture, source_dir, CREATED | UPDATED | ATTRIBUTE_UPDATED | MOVED_TO | DELETED);
	prohibit_events (fixture, dest_dir, CREATED | MOVED_FROM | DELETED);
	prohibit_events (fixture, file_in_source_dir, CREATED | UPDATED | ATTRIBUTE_UPDATED | MOVED_FROM | MOVED_TO | DELETED);
	prohibit_events (fixture, file_in_dest_dir, CREATED | UPDATED | ATTRIBUTE_UPDATED | MOVED_FROM | MOVED_TO | DELETED);
	events_wait (fixture);

	/* Cleanup environment */
	tracker_monitor_set_enabled (fixture->monitor, FALSE);
	/* Since tracker 0.9.33, monitors are NOT moved to the new location directly when the
	 * directory is moved, that is done by the upper layers.
	 * Note that monitor is now in dest_dir */
	g_assert_cmpint (tracker_monitor_remove (fixture->monitor, source_dir), ==, TRUE);
	g_assert_cmpint (tracker_monitor_remove (fixture->monitor, dest_dir), !=, TRUE);
	g_assert_cmpint (g_file_delete (file_in_dest_dir, NULL, NULL), ==, TRUE);
	g_assert_cmpint (g_file_delete (dest_dir, NULL, NULL), ==, TRUE);
	g_object_unref (source_dir);
	g_object_unref (file_in_source_dir);
	g_object_unref (dest_dir);
	g_object_unref (file_in_dest_dir);
	g_free (source_path);
	g_free (file_in_dest_dir_path);
	g_free (dest_path);
}

/* Same test as before, BUT, creating a new file in the directory while it's being monitored.
 * In this case, GIO dumps an extra DELETE event after the MOVE
 */
static void
test_monitor_directory_event_moved_to_monitored_after_file_create (TrackerMonitorTestFixture *fixture,
                                                                   gconstpointer              data)
{
	GFile *source_dir;
	gchar *source_path;
	GFile *dest_dir;
	gchar *dest_path;
	GFile *file_in_source_dir;
	GFile *file_in_dest_dir;
	gchar *file_in_dest_dir_path;

	/* Create directory to test with, before setting up the environment */
	create_directory (fixture->monitored_directory, "directory", &source_dir);
	source_path = g_file_get_path (source_dir);
	g_assert (source_dir != NULL);

	/* Set up environment */
	tracker_monitor_set_enabled (fixture->monitor, TRUE);

	/* Set to monitor the new dir also */
	g_assert_cmpint (tracker_monitor_add (fixture->monitor, source_dir), ==, TRUE);

	/* Add some file to the new dir, WHILE ALREADY MONITORING */
	set_file_contents (source_path, "file.txt", "whatever", &file_in_source_dir);

	/* Get final path of the file */
	file_in_dest_dir_path = g_build_path (G_DIR_SEPARATOR_S,
	                                      fixture->monitored_directory,
	                                      "renamed",
	                                      "file.txt",
	                                      NULL);
	file_in_dest_dir = g_file_new_for_path (file_in_dest_dir_path);
	g_assert (file_in_dest_dir != NULL);

	/* Now, rename the directory */
	dest_dir = g_file_get_parent (file_in_dest_dir);
	dest_path = g_file_get_path (dest_dir);

	g_assert_cmpint (g_rename (source_path, dest_path), ==, 0);

	g_hash_table_insert (fixture->events,
	                     g_object_ref (source_dir),
	                     GUINT_TO_POINTER (MONITOR_SIGNAL_NONE));
	g_hash_table_insert (fixture->events,
	                     g_object_ref (dest_dir),
	                     GUINT_TO_POINTER (MONITOR_SIGNAL_NONE));
	g_hash_table_insert (fixture->events,
	                     g_object_ref (file_in_dest_dir),
	                     GUINT_TO_POINTER (MONITOR_SIGNAL_NONE));
	g_hash_table_insert (fixture->events,
	                     g_object_ref (file_in_source_dir),
	                     GUINT_TO_POINTER (MONITOR_SIGNAL_NONE));

	/* Wait for events */
	require_events (fixture, source_dir, MOVED_FROM);
	require_events (fixture, dest_dir, MOVED_TO);
	/* require_events (fixture, file_in_source_dir, CREATED); */   /* This may or may not arrive */
	prohibit_events (fixture, source_dir, CREATED | UPDATED | ATTRIBUTE_UPDATED | MOVED_TO | DELETED);
	prohibit_events (fixture, dest_dir, CREATED | MOVED_FROM | DELETED);
	prohibit_events (fixture, file_in_source_dir, ATTRIBUTE_UPDATED | MOVED_FROM | MOVED_TO | DELETED);
	prohibit_events (fixture, file_in_dest_dir, CREATED | UPDATED | ATTRIBUTE_UPDATED | MOVED_FROM | MOVED_TO | DELETED);
	events_wait (fixture);

	/* Cleanup environment */
	tracker_monitor_set_enabled (fixture->monitor, FALSE);
	/* Since tracker 0.9.33, monitors are NOT moved to the new location directly when the
	 * directory is moved, that is done by the upper layers.
	 * Note that monitor is now in dest_dir */
	g_assert_cmpint (tracker_monitor_remove (fixture->monitor, source_dir), ==, TRUE);
	g_assert_cmpint (tracker_monitor_remove (fixture->monitor, dest_dir), !=, TRUE);
	g_assert_cmpint (g_file_delete (file_in_dest_dir, NULL, NULL), ==, TRUE);
	g_assert_cmpint (g_file_delete (dest_dir, NULL, NULL), ==, TRUE);
	g_object_unref (source_dir);
	g_object_unref (file_in_source_dir);
	g_object_unref (dest_dir);
	g_object_unref (file_in_dest_dir);
	g_free (source_path);
	g_free (file_in_dest_dir_path);
	g_free (dest_path);
}

/* Same test as before, BUT, updating an existing file in the directory while it's being monitored.
 * In this case, GIO dumps an extra DELETE event after the MOVE
 */
static void
test_monitor_directory_event_moved_to_monitored_after_file_update (TrackerMonitorTestFixture *fixture,
                                                                   gconstpointer              data)
{
	GFile *source_dir;
	gchar *source_path;
	GFile *dest_dir;
	gchar *dest_path;
	GFile *file_in_source_dir;
	GFile *file_in_dest_dir;
	gchar *file_in_dest_dir_path;

	/* Create directory to test with, before setting up the environment */
	create_directory (fixture->monitored_directory, "directory", &source_dir);
	source_path = g_file_get_path (source_dir);
	g_assert (source_dir != NULL);

	/* Add some file to the new dir */
	set_file_contents (source_path, "file.txt", "whatever", &file_in_source_dir);

	/* Set up environment */
	tracker_monitor_set_enabled (fixture->monitor, TRUE);

	/* Set to monitor the new dir also */
	g_assert_cmpint (tracker_monitor_add (fixture->monitor, source_dir), ==, TRUE);

	/* Get final path of the file */
	file_in_dest_dir_path = g_build_path (G_DIR_SEPARATOR_S,
	                                      fixture->monitored_directory,
	                                      "renamed",
	                                      "file.txt",
	                                      NULL);
	file_in_dest_dir = g_file_new_for_path (file_in_dest_dir_path);
	g_assert (file_in_dest_dir != NULL);

	/* Update file contents */
	set_file_contents (source_path, "file.txt", "hohoho", &file_in_source_dir);

	/* Now, rename the directory */
	dest_dir = g_file_get_parent (file_in_dest_dir);
	dest_path = g_file_get_path (dest_dir);

	g_assert_cmpint (g_rename (source_path, dest_path), ==, 0);

	g_hash_table_insert (fixture->events,
	                     g_object_ref (source_dir),
	                     GUINT_TO_POINTER (MONITOR_SIGNAL_NONE));
	g_hash_table_insert (fixture->events,
	                     g_object_ref (dest_dir),
	                     GUINT_TO_POINTER (MONITOR_SIGNAL_NONE));
	g_hash_table_insert (fixture->events,
	                     g_object_ref (file_in_dest_dir),
	                     GUINT_TO_POINTER (MONITOR_SIGNAL_NONE));
	g_hash_table_insert (fixture->events,
	                     g_object_ref (file_in_source_dir),
	                     GUINT_TO_POINTER (MONITOR_SIGNAL_NONE));

	/* Wait for events */
	require_events (fixture, source_dir, MOVED_FROM);
	require_events (fixture, dest_dir, MOVED_TO);
	/* require_events (fixture, file_in_source_dir, UPDATED); */  /* This may or may not arrive */
	prohibit_events (fixture, source_dir, CREATED | UPDATED | ATTRIBUTE_UPDATED | DELETED | MOVED_TO);
	prohibit_events (fixture, dest_dir, CREATED | MOVED_FROM | DELETED);
	prohibit_events (fixture, file_in_source_dir, CREATED | DELETED | MOVED_FROM | MOVED_TO);
	prohibit_events (fixture, file_in_dest_dir, CREATED | UPDATED | ATTRIBUTE_UPDATED | DELETED | MOVED_FROM | MOVED_TO);
	events_wait (fixture);

	/* Cleanup environment */
	tracker_monitor_set_enabled (fixture->monitor, FALSE);
	/* Since tracker 0.9.33, monitors are NOT moved to the new location directly when the
	 * directory is moved, that is done by the upper layers.
	 * Note that monitor is now in dest_dir */
	g_assert_cmpint (tracker_monitor_remove (fixture->monitor, source_dir), ==, TRUE);
	g_assert_cmpint (tracker_monitor_remove (fixture->monitor, dest_dir), !=, TRUE);
	g_assert_cmpint (g_file_delete (file_in_dest_dir, NULL, NULL), ==, TRUE);
	g_assert_cmpint (g_file_delete (dest_dir, NULL, NULL), ==, TRUE);
	g_object_unref (source_dir);
	g_object_unref (file_in_source_dir);
	g_object_unref (dest_dir);
	g_object_unref (file_in_dest_dir);
	g_free (source_path);
	g_free (file_in_dest_dir_path);
	g_free (dest_path);
}

static void
test_monitor_directory_event_moved_to_not_monitored (TrackerMonitorTestFixture *fixture,
						     gconstpointer              data)
{
	GFile *source_dir;
	gchar *source_path;
	GFile *dest_dir;
	gchar *dest_path;

	/* Create directory to test with, before setting up the environment */
	create_directory (fixture->monitored_directory, "directory", &source_dir);
	source_path = g_file_get_path (source_dir);
	g_assert (source_dir != NULL);

	/* Set to monitor the new dir also */
	g_assert_cmpint (tracker_monitor_add (fixture->monitor, source_dir), ==, TRUE);

	/* Set up environment */
	tracker_monitor_set_enabled (fixture->monitor, TRUE);

	/* Now, rename the directory */
	dest_path = g_build_path (G_DIR_SEPARATOR_S, fixture->not_monitored_directory, "directory", NULL);
	dest_dir = g_file_new_for_path (dest_path);

	g_assert_cmpint (g_rename (source_path, dest_path), ==, 0);

	g_hash_table_insert (fixture->events,
	                     g_object_ref (source_dir),
	                     GUINT_TO_POINTER (MONITOR_SIGNAL_NONE));
	g_hash_table_insert (fixture->events,
	                     g_object_ref (dest_dir),
	                     GUINT_TO_POINTER (MONITOR_SIGNAL_NONE));

	/* Wait for events */
	require_events (fixture, source_dir, DELETED);
	prohibit_events (fixture, source_dir, CREATED | UPDATED | ATTRIBUTE_UPDATED | MOVED_TO | MOVED_FROM);
	prohibit_events (fixture, dest_dir, CREATED | ATTRIBUTE_UPDATED | MOVED_TO | MOVED_FROM | DELETED);
	events_wait (fixture);

	/* Cleanup environment */
	tracker_monitor_set_enabled (fixture->monitor, FALSE);
	g_assert_cmpint (tracker_monitor_remove (fixture->monitor, source_dir), ==, TRUE);
	/* Note that monitor is NOT in dest_dir, so FAIL if we could remove it */
	g_assert_cmpint (tracker_monitor_remove (fixture->monitor, dest_dir), !=, TRUE);
	g_assert_cmpint (g_file_delete (dest_dir, NULL, NULL), ==, TRUE);
	g_object_unref (source_dir);
	g_object_unref (dest_dir);
	g_free (source_path);
	g_free (dest_path);
}

static void
test_monitor_directory_event_moved_from_not_monitored (TrackerMonitorTestFixture *fixture,
						       gconstpointer              data)
{
	GFile *source_dir;
	gchar *source_path;
	GFile *dest_dir;
	gchar *dest_path;

	/* Create directory to test with in a not-monitored place,
	 * before setting up the environment */
	create_directory (fixture->not_monitored_directory, "foo", &source_dir);
	source_path = g_file_get_path (source_dir);
	g_assert (source_dir != NULL);

	/* Set up environment */
	tracker_monitor_set_enabled (fixture->monitor, TRUE);

	/* Now, rename the directory to somewhere monitored */
	dest_path = g_build_path (G_DIR_SEPARATOR_S, fixture->monitored_directory, "foo", NULL);
	dest_dir = g_file_new_for_path (dest_path);

	g_assert_cmpint (g_rename (source_path, dest_path), ==, 0);

	g_hash_table_insert (fixture->events,
	                     g_object_ref (source_dir),
	                     GUINT_TO_POINTER (MONITOR_SIGNAL_NONE));
	g_hash_table_insert (fixture->events,
	                     g_object_ref (dest_dir),
	                     GUINT_TO_POINTER (MONITOR_SIGNAL_NONE));

	/* Wait for events */
	require_events (fixture, dest_dir, CREATED);
	prohibit_events (fixture, source_dir, CREATED | UPDATED | ATTRIBUTE_UPDATED | MOVED_TO | MOVED_FROM | DELETED);
	prohibit_events (fixture, dest_dir, ATTRIBUTE_UPDATED | MOVED_TO | MOVED_FROM | DELETED);
	events_wait (fixture);

	/* Cleanup environment */
	tracker_monitor_set_enabled (fixture->monitor, FALSE);
	/* Note that monitor is now in dest_dir, BUT TrackerMonitor should
	 * NOT add automatically a new monitor, so FAIL if we can remove it */
	g_assert_cmpint (tracker_monitor_remove (fixture->monitor, dest_dir), !=, TRUE);
	g_assert_cmpint (g_file_delete (dest_dir, NULL, NULL), ==, TRUE);
	g_object_unref (source_dir);
	g_object_unref (dest_dir);
	g_free (source_path);
	g_free (dest_path);
}

/* ----------------------------- BASIC API TESTS --------------------------------- */

static void
test_monitor_basic (void)
{
	TrackerMonitor *monitor;
	gchar *basename;
	gchar *path_for_monitor;
	GFile *file_for_monitor;
	GFile *file_for_tmp;

	/* Setup directories */
	basename = g_strdup_printf ("monitor-test-%d", getpid ());
	path_for_monitor = g_build_path (G_DIR_SEPARATOR_S, g_get_tmp_dir (), basename, NULL);
	g_free (basename);
	g_assert_cmpint (g_mkdir_with_parents (path_for_monitor, 00755), ==, 0);

	file_for_monitor = g_file_new_for_path (path_for_monitor);
	g_assert (G_IS_FILE (file_for_monitor));

	file_for_tmp = g_file_new_for_path (g_get_tmp_dir ());
	g_assert (G_IS_FILE (file_for_tmp));

	/* Create a monitor */
	monitor = tracker_monitor_new ();
	g_assert (monitor != NULL);

	/* Test general API with monitors enabled */
	tracker_monitor_set_enabled (monitor, TRUE);
	g_assert_cmpint (tracker_monitor_get_enabled (monitor), ==, TRUE);

	g_assert_cmpint (tracker_monitor_get_count (monitor), ==, 0);
	g_assert_cmpint (tracker_monitor_add (monitor, file_for_monitor), ==, TRUE);
	g_assert_cmpint (tracker_monitor_add (monitor, file_for_monitor), ==, TRUE); /* Test double add on purpose */
	g_assert_cmpint (tracker_monitor_get_count (monitor), ==, 1);
	g_assert_cmpint (tracker_monitor_is_watched (monitor, file_for_monitor), ==, TRUE);
	g_assert_cmpint (tracker_monitor_is_watched_by_string (monitor, path_for_monitor), ==, TRUE);
	g_assert_cmpint (tracker_monitor_remove (monitor, file_for_monitor), ==, TRUE);
	g_assert_cmpint (tracker_monitor_is_watched (monitor, file_for_monitor), ==, FALSE);
	g_assert_cmpint (tracker_monitor_is_watched_by_string (monitor, path_for_monitor), ==, FALSE);
	g_assert_cmpint (tracker_monitor_get_count (monitor), ==, 0);

	tracker_monitor_add (monitor, file_for_monitor);
	tracker_monitor_add (monitor, file_for_tmp);
	g_assert_cmpint (tracker_monitor_get_count (monitor), ==, 2);
	g_assert_cmpint (tracker_monitor_remove_recursively (monitor, file_for_tmp), ==, TRUE);
	g_assert_cmpint (tracker_monitor_get_count (monitor), ==, 0);

	/* Test general API with monitors disabled */
	tracker_monitor_set_enabled (monitor, FALSE);
	g_assert_cmpint (tracker_monitor_get_enabled (monitor), ==, FALSE);

	g_assert_cmpint (tracker_monitor_get_count (monitor), ==, 0);
	g_assert_cmpint (tracker_monitor_add (monitor, file_for_monitor), ==, TRUE);
	g_assert_cmpint (tracker_monitor_get_count (monitor), ==, 1);
	g_assert_cmpint (tracker_monitor_is_watched (monitor, file_for_monitor), ==, FALSE);
	g_assert_cmpint (tracker_monitor_is_watched_by_string (monitor, path_for_monitor), ==, FALSE);
	g_assert_cmpint (tracker_monitor_remove (monitor, file_for_monitor), ==, TRUE);
	g_assert_cmpint (tracker_monitor_is_watched (monitor, file_for_monitor), ==, FALSE);
	g_assert_cmpint (tracker_monitor_is_watched_by_string (monitor, path_for_monitor), ==, FALSE);
	g_assert_cmpint (tracker_monitor_get_count (monitor), ==, 0);

	tracker_monitor_add (monitor, file_for_monitor);
	tracker_monitor_add (monitor, file_for_tmp);
	g_assert_cmpint (tracker_monitor_get_count (monitor), ==, 2);
	g_assert_cmpint (tracker_monitor_remove_recursively (monitor, file_for_tmp), ==, TRUE);
	g_assert_cmpint (tracker_monitor_get_count (monitor), ==, 0);

	/* Cleanup */
	g_assert_cmpint (g_rmdir (path_for_monitor), ==, 0);
	g_assert (file_for_tmp != NULL);
	g_object_unref (file_for_tmp);
	g_assert (file_for_monitor != NULL);
	g_object_unref (file_for_monitor);
	g_assert (path_for_monitor != NULL);
	g_free (path_for_monitor);
	g_assert (monitor != NULL);
	g_object_unref (monitor);
}

gint
main (gint    argc,
      gchar **argv)
{
	g_test_init (&argc, &argv, NULL);

	g_test_message ("Testing filesystem monitor");

	/* Basic API tests */
	g_test_add_func ("/libtracker-miner/tracker-monitor/basic",
	                 test_monitor_basic);

	/* File Event tests */
	g_test_add ("/libtracker-miner/tracker-monitor/file-event/created",
	            TrackerMonitorTestFixture,
	            NULL,
	            test_monitor_common_setup,
	            test_monitor_file_event_created,
	            test_monitor_common_teardown);
	g_test_add ("/libtracker-miner/tracker-monitor/file-event/updated",
	            TrackerMonitorTestFixture,
	            NULL,
	            test_monitor_common_setup,
	            test_monitor_file_event_updated,
	            test_monitor_common_teardown);
	g_test_add ("/libtracker-miner/tracker-monitor/file-event/attribute-updated",
	            TrackerMonitorTestFixture,
	            NULL,
	            test_monitor_common_setup,
	            test_monitor_file_event_attribute_updated,
	            test_monitor_common_teardown);
	g_test_add ("/libtracker-miner/tracker-monitor/file-event/deleted",
	            TrackerMonitorTestFixture,
	            NULL,
	            test_monitor_common_setup,
	            test_monitor_file_event_deleted,
	            test_monitor_common_teardown);
	g_test_add ("/libtracker-miner/tracker-monitor/file-event/moved/to-monitored",
	            TrackerMonitorTestFixture,
	            NULL,
	            test_monitor_common_setup,
	            test_monitor_file_event_moved_to_monitored,
	            test_monitor_common_teardown);
	g_test_add ("/libtracker-miner/tracker-monitor/file-event/moved/to-not-monitored",
	            TrackerMonitorTestFixture,
	            NULL,
	            test_monitor_common_setup,
	            test_monitor_file_event_moved_to_not_monitored,
	            test_monitor_common_teardown);
	g_test_add ("/libtracker-miner/tracker-monitor/file-event/moved/from-not-monitored",
	            TrackerMonitorTestFixture,
	            NULL,
	            test_monitor_common_setup,
	            test_monitor_file_event_moved_from_not_monitored,
	            test_monitor_common_teardown);

	/* Directory Event tests */
	g_test_add ("/libtracker-miner/tracker-monitor/directory-event/created",
	            TrackerMonitorTestFixture,
	            NULL,
	            test_monitor_common_setup,
	            test_monitor_directory_event_created,
	            test_monitor_common_teardown);
	g_test_add ("/libtracker-miner/tracker-monitor/directory-event/deleted",
	            TrackerMonitorTestFixture,
	            NULL,
	            test_monitor_common_setup,
	            test_monitor_directory_event_deleted,
	            test_monitor_common_teardown);
	g_test_add ("/libtracker-miner/tracker-monitor/directory-event/moved/to-monitored",
	            TrackerMonitorTestFixture,
	            NULL,
	            test_monitor_common_setup,
	            test_monitor_directory_event_moved_to_monitored,
	            test_monitor_common_teardown);
	g_test_add ("/libtracker-miner/tracker-monitor/directory-event/moved/to-monitored-after-file-create",
	            TrackerMonitorTestFixture,
	            NULL,
	            test_monitor_common_setup,
	            test_monitor_directory_event_moved_to_monitored_after_file_create,
	            test_monitor_common_teardown);
	g_test_add ("/libtracker-miner/tracker-monitor/directory-event/moved/to-monitored-after-file-update",
	            TrackerMonitorTestFixture,
	            NULL,
	            test_monitor_common_setup,
	            test_monitor_directory_event_moved_to_monitored_after_file_update,
	            test_monitor_common_teardown);
	g_test_add ("/libtracker-miner/tracker-monitor/directory-event/moved/to-not-monitored",
	            TrackerMonitorTestFixture,
	            NULL,
	            test_monitor_common_setup,
		    test_monitor_directory_event_moved_to_not_monitored,
	            test_monitor_common_teardown);
	g_test_add ("/libtracker-miner/tracker-monitor/directory-event/moved/from-not-monitored",
	            TrackerMonitorTestFixture,
	            NULL,
	            test_monitor_common_setup,
		    test_monitor_directory_event_moved_from_not_monitored,
	            test_monitor_common_teardown);

	return g_test_run ();
}
