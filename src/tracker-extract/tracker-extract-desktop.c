/*
 * Copyright (C) 2018, Red Hat Inc.
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

#include "config.h"

#include <gio/gio.h>

#include "tracker-common.h"
#include <libtracker-extract/tracker-extract.h>

#include "tracker-main.h"

#define GROUP_DESKTOP_ENTRY          "Desktop Entry"

#define SOFTWARE_CATEGORY_URN_PREFIX "urn:software-category:"
#define THEME_ICON_URN_PREFIX        "urn:theme-icon:"

static GKeyFile *
get_desktop_key_file (GFile   *file,
                      gchar  **type,
                      GError **error)
{
	GKeyFile *key_file;
	gchar *path;
	gchar *str;

	path = g_file_get_path (file);
	key_file = g_key_file_new ();
	*type = NULL;

	if (!g_key_file_load_from_file (key_file, path, G_KEY_FILE_NONE, error)) {
		g_key_file_free (key_file);
		g_free (path);
		return NULL;
	}

	str = g_key_file_get_string (key_file, GROUP_DESKTOP_ENTRY, "Type", NULL);

	if (G_UNLIKELY (!str)) {
		*type = NULL;

		g_set_error_literal (error, G_KEY_FILE_ERROR,
		                     G_KEY_FILE_ERROR_GROUP_NOT_FOUND,
		                     "Desktop file doesn't contain type");
		g_key_file_free (key_file);
		g_free (path);
		return NULL;
	} else {
		/* Sanitize type */
		*type = g_strstrip (str);
	}

	g_free (path);

	return key_file;
}

static void
insert_data_from_desktop_file (TrackerResource *resource,
                               const gchar     *metadata_key,
                               GKeyFile        *desktop_file,
                               const gchar     *key,
                               const gchar     *locale)
{
	gchar *str;

	if (locale) {
		/* Try to get the key with our desired LANG locale... */
		str = g_key_file_get_locale_string (desktop_file, GROUP_DESKTOP_ENTRY, key, locale, NULL);
		/* If our desired locale failed, use the list of LANG locales prepared by GLib
		 * (will return untranslated string if none of the locales available) */
		if (!str) {
			str = g_key_file_get_locale_string (desktop_file, GROUP_DESKTOP_ENTRY, key, NULL, NULL);
		}
	} else {
		str = g_key_file_get_string (desktop_file, GROUP_DESKTOP_ENTRY, key, NULL);
	}

	if (str) {
		tracker_resource_set_string (resource, metadata_key, str);
		g_free (str);
	}
}

