/*
 * Copyright (C) 2017, Red Hat, Inc.
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
 * Authors: Carlos Garnacho <carlosg@gnome.org>
 */

#include "config-miners.h"

#include <gio/gio.h>
#include "tracker-domain-ontology.h"

#define DOMAIN_ONTOLOGY_SECTION "DomainOntology"

#define CACHE_KEY "CacheLocation"
#define JOURNAL_KEY "JournalLocation"
#define ONTOLOGY_KEY "OntologyLocation"
#define ONTOLOGY_NAME_KEY "OntologyName"
#define DOMAIN_KEY "Domain"
#define MINERS_KEY "Miners"

#define DEFAULT_RULE "default.rule"

static gchar *
find_rule_in_data_dirs (const gchar *name)
{
	const gchar* const *data_dirs;
	gchar *path, *rule_name;
	guint i;

	data_dirs = g_get_system_data_dirs ();
	rule_name = g_strconcat (name, ".rule", NULL);

	for (i = 0; data_dirs[i] != NULL; i++) {
		path = g_build_filename (data_dirs[i],
		                         "tracker", "domain-ontologies",
		                         rule_name, NULL);
		if (g_file_test (path, G_FILE_TEST_IS_REGULAR)) {
			g_free (rule_name);
			return path;
		}

		g_free (path);
	}

	g_free (rule_name);

	return NULL;
}

gboolean
tracker_load_domain_config (const gchar  *name,
                            gchar       **dbus_domain_name,
                            GError      **error)
{
	GKeyFile *key_file;
	gchar *path, *path_for_tests;
	GError *inner_error = NULL;

	if (name && name[0] == '/') {
		if (!g_file_test (name, G_FILE_TEST_IS_REGULAR)) {
			inner_error = g_error_new (G_KEY_FILE_ERROR,
			                           G_KEY_FILE_ERROR_NOT_FOUND,
			                           "Could not find rule at '%s'",
			                           name);
			goto end;
		}

		path = g_strdup (name);
	} else if (name) {
		path = find_rule_in_data_dirs (name);

		if (!path) {
			inner_error = g_error_new (G_KEY_FILE_ERROR,
			                           G_KEY_FILE_ERROR_NOT_FOUND,
			                           "Could not find rule '%s' in data dirs",
			                           name);
			goto end;
		}
	} else {
		path = g_build_filename (SHAREDIR, "tracker", "domain-ontologies",
		                         DEFAULT_RULE, NULL);

		if (!g_file_test (path, G_FILE_TEST_IS_REGULAR)) {
			/* This is only for uninstalled tests */
			path_for_tests = g_strdup (g_getenv ("TRACKER_TEST_DOMAIN_ONTOLOGY_RULE"));

			if (path_for_tests == NULL) {
				inner_error = g_error_new (G_KEY_FILE_ERROR,
				                           G_KEY_FILE_ERROR_NOT_FOUND,
				                           "Unable to find default domain ontology rule %s",
				                           path);
				goto end;
			}

			g_free (path);
			path = path_for_tests;
		}
	}

	key_file = g_key_file_new ();
	g_key_file_load_from_file (key_file, path, G_KEY_FILE_NONE, &inner_error);
	g_free (path);

	if (inner_error)
		goto end;

	*dbus_domain_name = g_key_file_get_string (key_file, DOMAIN_ONTOLOGY_SECTION,
	                                           DOMAIN_KEY, &inner_error);
	if (inner_error)
		goto end;

end:
	if (key_file)
		g_key_file_free (key_file);

	if (inner_error) {
		g_propagate_error (error, inner_error);
		return FALSE;
	}

	return TRUE;
}
