/*
 * Copyright (C) 2010, Nokia <ivan.frade@nokia.com>
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

#include <libtracker-miners-common/tracker-dbus.h>
#include <libtracker-sparql/tracker-sparql.h>
#include <libtracker-miner/tracker-miner.h>

#include "tracker-miner-files-index.h"
#include "tracker-miner-files-peer-listener.h"


static const gchar introspection_xml[] =
  "<node>"
  "  <interface name='org.freedesktop.Tracker3.Miner.Files.Index'>"
  "    <method name='IndexFile'>"
  "      <arg type='s' name='file_uri' direction='in' />"
  "      <arg type='as' name='graphs' direction='in' />"
  "    </method>"
  "    <property name='Graphs' type='as' access='read' />"
  "  </interface>"
  "</node>";

/* If defined, then a file provided to be indexed MUST be a child in
 * an configured path. if undefined, any file can be indexed, however
 * it is up to applications to maintain files outside the configured
 * locations.
 */
#undef REQUIRE_LOCATION_IN_CONFIG

typedef struct {
	TrackerMinerFiles *files_miner;
	TrackerMinerFilesPeerListener *peer_listener;
	GDBusConnection *d_connection;
	GDBusNodeInfo *introspection_data;
	guint registration_id;
	gchar *full_name;
	gchar *full_path;
} TrackerMinerFilesIndexPrivate;

enum {
	PROP_0,
	PROP_FILES_MINER
};

#define TRACKER_MINER_FILES_INDEX_GET_PRIVATE(o) (tracker_miner_files_index_get_instance_private (TRACKER_MINER_FILES_INDEX (o)))

static void     index_set_property        (GObject              *object,
                                           guint                 param_id,
                                           const GValue         *value,
                                           GParamSpec           *pspec);
static void     index_get_property        (GObject              *object,
                                           guint                 param_id,
                                           GValue               *value,
                                           GParamSpec           *pspec);
static void     index_finalize            (GObject              *object);

G_DEFINE_TYPE_WITH_PRIVATE(TrackerMinerFilesIndex, tracker_miner_files_index, G_TYPE_OBJECT)

#define TRACKER_MINER_INDEX_ERROR tracker_miner_index_error_quark ()

GQuark tracker_miner_index_error_quark (void);

typedef enum {
	TRACKER_MINER_INDEX_ERROR_FILE_NOT_FOUND,
	TRACKER_MINER_INDEX_ERROR_DIRECTORIES_ONLY,
	TRACKER_MINER_INDEX_ERROR_NOT_ELIGIBLE,
	TRACKER_MINER_INDEX_N_ERRORS
} TrackerMinerIndexError;

static const GDBusErrorEntry tracker_miner_index_error_entries[] =
{
	{TRACKER_MINER_INDEX_ERROR_FILE_NOT_FOUND, "org.freedesktop.Tracker.Miner.Files.Index.Error.FileNotFound"},
	{TRACKER_MINER_INDEX_ERROR_DIRECTORIES_ONLY, "org.freedesktop.Tracker.Miner.Files.Index.Error.DirectoriesOnly"},
	{TRACKER_MINER_INDEX_ERROR_NOT_ELIGIBLE, "org.freedesktop.Tracker.Miner.Files.Index.Error.NotEligible"},
};

G_STATIC_ASSERT (G_N_ELEMENTS (tracker_miner_index_error_entries) == TRACKER_MINER_INDEX_N_ERRORS);

GQuark
tracker_miner_index_error_quark (void)
{
	static volatile gsize quark_volatile = 0;
	g_dbus_error_register_error_domain ("tracker-miner-index-error-quark",
	                                    &quark_volatile,
	                                    tracker_miner_index_error_entries,
	                                    G_N_ELEMENTS (tracker_miner_index_error_entries));
	return (GQuark) quark_volatile;
}

