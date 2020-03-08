/*
 * Copyright (C) 2020 Sam Thursfield <sam@afuera.me.uk>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include <libtracker-sparql/tracker-sparql.h>

#include "tracker-indexing-status.h"
#include "tracker-miner-manager.h"

typedef struct _TrackerIndexingStatus
{
	GObject parent_instance;
} TrackerIndexingStatus;

typedef struct
{
	GRWLock lock;

	TrackerMinerManager *manager;
	gint signal_id;
	gint timeout_id;
	gint timeout;

	TrackerSparqlConnection *miner_fs_sparql;
	TrackerSparqlStatement *stmt;
	gint queries;

	GFile *root;
	GTask *task;
	gboolean root_is_directory;

	gboolean mining_complete;
	GList *succeeded;
	GHashTable *failed;
	GHashTable *to_extract;
} TrackerIndexingStatusPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (TrackerIndexingStatus, tracker_indexing_status, G_TYPE_OBJECT)

enum {
	COMPLETE,
	LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

/**
 * tracker_indexing_status_new:
 *
 * Create a new #TrackerIndexingStatus.
 *
 * Returns: (transfer full): a newly created #TrackerIndexingStatus
 */
TrackerIndexingStatus *
tracker_indexing_status_new (GTask *task,
                             GFile *root)
{
	TrackerIndexingStatus *self;
	TrackerIndexingStatusPrivate *priv;

	self = g_object_new (TRACKER_TYPE_INDEXING_STATUS, NULL);

	priv = tracker_indexing_status_get_instance_private (self);
	priv->root = g_object_ref (root);
	priv->task = g_object_ref (task);

	return self;
}

static void
tracker_indexing_status_finalize (GObject *object)
{
	TrackerIndexingStatus *self = (TrackerIndexingStatus *)object;
	TrackerIndexingStatusPrivate *priv = tracker_indexing_status_get_instance_private (self);

	g_clear_object (&priv->manager);
	g_clear_object (&priv->root);
	g_clear_object (&priv->task);

	g_clear_object (&priv->miner_fs_sparql);
	g_clear_object (&priv->stmt);

	g_hash_table_unref (priv->failed);
	g_hash_table_unref (priv->to_extract);
	g_list_free_full (priv->succeeded, g_object_unref);

	g_rw_lock_clear (&priv->lock);

	G_OBJECT_CLASS (tracker_indexing_status_parent_class)->finalize (object);
}

static void
tracker_indexing_status_class_init (TrackerIndexingStatusClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = tracker_indexing_status_finalize;

	/**
	 * TrackerIndexingStatus::complete:
	 * @status: the #TrackerIndexingStatus
	 *
	 * The ::complete signal is fired when indexing of the given location
	 * is finished.
	 **/
	signals [COMPLETE] =
		g_signal_new ("complete",
		              G_OBJECT_CLASS_TYPE (object_class),
		              G_SIGNAL_RUN_LAST,
		              0,
		              NULL, NULL,
		              NULL,
		              G_TYPE_NONE, 0);
}

static void
tracker_indexing_status_init (TrackerIndexingStatus *self)
{
	TrackerIndexingStatusPrivate *priv = tracker_indexing_status_get_instance_private (self);

	g_rw_lock_init (&priv->lock);

	priv->manager = NULL;
	priv->signal_id = 0;

	priv->mining_complete = FALSE;
	priv->succeeded = NULL;
	priv->failed = g_hash_table_new_full (g_file_hash, (GEqualFunc)g_file_equal, g_object_unref, g_free);
	priv->to_extract = g_hash_table_new_full (g_file_hash, (GEqualFunc)g_file_equal, g_object_unref, NULL);
}

static gboolean
file_is_same_or_child (GFile *root,
                       GFile *file)
{
	gchar *relative_path = NULL;
	gboolean is_child;

	if (g_file_equal (root, file))
		return TRUE;

	relative_path = g_file_get_relative_path (root, file);

	if (relative_path != NULL) {
		is_child = TRUE;
	} else  {
		is_child = FALSE;
	}

	g_free (relative_path);

	return is_child;
}

static void
indexing_complete (TrackerIndexingStatus *status)
{
	g_signal_emit (status, signals[COMPLETE], 0);
}

typedef struct {
	TrackerIndexingStatus *status;
	GFile *file;
} CheckWillBeExtractedData;

