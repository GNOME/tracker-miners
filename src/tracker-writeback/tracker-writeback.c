/*
 * Copyright (C) 2011, Nokia <ivan.frade@nokia.com>
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

#include "tracker-writeback.h"
#include "tracker-writeback-module.h"

#include <libtracker-miners-common/tracker-common.h>
#include <libtracker-miner/tracker-miner.h>
#include <libtracker-sparql/tracker-sparql.h>

#include <gio/gio.h>

#ifdef STAYALIVE_ENABLE_TRACE
#warning Stayalive traces enabled
#endif /* STAYALIVE_ENABLE_TRACE */

#ifdef THREAD_ENABLE_TRACE
#warning Controller thread traces enabled
#endif /* THREAD_ENABLE_TRACE */

typedef struct {
	TrackerController *controller;
	GCancellable *cancellable;
	GDBusMethodInvocation *invocation;
	TrackerDBusRequest *request;
	TrackerResource *resource;
	GList *writeback_handlers;
	GError *error;
} WritebackData;

typedef struct {
	GMainContext *context;
	GMainLoop *main_loop;

	GDBusConnection *d_connection;
	GDBusNodeInfo *introspection_data;
	guint registration_id;
	guint bus_name_id;

	GList *ongoing_tasks;

	guint shutdown_timeout;
	GSource *shutdown_source;

	GCond initialization_cond;
	GMutex initialization_mutex, mutex;
	GError *initialization_error;

	guint initialized : 1;

	GHashTable *modules;
	WritebackData *current;
} TrackerControllerPrivate;

#define TRACKER_WRITEBACK_SERVICE   "org.freedesktop.Tracker3.Writeback"
#define TRACKER_WRITEBACK_PATH      "/org/freedesktop/Tracker3/Writeback"
#define TRACKER_WRITEBACK_INTERFACE "org.freedesktop.Tracker3.Writeback"

static const gchar *introspection_xml =
	"<node>"
	"  <interface name='org.freedesktop.Tracker3.Writeback'>"
	"    <method name='Writeback'>"
	"      <arg type='a{sv}' name='rdf' direction='in' />"
	"    </method>"
	"  </interface>"
	"</node>";

enum {
	PROP_0,
	PROP_SHUTDOWN_TIMEOUT,
};

static void     tracker_controller_initable_iface_init  (GInitableIface     *iface);
static gboolean tracker_controller_dbus_start           (TrackerController  *controller,
                                                         GError            **error);
static void     tracker_controller_dbus_stop            (TrackerController  *controller);
static gboolean tracker_controller_start                (TrackerController  *controller,
                                                         GError            **error);

G_DEFINE_TYPE_WITH_CODE (TrackerController, tracker_controller, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (TrackerController)
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE,
                                                tracker_controller_initable_iface_init));

static gboolean
tracker_controller_initable_init (GInitable     *initable,
                                  GCancellable  *cancellable,
                                  GError       **error)
{
	return tracker_controller_start (TRACKER_CONTROLLER (initable), error);
}

static void
tracker_controller_initable_iface_init (GInitableIface *iface)
{
	iface->init = tracker_controller_initable_init;
}

static void
tracker_controller_finalize (GObject *object)
{
	TrackerControllerPrivate *priv;
	TrackerController *controller;

	controller = TRACKER_CONTROLLER (object);
	priv = tracker_controller_get_instance_private (controller);

	if (priv->shutdown_source) {
		g_source_destroy (priv->shutdown_source);
		priv->shutdown_source = NULL;
	}

	tracker_controller_dbus_stop (controller);

	g_hash_table_unref (priv->modules);

	g_main_loop_unref (priv->main_loop);
	g_main_context_unref (priv->context);

	g_cond_clear (&priv->initialization_cond);
	g_mutex_clear (&priv->initialization_mutex);
	g_mutex_clear (&priv->mutex);

	G_OBJECT_CLASS (tracker_controller_parent_class)->finalize (object);
}

static void
tracker_controller_get_property (GObject    *object,
                                 guint       param_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
	TrackerControllerPrivate *priv = tracker_controller_get_instance_private (TRACKER_CONTROLLER (object));

	switch (param_id) {
	case PROP_SHUTDOWN_TIMEOUT:
		g_value_set_uint (value, priv->shutdown_timeout);
		break;
	}
}

