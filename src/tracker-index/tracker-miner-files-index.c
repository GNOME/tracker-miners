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

#include "tracker-dbus-files-index.h"
#include "tracker-dbus-files-proxy.h"
#include "tracker-miner-files-index.h"
#include "tracker-miner-files-peer-listener.h"

/* If defined, then a file provided to be indexed MUST be a child in
 * an configured path. if undefined, any file can be indexed, however
 * it is up to applications to maintain files outside the configured
 * locations.
 */
#undef REQUIRE_LOCATION_IN_CONFIG

typedef struct {
	TrackerMinerFilesPeerListener *peer_listener;
	TrackerDBusMinerFilesIndex *skeleton;
	TrackerDBusMinerFilesProxy *proxy_skeleton;
	GDBusConnection *d_connection;
	GArray *indexed_files;
	GStrv graphs;
	gchar *full_name;
	gchar *full_path;
} TrackerMinerFilesIndexPrivate;

enum {
	PROP_0,
	PROP_FILES_MINER
};

#define TRACKER_MINER_FILES_INDEX_GET_PRIVATE(o) (tracker_miner_files_index_get_instance_private (TRACKER_MINER_FILES_INDEX (o)))

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
}

static void
index_finalize (GObject *object)
{
	TrackerMinerFilesIndexPrivate *priv = TRACKER_MINER_FILES_INDEX_GET_PRIVATE (object);

	g_object_unref (priv->skeleton);
	g_object_unref (priv->proxy_skeleton);

	if (priv->d_connection) {
		g_object_unref (priv->d_connection);
	}

	g_clear_object (&priv->peer_listener);
	g_array_unref (priv->indexed_files);
	g_free (priv->full_name);
	g_free (priv->full_path);
	g_strfreev (priv->graphs);
}

static void
update_indexed_files (TrackerMinerFilesIndex *index)
{
	TrackerMinerFilesIndexPrivate *priv;

	priv = TRACKER_MINER_FILES_INDEX_GET_PRIVATE (index);

	tracker_dbus_miner_files_proxy_set_indexed_files (priv->proxy_skeleton,
	                                                  (const gchar * const *) priv->indexed_files->data);
}

static void
tracker_miner_files_index_handle_index_file (TrackerDBusMinerFilesIndex *skeleton,
                                             GDBusMethodInvocation      *invocation,
                                             const gchar                *file_uri,
                                             const gchar * const        *graphs,
                                             TrackerMinerFilesIndex     *index)
{
	TrackerMinerFilesIndexPrivate *priv;
	TrackerDBusRequest *request;
	GFile *file;

	priv = TRACKER_MINER_FILES_INDEX_GET_PRIVATE (index);

	tracker_gdbus_async_return_if_fail (file_uri != NULL, invocation);

	request = tracker_g_dbus_request_begin (invocation, "%s(uri:'%s')", __FUNCTION__, file_uri);

	file = g_file_new_for_uri (file_uri);

	if (!tracker_miner_files_peer_listener_is_file_watched (priv->peer_listener, file)) {
		gchar *uri = g_strdup (file_uri);
		g_array_append_val (priv->indexed_files, uri);
		update_indexed_files (index);
	}

	tracker_miner_files_peer_listener_add_watch (priv->peer_listener,
	                                             g_dbus_method_invocation_get_sender (invocation),
	                                             file, graphs);

	tracker_dbus_request_end (request, NULL);
	g_dbus_method_invocation_return_value (invocation, NULL);

	g_object_unref (file);
}

static void
peer_listener_unwatch_file (TrackerMinerFilesPeerListener *listener,
                            GFile                         *file,
                            gpointer                       user_data)
{
	TrackerMinerFilesIndex *index = user_data;
	TrackerMinerFilesIndexPrivate *priv = TRACKER_MINER_FILES_INDEX_GET_PRIVATE (index);
	gchar *uri;
	gint i;

	uri = g_file_get_uri (file);

	for (i = 0; i < priv->indexed_files->len; i++) {
		const gchar *indexed_uri;

		indexed_uri = g_array_index (priv->indexed_files, gchar*, i);

		if (g_strcmp0 (uri, indexed_uri) == 0) {
			g_array_remove_index (priv->indexed_files, i);
			break;
		}
	}

	update_indexed_files (index);
}

