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
#include <libtracker-miners-common/tracker-enums.h>
#include <libtracker-miners-common/tracker-miners-enum-types.h>
#include <libtracker-sparql/tracker-sparql.h>
#include <libtracker-miner/tracker-miner.h>

#include "tracker-miner-files-index.h"
#include "tracker-miner-files-peer-listener.h"


static const gchar introspection_xml[] =
  "<node>"
  "  <interface name='org.freedesktop.Tracker3.Miner.Files.Index'>"
  "    <method name='IndexLocation'>"
  "      <arg type='s' name='uri' direction='in' />"
  "      <arg type='as' name='flags' direction='in' />"
  "    </method>"
  "  </interface>"
  "</node>";

typedef struct {
	GFile *file;
	TrackerDBusRequest *request;
	GDBusMethodInvocation *invocation;
} AwaitMinerFsData;

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
	TRACKER_MINER_INDEX_ERROR_UNKNOWN_FLAG,
	TRACKER_MINER_INDEX_ERROR_FILE_NOT_FOUND,
	TRACKER_MINER_INDEX_ERROR_DIRECTORIES_ONLY,
	TRACKER_MINER_INDEX_ERROR_NOT_ELIGIBLE,
	TRACKER_MINER_INDEX_N_ERRORS
} TrackerMinerIndexError;

