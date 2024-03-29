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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#ifndef __TRACKER_EXTRACT_DECORATOR_H__
#define __TRACKER_EXTRACT_DECORATOR_H__

#include <gio/gio.h>

#include "tracker-extract.h"
#include "tracker-decorator.h"
#include "tracker-extract-persistence.h"

G_BEGIN_DECLS

#define TRACKER_TYPE_EXTRACT_DECORATOR         (tracker_extract_decorator_get_type ())
#define TRACKER_EXTRACT_DECORATOR(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TRACKER_TYPE_EXTRACT_DECORATOR, TrackerExtractDecorator))
#define TRACKER_EXTRACT_DECORATOR_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c), TRACKER_TYPE_EXTRACT_DECORATOR, TrackerExtractDecoratorClass))
#define TRACKER_IS_EXTRACT_DECORATOR(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TRACKER_TYPE_EXTRACT_DECORATOR))
#define TRACKER_IS_EXTRACT_DECORATOR_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c), TRACKER_TYPE_EXTRACT_DECORATOR))
#define TRACKER_EXTRACT_DECORATOR_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TRACKER_TYPE_EXTRACT_DECORATOR, TrackerExtractDecoratorClass))

typedef struct TrackerExtractDecorator TrackerExtractDecorator;
typedef struct TrackerExtractDecoratorClass TrackerExtractDecoratorClass;

struct TrackerExtractDecorator {
	TrackerDecorator parent_instance;
};

struct TrackerExtractDecoratorClass {
	TrackerDecoratorClass parent_class;
};

GType              tracker_extract_decorator_get_type (void) G_GNUC_CONST;

TrackerDecorator * tracker_extract_decorator_new (TrackerSparqlConnection   *connection,
                                                  TrackerExtract            *extractor,
                                                  TrackerExtractPersistence *persistence);

void tracker_extract_decorator_set_throttled (TrackerExtractDecorator *decorator,
                                              gboolean                 throttled);

G_END_DECLS

#endif /* __TRACKER_EXTRACT_DECORATOR_H__ */