static CheckWillBeExtractedData *
check_will_be_extracted_data_new (TrackerIndexingStatus *status,
                                  GFile                 *file)
{
	CheckWillBeExtractedData *data;

	data = g_slice_new0 (CheckWillBeExtractedData);
	data->status = g_object_ref (status);
	data->file = g_object_ref (file);

	return data;
}

static void
check_will_be_extracted_data_free (CheckWillBeExtractedData *data)
{
	g_clear_object (&data->status);
	g_clear_object (&data->file);
	g_slice_free (CheckWillBeExtractedData, data);
}

static void
check_will_be_extracted_cb (GObject      *source_object,
                            GAsyncResult *res,
                            gpointer      user_data)
{
	CheckWillBeExtractedData *data = user_data;
	TrackerIndexingStatusPrivate *priv;
	TrackerSparqlCursor *cursor;
	GError *error = NULL;
	GCancellable *cancellable;

	priv = tracker_indexing_status_get_instance_private (data->status);

	g_rw_lock_writer_lock (&priv->lock);

	cancellable = g_task_get_cancellable (priv->task);

	cursor = tracker_sparql_statement_execute_finish (TRACKER_SPARQL_STATEMENT (source_object), res, &error);

	if (!error) {
		if (tracker_sparql_cursor_next (cursor, cancellable, &error)) {
			if (tracker_sparql_cursor_get_boolean (cursor, 0)) {
				/* Extractor will process this file, so we must wait for that. */
				g_hash_table_insert (priv->to_extract, g_object_ref (data->file), data->file);
			} else {
				/* Extractor will not process this file. */
				priv->succeeded = g_list_prepend (priv->succeeded, g_object_ref (data->file));
			}
		} else if (!error) {
			error = g_error_new (TRACKER_MINER_MANAGER_ERROR, 0, "Internal error: ASK query returned no result");
		}
	}

	if (error) {
		gchar *message;

		message = g_strdup_printf ("Internal error: %s", error->message);
		g_hash_table_insert (priv->failed, g_object_ref (data->file), message);
	}

	g_rw_lock_writer_unlock (&priv->lock);

	priv->queries -= 1;

	check_will_be_extracted_data_free (data);
}

static void
check_will_be_extracted (TrackerIndexingStatus *status,
                         GFile                 *file,
                         const gchar           *uri)
{
	TrackerIndexingStatusPrivate *priv;
	CheckWillBeExtractedData *data;
	GCancellable *cancellable;
	GError *error = NULL;

	priv = tracker_indexing_status_get_instance_private (status);
	cancellable = g_task_get_cancellable (priv->task);

	if (priv->stmt == NULL) {
	/* This list must match the supported_classes list declared in tracker-extract-decorator.c */
		const gchar *query = "ASK { "
		                     "    ?r nie:url <~url> ; "
		                     "        a ?type . "
		                     "    FILTER (?type IN ( "
		                     "                nfo:Document,nfo:Audio,nfo:Image,nfo:Video,"
		                     "                nfo:FilesystemImage,nmm:Playlist,nfo:SoftwareApplication)"
		                     "    )"
		                     "}";
		priv->stmt = tracker_sparql_connection_query_statement (priv->miner_fs_sparql, query, cancellable, &error);

		if (error) {
			g_critical ("Failed to prepare SPARQL statement: %s", error->message);
			g_clear_error (&error);
		}
	}

	data = check_will_be_extracted_data_new (status, file);

	tracker_sparql_statement_bind_string (priv->stmt, "url", uri);
	tracker_sparql_statement_execute_async (priv->stmt,
	                                        g_task_get_cancellable (priv->task),
	                                        check_will_be_extracted_cb,
	                                        data);

	priv->queries += 1;
}

/* Call this when you already have the read or write lock. */
static gboolean
processing_is_completed (TrackerIndexingStatus *status)
{
	TrackerIndexingStatusPrivate *priv;

	priv = tracker_indexing_status_get_instance_private (status);

	if (priv->mining_complete && g_hash_table_size (priv->to_extract) == 0 && priv->queries == 0) {
		return TRUE;
	} else {
		return FALSE;
	}
}