static const GDBusErrorEntry tracker_miner_index_error_entries[] =
{
	{TRACKER_MINER_INDEX_ERROR_UNKNOWN_FLAG, "org.freedesktop.Tracker.Miner.Files.Index.Error.UnknownFlag"},
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

static AwaitMinerFsData *
await_miner_fs_data_new (GFile                 *file,
                         TrackerDBusRequest    *request,
                         GDBusMethodInvocation *invocation)
{
	AwaitMinerFsData *data;

	data = g_slice_new0 (AwaitMinerFsData);
	data->file = g_object_ref (file);
	data->request = request;
	data->invocation = g_object_ref (invocation);

	return data;
}

static void
await_miner_fs_data_free (AwaitMinerFsData *data)
{
	g_object_unref (data->file);
	g_object_unref (data->request);
	g_object_unref (data->invocation);
	g_slice_free (AwaitMinerFsData, data);
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

static TrackerIndexLocationFlags
parse_index_location_flags (const gchar **flags_strv,
                            GError      **error)
{
	TrackerIndexLocationFlags flags = 0;
	GFlagsClass *type_class;
	GFlagsValue *value;

	type_class = g_type_class_ref (TRACKER_TYPE_INDEX_LOCATION_FLAGS);

	while (*flags_strv) {
		const gchar *flag_string = *flags_strv;

		value = g_flags_get_value_by_nick (type_class, flag_string);

		if (value == NULL) {
			g_set_error (error,
			             TRACKER_MINER_INDEX_ERROR,
			             TRACKER_MINER_INDEX_ERROR_UNKNOWN_FLAG,
			             "Unknown flag %s",
			             flag_string);
			break;
		}

		flags |= value->value;

		flags_strv ++;
	}

	g_type_class_unref (type_class);

	return flags;
}

/* Returns TRUE if 'directory' is currently indexed by Tracker */
static gboolean
directory_is_configured_for_indexing (TrackerIndexingTree *indexing_tree,
                                      GFile               *directory)
{
	GFile *root;
	TrackerDirectoryFlags flags;

	root = tracker_indexing_tree_get_root (indexing_tree, directory, &flags);

	if (root) {
		if (flags & TRACKER_DIRECTORY_FLAG_RECURSE) {
			return TRUE;
		} else if (g_file_equal (root, directory) && g_file_has_parent (directory, root)) {
			return TRUE;
		}
	}

	return FALSE;
}

static void
index_directory (TrackerMinerFilesIndex *miner,
                 GFile                  *directory,
                 GDBusMethodInvocation  *invocation,
                 gboolean                watch_for_caller)
{
	TrackerMinerFilesIndexPrivate *priv;
	TrackerIndexingTree *indexing_tree;
	gboolean is_watched, needs_watch = FALSE;

	priv = TRACKER_MINER_FILES_INDEX_GET_PRIVATE (miner);

	indexing_tree = tracker_miner_fs_get_indexing_tree (TRACKER_MINER_FS (priv->files_miner));

	if (directory_is_configured_for_indexing (indexing_tree, directory)) {
		tracker_indexing_tree_notify_update (indexing_tree, directory, TRUE);
		needs_watch = FALSE;
	} else {
		tracker_indexing_tree_add (indexing_tree, directory,
			                       TRACKER_DIRECTORY_FLAG_RECURSE |
			                       TRACKER_DIRECTORY_FLAG_PRIORITY |
			                       TRACKER_DIRECTORY_FLAG_CHECK_MTIME |
			                       TRACKER_DIRECTORY_FLAG_MONITOR);
		needs_watch = TRUE;
	}

	/* If the directory had already subscribers, we want to add all
	 * further watches, so the directory survives as long as there's
	 * watchers.
	 */
	is_watched = tracker_miner_files_peer_listener_is_file_watched (priv->peer_listener, directory);

	if (watch_for_caller && (is_watched || needs_watch)) {
		tracker_miner_files_peer_listener_add_watch (priv->peer_listener,
			                                         g_dbus_method_invocation_get_sender (invocation),
			                                         directory);
	}
}

static void
await_miner_fs_files_processed_cb (TrackerMinerFS *miner_fs,
                                   const gchar    *uri,
                                   gboolean        success,
                                   const gchar    *message,
                                   gpointer        user_data)
{
	AwaitMinerFsData *data = user_data;
	GFile *processed_file;

	processed_file = g_file_new_for_uri (uri);

	if (g_file_equal (data->file, processed_file)) {
		/* We got the file, so the IndexLocation DBus method call can now return */

		g_signal_handlers_disconnect_by_data (miner_fs, data);

		tracker_dbus_request_end (data->request, NULL);

		/* FIXME: return an error if success = FALSE ... */
		g_dbus_method_invocation_return_value (data->invocation, NULL);
	}
}

static void
handle_method_call_index_location (TrackerMinerFilesIndex *miner,
                                   GDBusMethodInvocation  *invocation,
                                   GVariant               *parameters)
{
	TrackerMinerFilesIndexPrivate *priv;
	TrackerDBusRequest *request;
	g_autoptr(GFile) file = NULL;
	GFileInfo *file_info;
	gboolean is_dir;
	g_autoptr(GError) internal_error = NULL;
	const gchar *file_uri;
	g_autofree const gchar **flags_strv = NULL;
	TrackerIndexLocationFlags flags;
	gboolean watch_for_caller;

	priv = TRACKER_MINER_FILES_INDEX_GET_PRIVATE (miner);

	g_variant_get (parameters, "(&s^a&s)", &file_uri, &flags_strv);

	tracker_gdbus_async_return_if_fail (file_uri != NULL, invocation);

	flags = parse_index_location_flags (flags_strv, &internal_error);

	if (internal_error != NULL) {
		g_dbus_method_invocation_return_gerror (invocation, internal_error);

		return;
	}

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

		return;
	}

	watch_for_caller = flags & TRACKER_INDEX_LOCATION_WATCH_FOR_CALLER;

	is_dir = (g_file_info_get_file_type (file_info) == G_FILE_TYPE_DIRECTORY);
	g_object_unref (file_info);

	if (flags & TRACKER_INDEX_LOCATION_AWAIT_MINER_FS) {
		AwaitMinerFsData *data;

		data = await_miner_fs_data_new (file, request, invocation);
		g_signal_connect_data (TRACKER_MINER_FS (priv->files_miner),
		                       "file-processed",
		                       G_CALLBACK (await_miner_fs_files_processed_cb),
		                       data,
		                       (GClosureNotify) await_miner_fs_data_free,
		                       0);
	}

	if (is_dir) {
		index_directory (miner, file, invocation, watch_for_caller);
	} else {
		if (watch_for_caller) {
			internal_error = g_error_new_literal (TRACKER_MINER_INDEX_ERROR,
			                                      TRACKER_MINER_INDEX_ERROR_DIRECTORIES_ONLY,
			                                      "Only directories can be processed in `watch-for-caller` mode");
			tracker_dbus_request_end (request, internal_error);
			g_dbus_method_invocation_return_gerror (invocation, internal_error);

			return;
		} else {
			tracker_miner_fs_check_file (TRACKER_MINER_FS (priv->files_miner),
			                             file, G_PRIORITY_HIGH, FALSE);
		}
	}

	if (flags & TRACKER_INDEX_LOCATION_AWAIT_MINER_FS) {
		/* DBus request will return in await_miner_fs_files_processed_cb */
	} else {
		tracker_dbus_request_end (request, NULL);
		g_dbus_method_invocation_return_value (invocation, NULL);
	}
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

	if (g_strcmp0 (method_name, "IndexLocation") == 0) {
		handle_method_call_index_location (miner, invocation, parameters);
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
