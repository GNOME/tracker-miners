/*
 * Copyright (C) 2011, Nokia <ivan.frade@nokia.com>
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
 *
 * Author: Carlos Garnacho <carlos@lanedo.com>
 */

#include "config-miners.h"

#include "tracker-task-pool.h"

enum {
	PROP_0,
	PROP_LIMIT,
	PROP_LIMIT_REACHED
};

typedef struct _TrackerTaskPoolPrivate TrackerTaskPoolPrivate;

struct _TrackerTaskPoolPrivate
{
	GPtrArray *tasks;
	GHashTable *tasks_by_file;
	guint limit;
};

struct _TrackerTask
{
	GFile *file;
	gpointer data;
	GDestroyNotify destroy_notify;
	gint ref_count;
};

G_DEFINE_TYPE_WITH_PRIVATE (TrackerTaskPool, tracker_task_pool, G_TYPE_OBJECT)

static void
tracker_task_pool_finalize (GObject *object)
{
	TrackerTaskPoolPrivate *priv;

	priv = tracker_task_pool_get_instance_private (TRACKER_TASK_POOL (object));
	g_ptr_array_unref (priv->tasks);
	g_hash_table_unref (priv->tasks_by_file);

	G_OBJECT_CLASS (tracker_task_pool_parent_class)->finalize (object);
}

