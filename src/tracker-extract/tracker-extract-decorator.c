/*
 * Copyright (C) 2014 Carlos Garnacho <carlosg@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "config-miners.h"

#include <libtracker-sparql/tracker-sparql.h>
#include <libtracker-extract/tracker-extract.h>

#include "tracker-extract-decorator.h"
#include "tracker-extract-persistence.h"

enum {
	PROP_EXTRACTOR = 1
};

#define MAX_EXTRACTING_FILES 1

#define TRACKER_EXTRACT_DECORATOR_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TRACKER_TYPE_EXTRACT_DECORATOR, TrackerExtractDecoratorPrivate))

typedef struct _TrackerExtractDecoratorPrivate TrackerExtractDecoratorPrivate;
typedef struct _ExtractData ExtractData;

struct _ExtractData {
	TrackerDecorator *decorator;
	TrackerDecoratorInfo *decorator_info;
	GFile *file;
};

struct _TrackerExtractDecoratorPrivate {
	TrackerExtract *extractor;
	GTimer *timer;
	guint n_extracting_files;

	TrackerExtractPersistence *persistence;
	GHashTable *recovery_files;
};

typedef struct {
	guint watch_id;
	gchar **rdf_types;
} AppData;

/* Preferably classes with tracker:notify true, if an
 * extractor module handles new ones, it must be added
 * here.
 */
static const gchar *supported_classes[] = {
	"nfo:Document",
	"nfo:Audio",
	"nfo:Image",
	"nfo:Video",
	"nfo:FilesystemImage",
	"nmm:Playlist",
	"nfo:SoftwareApplication",
	NULL
};

static GInitableIface *parent_initable_iface;

static void decorator_get_next_file (TrackerDecorator *decorator);
static void tracker_extract_decorator_initable_iface_init (GInitableIface *iface);

static void decorator_ignore_file (GFile    *file,
                                   gpointer  user_data);

G_DEFINE_TYPE_WITH_CODE (TrackerExtractDecorator, tracker_extract_decorator,
                        TRACKER_TYPE_DECORATOR_FS,
                        G_ADD_PRIVATE (TrackerExtractDecorator)
                        G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, tracker_extract_decorator_initable_iface_init))

static void
tracker_extract_decorator_get_property (GObject    *object,
                                        guint       param_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
	TrackerExtractDecoratorPrivate *priv;

	priv = tracker_extract_decorator_get_instance_private (TRACKER_EXTRACT_DECORATOR (object));

	switch (param_id) {
	case PROP_EXTRACTOR:
		g_value_set_object (value, priv->extractor);
		break;
	}
}

static void
tracker_extract_decorator_set_property (GObject      *object,
                                        guint         param_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
	TrackerExtractDecoratorPrivate *priv;

	priv = tracker_extract_decorator_get_instance_private (TRACKER_EXTRACT_DECORATOR (object));

	switch (param_id) {
	case PROP_EXTRACTOR:
		priv->extractor = g_value_dup_object (value);
		break;
	}
}

static void
tracker_extract_decorator_finalize (GObject *object)
{
	TrackerExtractDecoratorPrivate *priv;

	priv = tracker_extract_decorator_get_instance_private (TRACKER_EXTRACT_DECORATOR (object));

	if (priv->extractor)
		g_object_unref (priv->extractor);

	if (priv->timer)
		g_timer_destroy (priv->timer);

	g_hash_table_unref (priv->recovery_files);

	G_OBJECT_CLASS (tracker_extract_decorator_parent_class)->finalize (object);
}

static void
fill_data (TrackerResource *resource,
           const gchar     *url,
           const gchar     *mimetype)
{
	TrackerResource *dataobject;
	GStrv rdf_types;
	gint i;

	dataobject = tracker_resource_new (url);
	tracker_resource_set_string (resource, "nie:mimeType", mimetype);
	tracker_resource_add_take_relation (resource, "nie:isStoredAs", dataobject);
	tracker_resource_add_uri (dataobject, "nie:interpretedAs",
	                          tracker_resource_get_identifier (resource));

	rdf_types = tracker_extract_module_manager_get_rdf_types (mimetype);

	for (i = 0; rdf_types[i] != NULL; i++)
		tracker_resource_add_uri (resource, "rdf:type", rdf_types[i]);
}