static void
tracker_controller_set_property (GObject      *object,
                                 guint         param_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
	TrackerControllerPrivate *priv = tracker_controller_get_instance_private (TRACKER_CONTROLLER (object));

	switch (param_id) {
	case PROP_SHUTDOWN_TIMEOUT:
		priv->shutdown_timeout = g_value_get_uint (value);
		break;
	}
}

static void
tracker_controller_class_init (TrackerControllerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = tracker_controller_finalize;
	object_class->get_property = tracker_controller_get_property;
	object_class->set_property = tracker_controller_set_property;

	g_object_class_install_property (object_class,
	                                 PROP_SHUTDOWN_TIMEOUT,
	                                 g_param_spec_uint ("shutdown-timeout",
	                                                    "Shutdown timeout",
	                                                    "Shutdown timeout, 0 to disable",
	                                                    0, 1000, 0,
	                                                    G_PARAM_READWRITE |
	                                                    G_PARAM_CONSTRUCT_ONLY |
	                                                    G_PARAM_STATIC_STRINGS));
}

static WritebackData *
writeback_data_new (TrackerController       *controller,
                    GList                   *writeback_handlers,
                    TrackerResource         *resource,
                    GDBusMethodInvocation   *invocation,
                    TrackerDBusRequest      *request)
{
	WritebackData *data;

	data = g_slice_new (WritebackData);
	data->cancellable = g_cancellable_new ();
	data->controller = g_object_ref (controller);
	data->resource = g_object_ref (resource);
	data->invocation = invocation;
	data->writeback_handlers = writeback_handlers;
	data->request = request;
	data->error = NULL;

	return data;
}

static void
writeback_data_free (WritebackData *data)
{
	/* We rely on data->invocation being freed through
	 * the g_dbus_method_invocation_return_* methods
	 */
	g_object_unref (data->cancellable);
	g_object_unref (data->resource);

	g_list_foreach (data->writeback_handlers, (GFunc) g_object_unref, NULL);
	g_list_free (data->writeback_handlers);

	if (data->error) {
		g_error_free (data->error);
	}
	g_slice_free (WritebackData, data);
}

static gboolean
reset_shutdown_timeout_cb (gpointer user_data)
{
	TrackerControllerPrivate *priv;

#ifdef STAYALIVE_ENABLE_TRACE
	g_debug ("Stayalive --- time has expired");
#endif /* STAYALIVE_ENABLE_TRACE */

	g_message ("Shutting down due to no activity");

	priv = tracker_controller_get_instance_private (TRACKER_CONTROLLER (user_data));
	g_main_loop_quit (priv->main_loop);

	return FALSE;
}

static void
reset_shutdown_timeout (TrackerController *controller)
{
	TrackerControllerPrivate *priv;
	GSource *source;

	priv = tracker_controller_get_instance_private (controller);

	if (priv->shutdown_timeout == 0) {
		return;
	}

#ifdef STAYALIVE_ENABLE_TRACE
	g_debug ("Stayalive --- (Re)setting timeout");
#endif /* STAYALIVE_ENABLE_TRACE */

	if (priv->shutdown_source) {
		g_source_destroy (priv->shutdown_source);
		priv->shutdown_source = NULL;
	}

	source = g_timeout_source_new_seconds (priv->shutdown_timeout);
	g_source_set_callback (source,
	                       reset_shutdown_timeout_cb,
	                       controller, NULL);

	g_source_attach (source, priv->context);
	priv->shutdown_source = source;
}

static void
tracker_controller_init (TrackerController *controller)
{
	TrackerControllerPrivate *priv;

	priv = tracker_controller_get_instance_private (controller);

	priv->context = g_main_context_new ();
	priv->main_loop = g_main_loop_new (priv->context, FALSE);

	g_cond_init (&priv->initialization_cond);
	g_mutex_init (&priv->initialization_mutex);
	g_mutex_init (&priv->mutex);
}