static void
peer_listener_graphs_changed (TrackerMinerFilesPeerListener *listener,
			      GStrv                          graphs,
			      gpointer                       user_data)
{
	TrackerMinerFilesIndexPrivate *priv;

	priv = TRACKER_MINER_FILES_INDEX_GET_PRIVATE (user_data);
	g_strfreev (priv->graphs);
	priv->graphs = g_strdupv (graphs);

	tracker_dbus_miner_files_proxy_set_graphs (priv->proxy_skeleton,
	                                           (const gchar * const *) priv->graphs);
}

static void
string_clear (gpointer data)
{
	gchar **ptr = data;
	g_free (*ptr);
}

static void
tracker_miner_files_index_init (TrackerMinerFilesIndex *object)
{
	TrackerMinerFilesIndexPrivate *priv;

	priv = TRACKER_MINER_FILES_INDEX_GET_PRIVATE (object);

	priv->proxy_skeleton = tracker_dbus_miner_files_proxy_skeleton_new ();

	priv->skeleton = tracker_dbus_miner_files_index_skeleton_new ();
	g_signal_connect (priv->skeleton, "handle-index-file",
	                  G_CALLBACK (tracker_miner_files_index_handle_index_file),
	                  object);
	priv->indexed_files = g_array_new (TRUE, TRUE, sizeof (gchar *));
	g_array_set_clear_func (priv->indexed_files, string_clear);
}

TrackerMinerFilesIndex *
tracker_miner_files_index_new (void)
{
	GObject *miner;
	TrackerMinerFilesIndexPrivate *priv;
	gchar *full_path, *full_name;
	GError *error = NULL;

	miner = g_object_new (TRACKER_TYPE_MINER_FILES_INDEX,
	                      NULL);

	priv = TRACKER_MINER_FILES_INDEX_GET_PRIVATE (miner);

	priv->d_connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);

	if (!priv->d_connection) {
		g_critical ("Could not connect to the D-Bus session bus, %s",
		            error ? error->message : "no error given.");
		g_clear_error (&error);
		g_object_unref (miner);
		return NULL;
	}

	full_name = g_strconcat (TRACKER_MINER_DBUS_NAME_PREFIX, "Files.Index", NULL);
	priv->full_name = full_name;

	/* Register the service name for the miner */
	full_path = g_strconcat (TRACKER_MINER_DBUS_PATH_PREFIX, "Files/Index", NULL);

	g_debug ("Registering D-Bus object...");
	g_debug ("  Path:'%s'", full_path);
	g_debug ("  Object Type:'%s'", G_OBJECT_TYPE_NAME (miner));

	if (!g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (priv->skeleton),
	                                       priv->d_connection,
	                                       full_path,
	                                       &error)) {
		g_critical ("Could not register the D-Bus object %s, %s",
		            full_path,
		            error ? error->message : "no error given.");
		g_clear_error (&error);
		g_object_unref (miner);
		return NULL;
	}

	if (!g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (priv->proxy_skeleton),
	                                       priv->d_connection,
	                                       TRACKER_MINER_DBUS_NAME_PREFIX "Files.Proxy",
	                                       &error)) {
		g_critical ("Could not register the D-Bus object %s, %s",
		            TRACKER_MINER_DBUS_NAME_PREFIX "Files.Proxy",
		            error ? error->message : "no error given.");
		g_clear_error (&error);
		g_object_unref (miner);
		return NULL;
	}

	priv->full_path = full_path;

	priv->peer_listener = tracker_miner_files_peer_listener_new (priv->d_connection);
	g_signal_connect (priv->peer_listener, "unwatch-file",
	                  G_CALLBACK (peer_listener_unwatch_file), miner);
	g_signal_connect (priv->peer_listener, "graphs-changed",
	                  G_CALLBACK (peer_listener_graphs_changed), miner);

	return (TrackerMinerFilesIndex *) miner;
}