static void
get_metadata_cb (TrackerExtract *extract,
                 GAsyncResult   *result,
                 ExtractData    *data)
{
	TrackerExtractDecoratorPrivate *priv;
	TrackerExtractInfo *info;
	TrackerResource *resource;
	GError *error = NULL;
	const gchar *graph, *mime_type, *hash;
	gchar *update_hash_sparql;

	priv = tracker_extract_decorator_get_instance_private (TRACKER_EXTRACT_DECORATOR (data->decorator));
	info = tracker_extract_file_finish (extract, result, &error);

	tracker_extract_persistence_remove_file (priv->persistence, data->file);
	g_hash_table_remove (priv->recovery_files, tracker_decorator_info_get_url (data->decorator_info));

	if (error) {
		decorator_ignore_file (data->file, data->decorator);
		tracker_decorator_info_complete_error (data->decorator_info, error);
	} else {
		gchar *resource_sparql, *sparql;

		mime_type = tracker_extract_info_get_mimetype (info);
		hash = tracker_extract_module_manager_get_hash (mime_type);
		graph = tracker_extract_info_get_graph (info);
		resource = tracker_extract_info_get_resource (info);

		update_hash_sparql =
			g_strdup_printf ("INSERT DATA {"
			                 "  GRAPH tracker:FileSystem {"
			                 "    <%s> tracker:extractorHash \"%s\" ."
			                 "  }"
			                 "}",
			                 tracker_decorator_info_get_url (data->decorator_info),
			                 hash);

		if (resource) {
			fill_data (resource,
			           tracker_decorator_info_get_url (data->decorator_info),
			           mime_type);

			resource_sparql = tracker_resource_print_sparql_update (resource, NULL, graph);

			sparql = g_strdup_printf ("%s; %s",
			                          update_hash_sparql,
			                          resource_sparql);
			g_free (resource_sparql);
			g_free (update_hash_sparql);

			tracker_decorator_info_complete (data->decorator_info, sparql);
		} else {
			tracker_decorator_info_complete (data->decorator_info, update_hash_sparql);
		}

		tracker_extract_info_unref (info);
	}

	priv->n_extracting_files--;
	decorator_get_next_file (data->decorator);

	tracker_decorator_info_unref (data->decorator_info);
	g_object_unref (data->file);
	g_free (data);
}

static GFile *
decorator_get_recovery_file (TrackerExtractDecorator *decorator,
                             TrackerDecoratorInfo    *info)
{
	TrackerExtractDecoratorPrivate *priv;
	GFile *file;

	priv = tracker_extract_decorator_get_instance_private (decorator);
	file = g_hash_table_lookup (priv->recovery_files,
	                            tracker_decorator_info_get_url (info));

	if (file) {
		g_object_ref (file);
	} else {
		file = g_file_new_for_uri (tracker_decorator_info_get_url (info));
	}

	return file;
}

static void
decorator_next_item_cb (TrackerDecorator *decorator,
                        GAsyncResult     *result,
                        gpointer          user_data)
{
	TrackerExtractDecoratorPrivate *priv;
	TrackerDecoratorInfo *info;
	GError *error = NULL;
	ExtractData *data;
	GTask *task;

	priv = tracker_extract_decorator_get_instance_private (TRACKER_EXTRACT_DECORATOR (decorator));
	info = tracker_decorator_next_finish (decorator, result, &error);

	if (!info) {
		priv->n_extracting_files--;

		if (error &&
		    error->domain == tracker_decorator_error_quark ()) {
			switch (error->code) {
			case TRACKER_DECORATOR_ERROR_EMPTY:
				g_message ("There are no further items to extract");
				break;
			case TRACKER_DECORATOR_ERROR_PAUSED:
				g_message ("Next item is on hold because miner is paused");
			}
		} else if (error) {
			g_warning ("Next item could not be processed, %s", error->message);
		}

		g_clear_error (&error);
		return;
	} else if (!tracker_decorator_info_get_url (info)) {
		/* Skip virtual elements with no real file representation */
		priv->n_extracting_files--;
		tracker_decorator_info_unref (info);
		decorator_get_next_file (decorator);
		return;
	}

	data = g_new0 (ExtractData, 1);
	data->decorator = decorator;
	data->decorator_info = info;
	data->file = decorator_get_recovery_file (TRACKER_EXTRACT_DECORATOR (decorator), info);
	task = tracker_decorator_info_get_task (info);

	g_message ("Extracting metadata for '%s'", tracker_decorator_info_get_url (info));

	tracker_extract_persistence_add_file (priv->persistence, data->file);

	tracker_extract_file (priv->extractor,
	                      tracker_decorator_info_get_url (info),
	                      tracker_decorator_info_get_mimetype (info),
	                      g_task_get_cancellable (task),
	                      (GAsyncReadyCallback) get_metadata_cb, data);
}