static gboolean
perform_writeback_cb (gpointer user_data)
{
	TrackerControllerPrivate *priv;
	WritebackData *data;

	data = user_data;
	priv = tracker_controller_get_instance_private (data->controller);
	priv->ongoing_tasks = g_list_remove (priv->ongoing_tasks, data);

	if (data->error == NULL) {
		g_dbus_method_invocation_return_value (data->invocation, NULL);
	} else {
		g_dbus_method_invocation_return_gerror (data->invocation, data->error);
	}

	tracker_dbus_request_end (data->request, NULL);

	g_mutex_lock (&priv->mutex);
	priv->current = NULL;
	g_mutex_unlock (&priv->mutex);

	writeback_data_free (data);

	return FALSE;
}

static void
io_writeback_job (GTask        *task,
                  gpointer      source_object,
                  gpointer      task_data,
                  GCancellable *cancellable)
{
	WritebackData *data = task_data;
	TrackerControllerPrivate *priv = tracker_controller_get_instance_private (data->controller);
	GError *error = NULL;
	gboolean handled = FALSE;
	GList *writeback_handlers;

	g_mutex_lock (&priv->mutex);
	priv->current = data;
	g_mutex_unlock (&priv->mutex);

	writeback_handlers = data->writeback_handlers;

	while (writeback_handlers) {
		handled |= tracker_writeback_write_metadata (writeback_handlers->data,
		                                             data->resource,
		                                             data->cancellable,
		                                             (error) ? NULL : &error);
		writeback_handlers = writeback_handlers->next;
	}

	if (!handled) {
		if (error) {
			data->error = error;
		} else {
			g_set_error_literal (&data->error,
			                     TRACKER_DBUS_ERROR,
			                     TRACKER_DBUS_ERROR_UNSUPPORTED,
			                     "No writeback modules handled "
			                     "successfully this file");
		}
	} else {
		g_clear_error (&error);
	}

	g_idle_add (perform_writeback_cb, data);
}

gboolean
module_matches_resource (TrackerWritebackModule *module,
                         GList                  *types)
{
	TrackerNamespaceManager *namespaces;
	const gchar * const *module_types;
	GList *l;

	module_types = tracker_writeback_module_get_rdf_types (module);
	namespaces = tracker_namespace_manager_get_default ();

	for (l = types; l; l = l->next) {
		GValue *value = l->data;
		const gchar *type;

		if (G_VALUE_HOLDS_STRING (value)) {
			gchar *expanded;
			gboolean match;

			type = g_value_get_string (value);
			expanded = tracker_namespace_manager_expand_uri (namespaces,
			                                                 type);
			match = g_strv_contains (module_types, expanded);
			g_free (expanded);

			if (match)
				return TRUE;
		}
	}

	return FALSE;
}

static void
handle_method_call_writeback (TrackerController     *controller,
                              GDBusMethodInvocation *invocation,
                              GVariant              *parameters)
{
	TrackerControllerPrivate *priv;
	TrackerDBusRequest *request;
	TrackerResource *resource;
	GHashTableIter iter;
	gpointer key, value;
	GList *writeback_handlers = NULL;
	GList *types;

	priv = tracker_controller_get_instance_private (controller);

	reset_shutdown_timeout (controller);
	request = tracker_dbus_request_begin (NULL, "%s", __FUNCTION__);

	resource = tracker_resource_deserialize (g_variant_get_child_value (parameters, 0));
	if (!resource) {
		g_dbus_method_invocation_return_error (invocation,
		                                       G_DBUS_ERROR,
		                                       G_DBUS_ERROR_INVALID_ARGS,
		                                       "GVariant does not serialize to a resource");
		return;
	}

	types = tracker_resource_get_values (resource, "rdf:type");
	if (!types) {
		g_dbus_method_invocation_return_error (invocation,
		                                       G_DBUS_ERROR,
		                                       G_DBUS_ERROR_INVALID_ARGS,
		                                       "Resource does not define rdf:type");
		return;
	}

	g_hash_table_iter_init (&iter, priv->modules);

	while (g_hash_table_iter_next (&iter, &key, &value)) {
		TrackerWritebackModule *module;

		module = value;

		if (module_matches_resource (module, types)) {
			TrackerWriteback *writeback;

			g_debug ("Using module '%s' as a candidate",
			         module->name);

			writeback = tracker_writeback_module_create (module);
			writeback_handlers = g_list_prepend (writeback_handlers, writeback);
		}
	}

	g_list_free (types);

	if (writeback_handlers != NULL) {
		WritebackData *data;
		GTask *task;

		data = writeback_data_new (controller,
		                           writeback_handlers,
		                           resource,
		                           invocation,
		                           request);
		task = g_task_new (controller, data->cancellable, NULL, NULL);

		/* No need to free data here, it's done in the callback. */
		g_task_set_task_data (task, data, NULL);
		g_task_run_in_thread (task, io_writeback_job);
		g_object_unref (task);
	} else {
		g_dbus_method_invocation_return_error (invocation,
		                                       TRACKER_DBUS_ERROR,
		                                       TRACKER_DBUS_ERROR_UNSUPPORTED,
		                                       "Resource description does not match any writeback modules");
	}

	g_object_unref (resource);
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
	TrackerController *controller = user_data;

	if (g_strcmp0 (method_name, "Writeback") == 0) {
		handle_method_call_writeback (controller, invocation, parameters);
	} else {
		g_warning ("Unknown method '%s' called", method_name);
	}
}