static void
tracker_miner_files_index_class_init (TrackerMinerFilesIndexClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = index_finalize;
	object_class->set_property = index_set_property;
	object_class->get_property = index_get_property;

	g_object_class_install_property (object_class,
	                                 PROP_FILES_MINER,
	                                 g_param_spec_object ("files-miner",
	                                                      "files-miner",
	                                                      "The FS Miner",
	                                                      TRACKER_TYPE_MINER_FILES,
	                                                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));
}

static void
index_set_property (GObject      *object,
                    guint         param_id,
                    const GValue *value,
                    GParamSpec   *pspec)
{
	TrackerMinerFilesIndexPrivate *priv;

	priv = TRACKER_MINER_FILES_INDEX_GET_PRIVATE (object);

	switch (param_id) {
	case PROP_FILES_MINER:
		priv->files_miner = g_value_dup_object (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}


static void
index_get_property (GObject    *object,
                    guint       param_id,
                    GValue     *value,
                    GParamSpec *pspec)
{
	TrackerMinerFilesIndexPrivate *priv;

	priv = TRACKER_MINER_FILES_INDEX_GET_PRIVATE (object);

	switch (param_id) {
	case PROP_FILES_MINER:
		g_value_set_object (value, priv->files_miner);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
index_finalize (GObject *object)
{
	TrackerMinerFilesIndexPrivate *priv = TRACKER_MINER_FILES_INDEX_GET_PRIVATE (object);

	if (priv->registration_id != 0) {
		g_dbus_connection_unregister_object (priv->d_connection,
		                                     priv->registration_id);
	}

	if (priv->introspection_data) {
		g_dbus_node_info_unref (priv->introspection_data);
	}

	if (priv->d_connection) {
		g_object_unref (priv->d_connection);
	}

	g_clear_object (&priv->peer_listener);
	g_free (priv->full_name);
	g_free (priv->full_path);

	g_object_unref (priv->files_miner);
}

static void
handle_method_call_index_file (TrackerMinerFilesIndex *miner,
                               GDBusMethodInvocation  *invocation,
                               GVariant               *parameters,
                               gboolean                watch_source)
{
	TrackerMinerFilesIndexPrivate *priv;
	TrackerDBusRequest *request;
	GFile *file;
	GFileInfo *file_info;
	gboolean is_dir;
	gboolean do_checks = FALSE;
	GError *internal_error;
	const gchar *file_uri;
	const gchar * const *graphs;

	priv = TRACKER_MINER_FILES_INDEX_GET_PRIVATE (miner);

	g_variant_get (parameters, "(&s^a&s)", &file_uri, &graphs);

	tracker_gdbus_async_return_if_fail (file_uri != NULL, invocation);

	request = tracker_g_dbus_request_begin (invocation, "%s(uri:'%s')", __FUNCTION__, file_uri);

	file = g_file_new_for_uri (file_uri);

	file_info = g_file_query_info (file,
	                               G_FILE_ATTRIBUTE_STANDARD_TYPE,
	                               G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
	                               NULL, NULL);

	if (!file_info) {
		internal_error = g_error_new_literal (TRACKER_MINER_INDEX_ERROR,
		                                      TRACKER_MINER_INDEX_ERROR_FILE_NOT_FOUND,
		                                      "File does not exist");
		tracker_dbus_request_end (request, internal_error);
		g_dbus_method_invocation_return_gerror (invocation, internal_error);

		g_error_free (internal_error);

		g_object_unref (file);

		return;
	}

	is_dir = (g_file_info_get_file_type (file_info) == G_FILE_TYPE_DIRECTORY);
	g_object_unref (file_info);

#ifdef REQUIRE_LOCATION_IN_CONFIG
	do_checks = TRUE;
	if (!tracker_miner_files_is_file_eligible (priv->files_miner, file)) {
		internal_error = g_error_new_literal (TRACKER_MINER_INDEX_ERROR,
		                                      TRACKER_MINER_INDEX_ERROR_NOT_ELIGIBLE,
		                                      "File is not eligible to be indexed");
		tracker_dbus_request_end (request, internal_error);
		g_dbus_method_invocation_return_gerror (invocation, internal_error);

		g_error_free (internal_error);

		g_object_unref (file);

		return;
	}
#endif /* REQUIRE_LOCATION_IN_CONFIG */

	if (is_dir) {
		TrackerIndexingTree *indexing_tree;
		TrackerDirectoryFlags flags;
		gboolean is_watched, needs_watch = FALSE;
		GFile *root;

		indexing_tree = tracker_miner_fs_get_indexing_tree (TRACKER_MINER_FS (priv->files_miner));
		root = tracker_indexing_tree_get_root (indexing_tree, file, &flags);

		/* If the directory had already subscribers, we want to add all
		 * further watches, so the directory survives as long as there's
		 * watchers.
		 */
		is_watched = tracker_miner_files_peer_listener_is_file_watched (priv->peer_listener, file);

		/* Check whether the requested dir is not over a (recursively)
		 * watched directory already, in that case we don't add the
		 * directory (nor add a watch if we're positive it comes from
		 * config).
		 */
		if (!root ||
		    (!(flags & TRACKER_DIRECTORY_FLAG_RECURSE) &&
		     !g_file_equal (root, file) &&
		     !g_file_has_parent (file, root))) {
			tracker_indexing_tree_add (indexing_tree, file,
			                           TRACKER_DIRECTORY_FLAG_RECURSE |
			                           TRACKER_DIRECTORY_FLAG_PRIORITY |
			                           TRACKER_DIRECTORY_FLAG_CHECK_MTIME |
			                           TRACKER_DIRECTORY_FLAG_MONITOR);
			needs_watch = TRUE;
		} else {
			tracker_indexing_tree_notify_update (indexing_tree, file, TRUE);
		}

		if (watch_source && (is_watched || needs_watch)) {
			tracker_miner_files_peer_listener_add_watch (priv->peer_listener,
			                                             g_dbus_method_invocation_get_sender (invocation),
			                                             file);
		}
	} else {
		tracker_miner_fs_check_file (TRACKER_MINER_FS (priv->files_miner),
		                             file, G_PRIORITY_HIGH, do_checks);
	}

	tracker_dbus_request_end (request, NULL);
	g_dbus_method_invocation_return_value (invocation, NULL);

	g_object_unref (file);
}

static void
handle_method_call (GDBusConnection       *connection,
                    const gchar           *sender,
                    const gchar           *object_path,
                    const gchar           *interface_name,
                    const gchar           *method_name,
                    GVariant              *parameters,
                    GDBusMethodInvocation *invocation,
                    gpointer               user_data)
{
	TrackerMinerFilesIndex *miner = user_data;

	tracker_gdbus_async_return_if_fail (miner != NULL, invocation);
	tracker_gdbus_async_return_if_fail (TRACKER_IS_MINER_FILES_INDEX (miner), invocation);

	if (g_strcmp0 (method_name, "IndexFile") == 0) {
		handle_method_call_index_file (miner, invocation, parameters, TRUE);
	} else {
		g_assert_not_reached ();
	}
}

static void
peer_listener_unwatch_file (TrackerMinerFilesPeerListener *listener,
                            GFile                         *file,
                            gpointer                       user_data)
{
	TrackerMinerFilesIndexPrivate *priv;
	TrackerIndexingTree *indexing_tree;

	priv = TRACKER_MINER_FILES_INDEX_GET_PRIVATE (user_data);
	indexing_tree = tracker_miner_fs_get_indexing_tree (TRACKER_MINER_FS (priv->files_miner));
	tracker_indexing_tree_remove (indexing_tree, file);
}

static void
indexing_tree_directory_remove (TrackerIndexingTree *indexing_tree,
                                GFile               *file,
                                gpointer             user_data)
{
	TrackerMinerFilesIndexPrivate *priv;

	priv = TRACKER_MINER_FILES_INDEX_GET_PRIVATE (user_data);
	tracker_miner_files_peer_listener_remove_file (priv->peer_listener, file);
}

static GVariant *
handle_get_property (GDBusConnection  *connection,
                     const gchar      *sender,
                     const gchar      *object_path,
                     const gchar      *interface_name,
                     const gchar      *property_name,
                     GError          **error,
                     gpointer          user_data)
{
	g_assert_not_reached ();
	return NULL;
}

static gboolean
handle_set_property (GDBusConnection  *connection,
                     const gchar      *sender,
                     const gchar      *object_path,
                     const gchar      *interface_name,
                     const gchar      *property_name,
                     GVariant         *value,
                     GError          **error,
                     gpointer          user_data)
{
	g_assert_not_reached ();
	return TRUE;
}

static void
tracker_miner_files_index_init (TrackerMinerFilesIndex *object)
{
}

TrackerMinerFilesIndex *
tracker_miner_files_index_new (TrackerMinerFiles *miner_files)
{
	GObject *miner;
	TrackerMinerFilesIndexPrivate *priv;
	gchar *full_path, *full_name;
	GError *error = NULL;
	TrackerIndexingTree *indexing_tree;
	GDBusInterfaceVTable interface_vtable = {
		handle_method_call,
		handle_get_property,
		handle_set_property
	};

	miner = g_object_new (TRACKER_TYPE_MINER_FILES_INDEX,
	                      "files-miner", miner_files,
	                      NULL);

	priv = TRACKER_MINER_FILES_INDEX_GET_PRIVATE (miner);

	priv->d_connection = g_bus_get_sync (TRACKER_IPC_BUS, NULL, &error);

	if (!priv->d_connection) {
		g_critical ("Could not connect to the D-Bus session bus, %s",
		            error ? error->message : "no error given.");
		g_clear_error (&error);
		g_object_unref (miner);
		return NULL;
	}

	priv->introspection_data = g_dbus_node_info_new_for_xml (introspection_xml, &error);
	if (!priv->introspection_data) {
		g_critical ("Could not create node info from introspection XML, %s",
		            error ? error->message : "no error given.");
		g_clear_error (&error);
		return NULL;
	}

	full_name = g_strconcat (TRACKER_MINER_DBUS_NAME_PREFIX, "Files.Index", NULL);
	priv->full_name = full_name;

	/* Register the service name for the miner */
	full_path = g_strconcat (TRACKER_MINER_DBUS_PATH_PREFIX, "Files/Index", NULL);

	g_debug ("Registering D-Bus object...");
	g_debug ("  Path:'%s'", full_path);
	g_debug ("  Object Type:'%s'", G_OBJECT_TYPE_NAME (miner));

	priv->registration_id =
		g_dbus_connection_register_object (priv->d_connection,
		                                   full_path,
		                                   priv->introspection_data->interfaces[0],
		                                   &interface_vtable,
		                                   miner,
		                                   NULL,
		                                   &error);

	if (error) {
		g_critical ("Could not register the D-Bus object %s, %s",
		            full_path,
		            error ? error->message : "no error given.");
		g_clear_error (&error);
		g_object_unref (miner);
		return NULL;
	}

	priv->full_path = full_path;

	priv->peer_listener = tracker_miner_files_peer_listener_new (priv->d_connection);
	g_signal_connect (priv->peer_listener, "unwatch-file",
	                  G_CALLBACK (peer_listener_unwatch_file), miner);

	indexing_tree = tracker_miner_fs_get_indexing_tree (TRACKER_MINER_FS (miner_files));
	g_signal_connect (indexing_tree, "directory-removed",
	                  G_CALLBACK (indexing_tree_directory_remove), miner);

	return (TrackerMinerFilesIndex *) miner;
}