static void
decorator_get_next_file (TrackerDecorator *decorator)
{
	TrackerExtractDecoratorPrivate *priv;
	guint available_items;

	priv = tracker_extract_decorator_get_instance_private (TRACKER_EXTRACT_DECORATOR (decorator));

	if (!tracker_miner_is_started (TRACKER_MINER (decorator)) ||
	    tracker_miner_is_paused (TRACKER_MINER (decorator)))
		return;

	available_items = tracker_decorator_get_n_items (decorator);
	while (priv->n_extracting_files < MAX_EXTRACTING_FILES &&
	       available_items > 0) {
		priv->n_extracting_files++;
		available_items--;
		tracker_decorator_next (decorator, NULL,
		                        (GAsyncReadyCallback) decorator_next_item_cb,
		                        NULL);
	}
}

static void
tracker_extract_decorator_paused (TrackerMiner *miner)
{
	TrackerExtractDecoratorPrivate *priv;

	priv = tracker_extract_decorator_get_instance_private (TRACKER_EXTRACT_DECORATOR (miner));
	g_message ("Decorator paused");

	if (priv->timer)
		g_timer_stop (priv->timer);
}

static void
tracker_extract_decorator_resumed (TrackerMiner *miner)
{
	TrackerExtractDecoratorPrivate *priv;

	priv = tracker_extract_decorator_get_instance_private (TRACKER_EXTRACT_DECORATOR (miner));
	g_message ("Decorator resumed, processing remaining %d items",
	           tracker_decorator_get_n_items (TRACKER_DECORATOR (miner)));

	if (priv->timer)
		g_timer_continue (priv->timer);

	decorator_get_next_file (TRACKER_DECORATOR (miner));
}

static void
tracker_extract_decorator_items_available (TrackerDecorator *decorator)
{
	TrackerExtractDecoratorPrivate *priv;

	priv = tracker_extract_decorator_get_instance_private (TRACKER_EXTRACT_DECORATOR (decorator));
	g_debug ("Starting to process %d items",
	         tracker_decorator_get_n_items (decorator));

	priv->timer = g_timer_new ();
	if (tracker_miner_is_paused (TRACKER_MINER (decorator)))
		g_timer_stop (priv->timer);

	decorator_get_next_file (decorator);
}

static void
tracker_extract_decorator_finished (TrackerDecorator *decorator)
{
	TrackerExtractDecoratorPrivate *priv;
	gchar *time_str;
	gdouble elapsed = 0;

	priv = tracker_extract_decorator_get_instance_private (TRACKER_EXTRACT_DECORATOR (decorator));
	if (priv->timer) {
		elapsed = g_timer_elapsed (priv->timer, NULL);
		g_clear_pointer (&priv->timer, g_timer_destroy);
	}

	time_str = tracker_seconds_to_string (elapsed, TRUE);
	g_debug ("Extraction finished in %s", time_str);
	g_free (time_str);
}

static void
tracker_extract_decorator_class_init (TrackerExtractDecoratorClass *klass)
{
	TrackerDecoratorClass *decorator_class = TRACKER_DECORATOR_CLASS (klass);
	TrackerMinerClass *miner_class = TRACKER_MINER_CLASS (klass);
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = tracker_extract_decorator_finalize;
	object_class->get_property = tracker_extract_decorator_get_property;
	object_class->set_property = tracker_extract_decorator_set_property;

	miner_class->paused = tracker_extract_decorator_paused;
	miner_class->resumed = tracker_extract_decorator_resumed;

	decorator_class->items_available = tracker_extract_decorator_items_available;
	decorator_class->finished = tracker_extract_decorator_finished;

	g_object_class_install_property (object_class,
	                                 PROP_EXTRACTOR,
	                                 g_param_spec_object ("extractor",
	                                                      "Extractor",
	                                                      "Extractor",
	                                                      TRACKER_TYPE_EXTRACT,
	                                                      G_PARAM_READWRITE |
	                                                      G_PARAM_CONSTRUCT_ONLY |
	                                                      G_PARAM_STATIC_STRINGS));
}