static void
controller_notify_main_thread (TrackerController *controller,
                               GError            *error)
{
	TrackerControllerPrivate *priv;

	priv = tracker_controller_get_instance_private (controller);

	g_mutex_lock (&priv->initialization_mutex);

	priv->initialized = TRUE;
	priv->initialization_error = error;

	/* Notify about the initialization */
	g_cond_signal (&priv->initialization_cond);
	g_mutex_unlock (&priv->initialization_mutex);
}

static void
bus_name_acquired_cb (GDBusConnection *connection,
                      const gchar     *name,
                      gpointer         user_data)
{
	controller_notify_main_thread (TRACKER_CONTROLLER (user_data), NULL);
}

static void
bus_name_vanished_cb (GDBusConnection *connection,
                      const gchar     *name,
                      gpointer         user_data)
{
	TrackerController *controller;
	TrackerControllerPrivate *priv;

	controller = user_data;
	priv = tracker_controller_get_instance_private (controller);

	if (!priv->initialized) {
		GError *error;

		error = g_error_new_literal (TRACKER_DBUS_ERROR, 0,
		                             "Could not acquire bus name, "
		                             "perhaps it's already taken?");
		controller_notify_main_thread (controller, error);
	} else {
		/* We're already in control of the program
		 * lifetime, so just quit the mainloop
		 */
		g_main_loop_quit (priv->main_loop);
	}
}

static gboolean
tracker_controller_dbus_start (TrackerController   *controller,
                               GError             **error)
{
	TrackerControllerPrivate *priv;
	GError *err = NULL;
	GDBusInterfaceVTable interface_vtable = {
		handle_method_call,
		NULL, NULL
	};

	priv = tracker_controller_get_instance_private (controller);

	priv->d_connection = g_bus_get_sync (TRACKER_IPC_BUS, NULL, &err);

	if (!priv->d_connection) {
		g_propagate_error (error, err);
		return FALSE;
	}

	priv->introspection_data = g_dbus_node_info_new_for_xml (introspection_xml, &err);
	if (!priv->introspection_data) {
		g_propagate_error (error, err);
		return FALSE;
	}

	g_message ("Registering D-Bus object...");
	g_message ("  Path:'" TRACKER_WRITEBACK_PATH "'");
	g_message ("  Object Type:'%s'", G_OBJECT_TYPE_NAME (controller));

	priv->registration_id =
		g_dbus_connection_register_object (priv->d_connection,
		                                   TRACKER_WRITEBACK_PATH,
		                                   priv->introspection_data->interfaces[0],
		                                   &interface_vtable,
		                                   controller,
		                                   NULL,
		                                   &err);

	if (err) {
		g_critical ("Could not register the D-Bus object "TRACKER_WRITEBACK_PATH", %s",
		            err ? err->message : "no error given.");
		g_propagate_error (error, err);
		return FALSE;
	}

	priv->bus_name_id =
		g_bus_own_name_on_connection (priv->d_connection,
		                              TRACKER_WRITEBACK_SERVICE,
		                              G_BUS_NAME_OWNER_FLAGS_NONE,
		                              bus_name_acquired_cb,
		                              bus_name_vanished_cb,
		                              controller, NULL);

	if (err) {
		g_critical ("Could not own the D-Bus name "TRACKER_WRITEBACK_SERVICE", %s",
		            err ? err->message : "no error given.");
		g_propagate_error (error, err);
		return FALSE;
	}

	return TRUE;
}