static gboolean
timeout_cb (gpointer user_data)
{
	TrackerIndexingStatus *status = TRACKER_INDEXING_STATUS (user_data);
	TrackerIndexingStatusPrivate *priv;
	GHashTableIter iter;
	GFile *key;

	priv = tracker_indexing_status_get_instance_private (status);

	g_debug ("Indexing timed out. Setting all unextracted files to failed.");

	g_rw_lock_writer_lock (&priv->lock);

	g_signal_handler_disconnect (priv->manager, priv->signal_id);
	priv->signal_id = 0;
	priv->timeout_id = 0;

	g_hash_table_iter_init (&iter, priv->to_extract);
	while (g_hash_table_iter_next (&iter, (gpointer *)&key, NULL)) {
		g_hash_table_insert (priv->failed, g_object_ref (key),
		                     g_strdup ("Timed out waiting for extractor"));
		g_hash_table_iter_remove (&iter);
	}

	g_idle_add_full (G_PRIORITY_DEFAULT_IDLE, (GSourceFunc) indexing_complete, g_object_ref (status), g_object_unref);

	g_rw_lock_writer_unlock (&priv->lock);

	return G_SOURCE_REMOVE;
}

static void
reset_timeout (TrackerIndexingStatus *status)
{
	TrackerIndexingStatusPrivate *priv;

	priv = tracker_indexing_status_get_instance_private (status);

	if (priv->timeout_id) {
		g_source_remove (priv->timeout_id);
		priv->timeout_id = 0;
	}

	priv->timeout_id = g_timeout_add_seconds (priv->timeout, timeout_cb, status);
}

static void
file_processed_cb (TrackerMinerManager   *miner_manager,
                   const gchar           *miner,
                   const gchar           *uri,
                   const gboolean         success,
                   const gchar           *message,
                   TrackerIndexingStatus *status)
{
	GFile *file;
	TrackerIndexingStatusPrivate *priv;

	priv = tracker_indexing_status_get_instance_private (status);

	file = g_file_new_for_uri (uri);

	if (!file_is_same_or_child (priv->root, file)) {
		g_object_unref (file);
		return;
	}

	g_rw_lock_writer_lock (&priv->lock);

	g_main_context_push_thread_default (g_task_get_context (priv->task));

	if (g_str_equal (miner, TRACKER_MINER_FS_DBUS_NAME)) {
		if (success) {
			g_debug ("%s: miner-fs processed successfully", uri);

			check_will_be_extracted (status, file, uri);
		} else {
			g_debug ("%s: error from miner-fs: %s", uri, message);
			g_hash_table_insert (priv->failed, g_object_ref (file), g_strdup (message));
		}

		/* We require that the miner-fs returns file-processed for the root after all
		 * of its children are complete.
		 */
		if (g_file_equal (priv->root, file)) {
			priv->mining_complete = TRUE;
		}
	} else if (g_str_equal (miner, TRACKER_EXTRACT_DBUS_NAME)) {
		g_hash_table_remove (priv->to_extract, file);

		if (success) {
			g_debug ("%s: extractor processed successfully", uri);
			priv->succeeded = g_list_prepend (priv->succeeded, g_object_ref (file));
		} else {
			g_debug ("%s: error from miner-fs: %s", uri, message);
			g_hash_table_insert (priv->failed, g_object_ref (file), g_strdup (message));
		}
	}

	if (processing_is_completed (status)) {
		g_debug ("Indexing complete");

		g_signal_handler_disconnect (priv->manager, priv->signal_id);
		g_source_remove (priv->timeout_id);

		priv->signal_id = 0;
		priv->timeout_id = 0;

		g_idle_add_full (G_PRIORITY_DEFAULT_IDLE, (GSourceFunc) indexing_complete, g_object_ref (status), g_object_unref);
	}

	reset_timeout (status);

	g_main_context_pop_thread_default (g_task_get_context (priv->task));

	g_rw_lock_writer_unlock (&priv->lock);

	g_object_unref (file);
}

/**
 * tracker_indexing_status_start_watching:
 * @status: a #TrackerIndexingStatus instance
 * @manager: a #TrackerMinerManager instance
 * @timeout: a timeout value in seconds
 * @error: return location for a #GError
 *
 * Start monitoring an indexing process using the given
 * #TrackerMinerManager.
 *
 * You should not need to call this function directly, as the
 * tracker_miner_manager_index_file() family of functions will call it for you.
 *
 * In order to avoid hanging, the watch will timeout after @timeout seconds
 * without a signal from any miner process. Pass 0 for the default timeout
 * value of 10 seconds.
 */