static void
decorator_retry_file (GFile    *file,
                      gpointer  user_data)
{
	TrackerExtractDecorator *decorator = user_data;
	TrackerExtractDecoratorPrivate *priv = tracker_extract_decorator_get_instance_private (decorator);
	gchar *path;

	path = g_file_get_uri (file);
	g_hash_table_insert (priv->recovery_files, path, g_object_ref (file));
	tracker_decorator_fs_prepend_file (TRACKER_DECORATOR_FS (decorator), file);
}

static void
decorator_ignore_file (GFile    *file,
                       gpointer  user_data)
{
	TrackerExtractDecorator *decorator = user_data;
	TrackerSparqlConnection *conn;
	GError *error = NULL;
	gchar *uri, *query;
	const gchar *mimetype, *hash;
	GFileInfo *info;

	uri = g_file_get_uri (file);
	g_message ("Extraction on file '%s' has been attempted too many times, ignoring", uri);

	info = g_file_query_info (file,
	                          G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
	                          G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
	                          NULL, &error);
	if (!info) {
		g_warning ("Could not get mimetype: %s", error->message);
		g_error_free (error);
		return;
	}

	mimetype = g_file_info_get_attribute_string (info,
	                                             G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE);
	hash = tracker_extract_module_manager_get_hash (mimetype);
	g_object_unref (info);

	conn = tracker_miner_get_connection (TRACKER_MINER (decorator));
	query = g_strdup_printf ("INSERT DATA { GRAPH tracker:FileSystem {"
	                         "  <%s> tracker:extractorHash \"%s\" ;"
	                         "}}",
	                         uri, hash);

	tracker_sparql_connection_update (conn, query, NULL, &error);

	if (error) {
		g_warning ("Failed to update ignored file '%s': %s",
		           uri, error->message);
		g_error_free (error);
	}

	g_free (query);
	g_free (uri);
}

static void
tracker_extract_decorator_init (TrackerExtractDecorator *decorator)
{
	TrackerExtractDecoratorPrivate *priv;

	priv = tracker_extract_decorator_get_instance_private (decorator);
	priv->recovery_files = g_hash_table_new_full (g_str_hash, g_str_equal,
	                                              (GDestroyNotify) g_free,
	                                              (GDestroyNotify) g_object_unref);
}

static gboolean
tracker_extract_decorator_initable_init (GInitable     *initable,
                                         GCancellable  *cancellable,
                                         GError       **error)
{
	TrackerExtractDecorator        *decorator;
	TrackerExtractDecoratorPrivate *priv;
	GDBusConnection                *conn;
	gboolean                        ret = TRUE;

	decorator = TRACKER_EXTRACT_DECORATOR (initable);
	priv = tracker_extract_decorator_get_instance_private (TRACKER_EXTRACT_DECORATOR (decorator));

	conn = g_bus_get_sync (TRACKER_IPC_BUS, NULL, error);
	if (conn == NULL) {
		ret = FALSE;
		goto out;
	}

	/* Chainup to parent's init last, to have a chance to export our
	 * DBus interface before RequestName returns. Otherwise our iface
	 * won't be ready by the time the tracker-extract appear on the bus. */
	if (!parent_initable_iface->init (initable, cancellable, error)) {
		ret = FALSE;
	}

	priv->persistence = tracker_extract_persistence_initialize (decorator_retry_file,
	                                                            decorator_ignore_file,
	                                                            decorator);
out:
	g_clear_object (&conn);

	return ret;
}

static void
tracker_extract_decorator_initable_iface_init (GInitableIface *iface)
{
	parent_initable_iface = g_type_interface_peek_parent (iface);
	iface->init = tracker_extract_decorator_initable_init;
}

TrackerDecorator *
tracker_extract_decorator_new (TrackerSparqlConnection  *connection,
                               TrackerExtract           *extract,
                               GCancellable             *cancellable,
                               GError                  **error)
{
	return g_initable_new (TRACKER_TYPE_EXTRACT_DECORATOR,
	                       cancellable, error,
	                       "connection", connection,
	                       "class-names", supported_classes,
	                       "extractor", extract,
	                       NULL);
}