static void
tracker_controller_dbus_stop (TrackerController *controller)
{
	TrackerControllerPrivate *priv;

	priv = tracker_controller_get_instance_private (controller);

	if (priv->registration_id != 0) {
		g_dbus_connection_unregister_object (priv->d_connection,
		                                     priv->registration_id);
	}

	if (priv->bus_name_id != 0) {
		g_bus_unown_name (priv->bus_name_id);
	}

	if (priv->introspection_data) {
		g_dbus_node_info_unref (priv->introspection_data);
	}

	if (priv->d_connection) {
		g_object_unref (priv->d_connection);
	}
}

TrackerController *
tracker_controller_new (guint             shutdown_timeout,
                        GError          **error)
{
	return g_initable_new (TRACKER_TYPE_CONTROLLER,
	                       NULL, error,
	                       "shutdown-timeout", shutdown_timeout,
	                       NULL);
}

static gpointer
tracker_controller_thread_func (gpointer user_data)
{
	TrackerController *controller;
	TrackerControllerPrivate *priv;
	GError *error = NULL;

#ifdef THREAD_ENABLE_TRACE
	g_debug ("Thread:%p (Controller) --- Created, dispatching...",
	         g_thread_self ());
#endif /* THREAD_ENABLE_TRACE */

	controller = user_data;
	priv = tracker_controller_get_instance_private (controller);
	g_main_context_push_thread_default (priv->context);

	reset_shutdown_timeout (controller);

	if (!tracker_controller_dbus_start (controller, &error)) {
		/* Error has been filled in, so we return
		 * in this thread. The main thread will be
		 * notified about the error and exit.
		 */
		controller_notify_main_thread (controller, error);
		return NULL;
	}

	g_main_loop_run (priv->main_loop);

#ifdef THREAD_ENABLE_TRACE
	g_debug ("Thread:%p (Controller) --- Shutting down...",
	         g_thread_self ());
#endif /* THREAD_ENABLE_TRACE */

	g_object_unref (controller);

	/* This is where we exit through the timeout being reached
	 */
	exit (0);
	return NULL;
}

static gboolean
tracker_controller_start (TrackerController  *controller,
                          GError            **error)
{
	TrackerControllerPrivate *priv;
	GList *modules;
	GThread *thread;

	priv = tracker_controller_get_instance_private (controller);

	priv->modules = g_hash_table_new_full (g_str_hash,
	                                       g_str_equal,
	                                       (GDestroyNotify) g_free,
	                                       NULL);

	modules = tracker_writeback_modules_list ();

	while (modules) {
		TrackerWritebackModule *module;
		const gchar *path;

		path = modules->data;
		module = tracker_writeback_module_get (path);

		if (module) {
			g_hash_table_insert (priv->modules, g_strdup (path), module);
		}

		modules = modules->next;
	}

	thread = g_thread_try_new ("controller",
	                           tracker_controller_thread_func,
	                           controller,
	                           error);
	if (!thread)
		return FALSE;

	/* We don't want to join it, so just unref the GThread */
	g_thread_unref (thread);

#ifdef THREAD_ENABLE_TRACE
	g_debug ("Thread:%p (Controller) --- Waiting for controller thread to initialize...",
	         g_thread_self ());
#endif /* THREAD_ENABLE_TRACE */

	/* Wait for the controller thread to notify initialization */
	g_mutex_lock (&priv->initialization_mutex);
	while (!priv->initialized)
		g_cond_wait (&priv->initialization_cond, &priv->initialization_mutex);
	g_mutex_unlock (&priv->initialization_mutex);

	/* If there was any error resulting from initialization, propagate it */
	if (priv->initialization_error != NULL) {
		g_propagate_error (error, priv->initialization_error);
		return FALSE;
	}

#ifdef THREAD_ENABLE_TRACE
	g_debug ("Thread:%p (Controller) --- Initialized",
	         g_thread_self ());
#endif /* THREAD_ENABLE_TRACE */

	return TRUE;
}
