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
 */

#ifndef __TRACKER_EXTRACT_MODULE_MANAGER_H__
#define __TRACKER_EXTRACT_MODULE_MANAGER_H__

#if !defined (__LIBTRACKER_EXTRACT_INSIDE__) && !defined (TRACKER_COMPILATION)
#error "only <libtracker-extract/tracker-extract.h> must be included directly."
#endif

#include <glib.h>
#include <gmodule.h>

#include <libtracker-sparql/tracker-sparql.h>
#include "tracker-extract-info.h"

G_BEGIN_DECLS

typedef struct _TrackerMimetypeInfo TrackerMimetypeInfo;

typedef gboolean (* TrackerExtractInitFunc)     (GError **error);
typedef void     (* TrackerExtractShutdownFunc) (void);

typedef gboolean (* TrackerExtractMetadataFunc) (TrackerExtractInfo *info);


gboolean  tracker_extract_module_manager_init                (void) G_GNUC_CONST;

GStrv tracker_extract_module_manager_get_all_rdf_types (void);

GStrv     tracker_extract_module_manager_get_rdf_types (const gchar *mimetype);

GModule * tracker_extract_module_manager_get_module (const gchar                 *mimetype,
                                                     const gchar                **rule_out,
                                                     TrackerExtractMetadataFunc  *extract_func_out);

GList * tracker_extract_module_manager_get_matching_rules (const gchar *mimetype);

void tracker_module_manager_load_modules (void);

G_END_DECLS

#endif /* __TRACKER_EXTRACT_MODULE_MANAGER_H__ */