void
tracker_indexing_status_start_watching (TrackerIndexingStatus *status,
                                        TrackerMinerManager   *manager,
                                        guint                  timeout,
                                        GError               **error)
{
	TrackerIndexingStatusPrivate *priv;
	GError *inner_error = NULL;

	priv = tracker_indexing_status_get_instance_private (status);

	g_return_if_fail (priv->manager == NULL);

	g_rw_lock_writer_lock (&priv->lock);

	priv->manager = g_object_ref (manager);

	priv->signal_id = g_signal_connect_data (G_OBJECT (manager),
	                                         "miner-file-processed",
	                                          G_CALLBACK (file_processed_cb),
	                                          g_object_ref (status),
	                                          (GClosureNotify)g_object_unref,
	                                          0);

	priv->miner_fs_sparql = tracker_sparql_connection_bus_new (TRACKER_MINER_FS_DBUS_NAME,
	                                                           NULL,
	                                                           tracker_miner_manager_get_dbus_connection (manager),
	                                                           &inner_error);

	if (inner_error) {
		g_propagate_error (error, inner_error);
	}

	priv->timeout = timeout > 0 ? timeout : 10;

	reset_timeout (status);

	g_rw_lock_writer_unlock (&priv->lock);
}

/**
 * tracker_indexing_status_get_completed:
 * @status: a #TrackerIndexingStatus instance
 *
 * Returns: %TRUE if indexing has finished, %FALSE otherwise.
 */
gboolean
tracker_indexing_status_get_completed (TrackerIndexingStatus *status)
{
	TrackerIndexingStatusPrivate *priv;
	guint result;

	priv = tracker_indexing_status_get_instance_private (status);

	g_rw_lock_reader_lock (&priv->lock);

	result = processing_is_completed (status);

	g_rw_lock_reader_unlock (&priv->lock);

	return result;
}

/**
 * tracker_indexing_status_get_n_indexed_files:
 * @status: a #TrackerIndexingStatus instance
 *
 * Return the number of files which have been successfully processed.
 *
 * Returns: a #guint
 */
guint
tracker_indexing_status_get_n_indexed_files (TrackerIndexingStatus *status)
{
	TrackerIndexingStatusPrivate *priv;
	guint result;

	priv = tracker_indexing_status_get_instance_private (status);

	g_rw_lock_reader_lock (&priv->lock);

	result = g_list_length (priv->succeeded);

	g_rw_lock_reader_unlock (&priv->lock);

	return result;
}

/**
 * tracker_indexing_status_get_errors:
 * @status: a #TrackerIndexingStatus instance
 *
 * Return all of the errors encountered so far during indexing.
 * Each string is formatted with the URI and then the error message,
 * for example:
 *
 *     file:///home/sam/Example.mp3: Could not parse file as MP3.
 *
 * If indexing cannot be started, an error is returned via the
 * #GAsyncResult and won't be listed here.
 *
 * Returns: (transfer full): a #GList of strings which you must free.
 */
GList *
tracker_indexing_status_get_errors (TrackerIndexingStatus *status)
{
	TrackerIndexingStatusPrivate *priv;
	GHashTableIter iter;
	GList *result = NULL;
	GFile *key;
	const gchar *value;

	priv = tracker_indexing_status_get_instance_private (status);

	g_rw_lock_reader_lock (&priv->lock);

	g_hash_table_iter_init (&iter, priv->failed);
	while (g_hash_table_iter_next (&iter, (gpointer *)&key, (gpointer *)&value)) {
		gchar *message;
		gchar *uri;

		uri = g_file_get_uri (key);
		message = g_strdup_printf ("%s: %s", uri, value);

		result = g_list_prepend (result, message);

		g_free (uri);
	}

	g_rw_lock_reader_unlock (&priv->lock);

	return result;
}

/**
 * tracker_indexing_status_had_indexing_error:
 * @status: a #TrackerIndexingStatus instance
 *
 * Returns %TRUE if there have been any errors during indexing.
 *
 * If indexing couldn't be started, this will return %FALSE. Use g_task_had_error()
 * to check for this kind of error.
 *
 * Returns: %TRUE if any errors have been encountered during indexing, %FALSE otherwise.
 */
gboolean
tracker_indexing_status_had_error (TrackerIndexingStatus *status)
{
	TrackerIndexingStatusPrivate *priv;
	gboolean result;

	priv = tracker_indexing_status_get_instance_private (status);

	g_rw_lock_reader_lock (&priv->lock);

	result = (g_hash_table_size (priv->failed) > 0);

	g_rw_lock_reader_unlock (&priv->lock);

	return result;
}