static void
tracker_task_pool_set_property (GObject      *object,
                                guint         param_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
	TrackerTaskPool *pool = TRACKER_TASK_POOL (object);


	switch (param_id) {
	case PROP_LIMIT:
		tracker_task_pool_set_limit (pool,
		                             g_value_get_uint (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
	}
}

static void
tracker_task_pool_get_property (GObject    *object,
                                guint       param_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
	TrackerTaskPool *pool = TRACKER_TASK_POOL (object);

	switch (param_id) {
	case PROP_LIMIT:
		g_value_set_uint (value,
		                  tracker_task_pool_get_limit (pool));
		break;
	case PROP_LIMIT_REACHED:
		g_value_set_boolean (value,
		                     tracker_task_pool_limit_reached (pool));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
	}
}

static void
tracker_task_pool_class_init (TrackerTaskPoolClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = tracker_task_pool_finalize;
	object_class->set_property = tracker_task_pool_set_property;
	object_class->get_property = tracker_task_pool_get_property;

	g_object_class_install_property (object_class,
	                                 PROP_LIMIT,
	                                 g_param_spec_uint ("limit",
	                                                    "Limit",
	                                                    "Task limit",
	                                                    1, G_MAXUINT, 1,
	                                                    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (object_class,
	                                 PROP_LIMIT_REACHED,
	                                 g_param_spec_boolean ("limit-reached",
	                                                       "Limit reached",
	                                                       "Task limit reached",
	                                                       FALSE,
	                                                       G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

static gboolean
file_equal (GFile *file1,
            GFile *file2)
{
	if (file1 == file2) {
		return TRUE;
	} else {
		return g_file_equal (file1, file2);
	}
}

static void
tracker_task_pool_init (TrackerTaskPool *pool)
{
	TrackerTaskPoolPrivate *priv;

	priv = tracker_task_pool_get_instance_private (pool);
	priv->tasks =
		g_ptr_array_new_with_free_func ((GDestroyNotify) tracker_task_unref);
	priv->tasks_by_file =
		g_hash_table_new_full (g_file_hash,
		                       (GEqualFunc) file_equal,
		                       NULL,
		                       (GDestroyNotify) g_list_free);
	priv->limit = 0;
}

TrackerTaskPool *
tracker_task_pool_new (guint limit)
{
	return g_object_new (TRACKER_TYPE_TASK_POOL,
	                     "limit", limit,
	                     NULL);
}

void
tracker_task_pool_set_limit (TrackerTaskPool *pool,
                             guint            limit)
{
	TrackerTaskPoolPrivate *priv;
	gboolean old_limit_reached;

	g_return_if_fail (TRACKER_IS_TASK_POOL (pool));

	old_limit_reached = tracker_task_pool_limit_reached (pool);

	priv = tracker_task_pool_get_instance_private (pool);
	priv->limit = limit;

	if (old_limit_reached !=
	    tracker_task_pool_limit_reached (pool)) {
		g_object_notify (G_OBJECT (pool), "limit-reached");
	}
}

guint
tracker_task_pool_get_limit (TrackerTaskPool *pool)
{
	TrackerTaskPoolPrivate *priv;

	g_return_val_if_fail (TRACKER_IS_TASK_POOL (pool), 0);

	priv = tracker_task_pool_get_instance_private (pool);
	return priv->limit;
}

guint
tracker_task_pool_get_size (TrackerTaskPool *pool)
{
	TrackerTaskPoolPrivate *priv;

	g_return_val_if_fail (TRACKER_IS_TASK_POOL (pool), 0);

	priv = tracker_task_pool_get_instance_private (pool);
	return priv->tasks->len;
}

gboolean
tracker_task_pool_limit_reached (TrackerTaskPool *pool)
{
	TrackerTaskPoolPrivate *priv;

	g_return_val_if_fail (TRACKER_IS_TASK_POOL (pool), FALSE);

	priv = tracker_task_pool_get_instance_private (pool);
	return (priv->tasks->len >= priv->limit);
}

void
tracker_task_pool_add (TrackerTaskPool *pool,
                       TrackerTask     *task)
{
	TrackerTaskPoolPrivate *priv;
	GList *other_tasks;
	GFile *file;

	g_return_if_fail (TRACKER_IS_TASK_POOL (pool));

	priv = tracker_task_pool_get_instance_private (pool);

	file = tracker_task_get_file (task);

	g_ptr_array_add (priv->tasks, tracker_task_ref (task));

	other_tasks = g_hash_table_lookup (priv->tasks_by_file, file);
	g_hash_table_steal (priv->tasks_by_file, file);
	other_tasks = g_list_prepend (other_tasks, task);
	g_hash_table_insert (priv->tasks_by_file, file, other_tasks);

	if (priv->tasks->len == priv->limit) {
		g_object_notify (G_OBJECT (pool), "limit-reached");
	}
}

gboolean
tracker_task_pool_remove (TrackerTaskPool *pool,
                          TrackerTask     *task)
{
	TrackerTaskPoolPrivate *priv;
	GList *other_tasks;
	GFile *file;

	g_return_val_if_fail (TRACKER_IS_TASK_POOL (pool), FALSE);

	priv = tracker_task_pool_get_instance_private (pool);

	file = tracker_task_get_file (task);
	other_tasks = g_hash_table_lookup (priv->tasks_by_file, file);
	g_hash_table_steal (priv->tasks_by_file, file);
	other_tasks = g_list_remove (other_tasks, task);

	if (other_tasks)
		g_hash_table_insert (priv->tasks_by_file, file, other_tasks);
	else
		g_hash_table_remove (priv->tasks_by_file, file);

	if (g_ptr_array_remove_fast (priv->tasks, task)) {
		if (priv->tasks->len == priv->limit - 1) {
			/* We've gone below the threshold again */
			g_object_notify (G_OBJECT (pool), "limit-reached");
		}

		return TRUE;
	}

	return FALSE;
}

void
tracker_task_pool_foreach (TrackerTaskPool *pool,
                           GFunc            func,
                           gpointer         user_data)
{
	TrackerTaskPoolPrivate *priv;
	TrackerTask *task;
	guint i;

	g_return_if_fail (TRACKER_IS_TASK_POOL (pool));
	g_return_if_fail (func != NULL);

	priv = tracker_task_pool_get_instance_private (pool);

	for (i = 0; i < priv->tasks->len; i++) {
		task = g_ptr_array_index (priv->tasks, i);
		(func) (task, user_data);
	}
}

gboolean
tracker_task_pool_find (TrackerTaskPool *pool,
                        GFile           *file)
{
	TrackerTaskPoolPrivate *priv;

	g_return_val_if_fail (TRACKER_IS_TASK_POOL (pool), FALSE);
	g_return_val_if_fail (G_IS_FILE (file), FALSE);

	priv = tracker_task_pool_get_instance_private (pool);
	return g_hash_table_contains (priv->tasks_by_file, file);
}

/* Task */
TrackerTask *
tracker_task_new (GFile          *file,
                  gpointer        data,
                  GDestroyNotify  destroy_notify)
{
	TrackerTask *task;

	task = g_slice_new0 (TrackerTask);
	task->file = g_object_ref (file);
	task->destroy_notify = destroy_notify;
	task->data = data;
	task->ref_count = 1;

	return task;
}

TrackerTask *
tracker_task_ref (TrackerTask *task)
{
	g_return_val_if_fail (task != NULL, NULL);

	g_atomic_int_inc (&task->ref_count);

	return task;
}
void
tracker_task_unref (TrackerTask *task)
{
	g_return_if_fail (task != NULL);

	if (g_atomic_int_dec_and_test (&task->ref_count)) {
		g_object_unref (task->file);

		if (task->data &&
		    task->destroy_notify) {
			(task->destroy_notify) (task->data);
		}

		g_slice_free (TrackerTask, task);
	}
}

GFile *
tracker_task_get_file (TrackerTask *task)
{
	g_return_val_if_fail (task != NULL, NULL);

	return task->file;
}

gpointer
tracker_task_get_data (TrackerTask *task)
{
	g_return_val_if_fail (task != NULL, NULL);

	return task->data;
}
