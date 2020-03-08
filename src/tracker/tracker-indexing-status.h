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

#ifndef __TRACKER_INDEXING_STATUS_H__
#define __TRACKER_INDEXING_STATUS_H__

#include <gio/gio.h>

G_BEGIN_DECLS

typedef struct _TrackerMinerManager TrackerMinerManager;

#define TRACKER_TYPE_INDEXING_STATUS (tracker_indexing_status_get_type())

G_DECLARE_FINAL_TYPE (TrackerIndexingStatus, tracker_indexing_status, TRACKER, INDEXING_STATUS, GObject)

TrackerIndexingStatus *tracker_indexing_status_new (GTask *task,
                                                    GFile *root);

void     tracker_indexing_status_start_watching       (TrackerIndexingStatus *status,
                                                       TrackerMinerManager   *manager,
                                                       guint                  timeout,
                                                       GError               **error);

gboolean       tracker_indexing_status_get_completed        (TrackerIndexingStatus *status);
guint          tracker_indexing_status_get_n_indexed_files  (TrackerIndexingStatus *status);
GList *        tracker_indexing_status_get_errors           (TrackerIndexingStatus *status);
gboolean       tracker_indexing_status_had_error            (TrackerIndexingStatus *status);


G_END_DECLS

#endif /* __TRACKER_INDEXING_STATUS_H__ */