static gboolean
process_desktop_file (TrackerResource  *resource,
                      GFile            *file)
{
	GKeyFile *key_file;
	GError *error = NULL;
	gchar *name = NULL;
	gchar *type;
	GStrv cats;
	gsize cats_len;
	gboolean is_software = FALSE;
	gchar *lang;

	key_file = get_desktop_key_file (file, &type, &error);
	if (!key_file) {
		gchar *uri;

		uri = g_file_get_uri (file);
		g_warning ("Could not load desktop file '%s': %s",
		           uri,
		           error->message ? error->message : "no error given");

		g_error_free (error);
		g_free (uri);
		return FALSE;
	}

	if (g_key_file_get_boolean (key_file, GROUP_DESKTOP_ENTRY, "Hidden", NULL)) {
		g_debug ("Desktop file is hidden");
		g_key_file_free (key_file);
		return FALSE;
	}

	/* Retrieve LANG locale setup */
	lang = tracker_locale_get (TRACKER_LOCALE_LANGUAGE);

	/* Try to get the categories with our desired LANG locale... */
	cats = g_key_file_get_locale_string_list (key_file, GROUP_DESKTOP_ENTRY, "Categories", lang, &cats_len, NULL);
	if (!cats) {
		/* If our desired locale failed, use the list of LANG locales prepared by GLib
		 * (will return untranslated string if none of the locales available) */
		cats = g_key_file_get_locale_string_list (key_file, GROUP_DESKTOP_ENTRY, "Categories", NULL, &cats_len, NULL);
	}

	if (!name) {
		/* Try to get the name with our desired LANG locale... */
		name = g_key_file_get_locale_string (key_file, GROUP_DESKTOP_ENTRY, "Name", lang, NULL);
		if (!name) {
			/* If our desired locale failed, use the list of LANG locales prepared by GLib
			 * (will return untranslated string if none of the locales available) */
			name = g_key_file_get_locale_string (key_file, GROUP_DESKTOP_ENTRY, "Name", NULL, NULL);
		}
	}

	/* Sanitize name */
	if (name) {
		g_strstrip (name);
	}

	if (name && g_ascii_strcasecmp (type, "Application") == 0) {
		tracker_resource_add_uri (resource, "rdf:type", "nfo:SoftwareApplication");
		is_software = TRUE;
	} else {
		/* Invalid type, all valid types are already listed above */
		g_warning ("Unknown desktop entry type '%s'", type);
		g_key_file_free (key_file);
		g_strfreev (cats);
		g_free (lang);
		g_free (name);
		return FALSE;
	}

	/* We should always always have a proper name if the desktop file is correct
	 * w.r.t to the Freedesktop specs, but sometimes this is not true, so
	 * instead of passing wrong stuff to the SPARQL builder, we avoid it.
	 * If we don't have a proper name, we already warned it before. */
	if (name)
		tracker_resource_set_string (resource, "nie:title", name);

	if (is_software) {
		gchar *icon;

		tracker_resource_add_uri (resource, "rdf:type", "nfo:Executable");
		insert_data_from_desktop_file (resource,
		                               "nie:comment",
		                               key_file,
		                               "Comment",
		                               lang);
		insert_data_from_desktop_file (resource,
		                               "nfo:softwareCmdLine",
		                               key_file,
		                               "Exec",
		                               lang);

		icon = g_key_file_get_string (key_file, GROUP_DESKTOP_ENTRY, "Icon", NULL);

		if (icon) {
			TrackerResource *icon_resource;
			gchar *escaped_icon;
			gchar *icon_uri;

			/* Sanitize icon */
			g_strstrip (icon);

			escaped_icon = g_uri_escape_string (icon, G_URI_RESERVED_CHARS_ALLOWED_IN_PATH, FALSE);

			icon_uri = g_strdup_printf (THEME_ICON_URN_PREFIX "%s", escaped_icon);

			icon_resource = tracker_resource_new (icon_uri);
			tracker_resource_add_uri (icon_resource, "rdf:type", "nfo:Image");
			tracker_resource_set_take_relation (resource, "nfo:softwareIcon", icon_resource);

			g_free (icon_uri);
			g_free (escaped_icon);
			g_free (icon);
		}
	}

	if (cats) {
		gsize i;

		for (i = 0 ; cats[i] && i < cats_len ; i++) {
			TrackerResource *category;
			gchar *cat_uri;
			gchar *cat;

			cat = cats[i];

			if (!cat) {
				continue;
			}

			/* Sanitize category */
			g_strstrip (cat);

			cat_uri = tracker_sparql_escape_uri_printf (SOFTWARE_CATEGORY_URN_PREFIX "%s", cat);

			/* There are also .desktop
			 * files that describe these categories, but we can handle
			 * preemptively creating them if we visit a app .desktop
			 * file that mentions one that we don't yet know about */

			category = tracker_resource_new (cat_uri);
			tracker_resource_add_uri (category, "rdf:type", "nfo:SoftwareCategory");
			tracker_resource_set_string (category, "nie:title", cat);
			g_free (cat_uri);

			tracker_resource_add_take_relation (resource, "nie:isLogicalPartOf", category);
		}
	}

	g_strfreev (cats);

	g_free (name);
	g_free (lang);

	return TRUE;
}

G_MODULE_EXPORT gboolean
tracker_extract_get_metadata (TrackerExtractInfo *info)
{
	TrackerResource *metadata;

	metadata = tracker_resource_new (NULL);

	if (!process_desktop_file (metadata, tracker_extract_info_get_file (info))) {
		g_object_unref (metadata);
		return FALSE;
	}

	tracker_extract_info_set_resource (info, metadata);
	g_object_unref (metadata);

	return TRUE;
}
