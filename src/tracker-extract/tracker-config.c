/*
 * Copyright (C) 2009, Nokia <ivan.frade@nokia.com>
 * Copyright (C) 2014, Lanedo <martyn@lanedo.com>
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

#include "config-miners.h"

#define G_SETTINGS_ENABLE_BACKEND
#include <gio/gsettingsbackend.h>

#include <libtracker-miners-common/tracker-common.h>

#include "tracker-config.h"

#define CONFIG_SCHEMA "org.freedesktop.Tracker3.Extract"
#define CONFIG_PATH   "/org/freedesktop/tracker/extract/"

static void     config_set_property         (GObject       *object,
                                             guint          param_id,
                                             const GValue  *value,
                                             GParamSpec    *pspec);
static void     config_get_property         (GObject       *object,
                                             guint          param_id,
                                             GValue        *value,
                                             GParamSpec    *pspec);
static void     config_finalize             (GObject       *object);
static void     config_constructed          (GObject       *object);

enum {
	PROP_0,
	PROP_MAX_BYTES,
	PROP_TEXT_ALLOWLIST,
	PROP_WAIT_FOR_MINER_FS,
};

G_DEFINE_TYPE (TrackerConfig, tracker_config, G_TYPE_SETTINGS);

static void
tracker_config_class_init (TrackerConfigClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->set_property = config_set_property;
	object_class->get_property = config_get_property;
	object_class->finalize     = config_finalize;
	object_class->constructed  = config_constructed;

	/* General */
	g_object_class_install_property (object_class,
	                                 PROP_MAX_BYTES,
	                                 g_param_spec_int ("max-bytes",
	                                                   "Max Bytes",
	                                                   "Maximum number of UTF-8 bytes to extract per file [0->10485760]",
	                                                   0, 1024 * 1024 * 10,
	                                                   1024 * 1024,
	                                                   G_PARAM_READWRITE));

	g_object_class_install_property (object_class,
	                                 PROP_TEXT_ALLOWLIST,
	                                 g_param_spec_boxed ("text-allowlist",
	                                                     "Text file allowlist",
	                                                     "Filename patterns for plain text documents that should be indexed",
	                                                     G_TYPE_STRV,
	                                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (object_class,
	                                 PROP_WAIT_FOR_MINER_FS,
	                                 g_param_spec_boolean ("wait-for-miner-fs",
	                                                       "Wait for FS miner to be done before extracting",
	                                                       "%TRUE to wait for tracker-miner-fs is done before extracting. %FAlSE otherwise",
	                                                       FALSE,
	                                                       G_PARAM_READWRITE));
}

static void
tracker_config_init (TrackerConfig *object)
{
}

static void
config_set_property (GObject      *object,
                     guint         param_id,
                     const GValue *value,
                     GParamSpec   *pspec)
{
	switch (param_id) {
	/* We don't care about these... we don't save anyway. */
	case PROP_MAX_BYTES:
	case PROP_TEXT_ALLOWLIST:
	case PROP_WAIT_FOR_MINER_FS:
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	};
}

static void
config_get_property (GObject    *object,
                     guint       param_id,
                     GValue     *value,
                     GParamSpec *pspec)
{
	TrackerConfig *config = TRACKER_CONFIG (object);

	switch (param_id) {
	case PROP_MAX_BYTES:
		g_value_set_int (value,
		                 tracker_config_get_max_bytes (config));
		break;

	case PROP_TEXT_ALLOWLIST:
		g_value_take_boxed (value, tracker_gslist_to_string_list (config->text_allowlist));
		break;

	case PROP_WAIT_FOR_MINER_FS:
		g_value_set_boolean (value,
		                     tracker_config_get_wait_for_miner_fs (config));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	};
}

static void
config_set_text_allowlist_conveniences (TrackerConfig *config)
{
	GSList *l;
	GSList *patterns = NULL;

	g_slist_foreach (config->text_allowlist_patterns,
	                 (GFunc) g_pattern_spec_free,
	                 NULL);
	g_slist_free (config->text_allowlist_patterns);

	for (l = config->text_allowlist; l; l = l->next) {
		GPatternSpec *spec;
		const gchar *str = l->data;

		if (str) {
			spec = g_pattern_spec_new (l->data);
			patterns = g_slist_prepend (patterns, spec);
		}
	}

	config->text_allowlist_patterns = g_slist_reverse (patterns);
}

static void
config_finalize (GObject *object)
{
	TrackerConfig *config = TRACKER_CONFIG (object);

	g_slist_foreach (config->text_allowlist_patterns,
	                 (GFunc) g_pattern_spec_free,
	                 NULL);
	g_slist_free (config->text_allowlist);

	(G_OBJECT_CLASS (tracker_config_parent_class)->finalize) (object);

}

static void
config_constructed (GObject *object)
{
	GSettings *settings;

	(G_OBJECT_CLASS (tracker_config_parent_class)->constructed) (object);

	settings = G_SETTINGS (object);

	if (G_LIKELY (!g_getenv ("TRACKER_USE_CONFIG_FILES"))) {
		g_settings_delay (settings);
	}

	/* Set up bindings:
	 *
	 * We don't bind the G_SETTINGS_BIND_SET because we don't want to save
	 * anything, ever, we only want to know about updates to the settings as
	 * they're changed externally. The only time this may be
	 * different is where we use the environment variable
	 * TRACKER_USE_CONFIG_FILES and we want to write a config
	 * file for convenience. But this is only necessary if the
	 * config is different to the default.
	 */
	g_settings_bind (settings, "wait-for-miner-fs", object, "wait-for-miner-fs", G_SETTINGS_BIND_GET);

	/* Cache settings accessed from extractor modules, we don't want
	 * the GSettings object accessed within these as it may trigger
	 * unintended open() calls.
	 */
	TRACKER_CONFIG (settings)->max_bytes = g_settings_get_int (settings, "max-bytes");
	TRACKER_CONFIG (settings)->text_allowlist = tracker_string_list_to_gslist (g_settings_get_strv (settings, "text-allowlist"), -1);

	config_set_text_allowlist_conveniences (TRACKER_CONFIG (settings));
}

TrackerConfig *
tracker_config_new (void)
{
	TrackerConfig *config = NULL;

	/* FIXME: should we unset GSETTINGS_BACKEND env var? */

	if (G_UNLIKELY (g_getenv ("TRACKER_USE_CONFIG_FILES"))) {
		GSettingsBackend *backend;
		gchar *filename, *basename;
		gboolean need_to_save;

		basename = g_strdup_printf ("%s.cfg", g_get_prgname ());
		filename = g_build_filename (g_get_user_config_dir (), "tracker", basename, NULL);
		g_free (basename);

		need_to_save = g_file_test (filename, G_FILE_TEST_EXISTS) == FALSE;

		backend = g_keyfile_settings_backend_new (filename, CONFIG_PATH, "General");
		g_info ("Using config file '%s'", filename);
		g_free (filename);

		config = g_object_new (TRACKER_TYPE_CONFIG,
		                       "backend", backend,
		                       "schema-id", CONFIG_SCHEMA,
		                       "path", CONFIG_PATH,
		                       NULL);
		g_object_unref (backend);

		if (need_to_save) {
			g_info ("  Config file does not exist, using default values...");
		}
	} else {
		config = g_object_new (TRACKER_TYPE_CONFIG,
		                       "schema-id", CONFIG_SCHEMA,
		                       "path", CONFIG_PATH,
		                       NULL);
	}

	return config;
}

gint
tracker_config_get_max_bytes (TrackerConfig *config)
{
	g_return_val_if_fail (TRACKER_IS_CONFIG (config), 0);

	return config->max_bytes;
}

GSList *
tracker_config_get_text_allowlist (TrackerConfig *config)
{
	g_return_val_if_fail (TRACKER_IS_CONFIG (config), NULL);

	return config->text_allowlist;
}

gboolean
tracker_config_get_wait_for_miner_fs (TrackerConfig *config)
{
	g_return_val_if_fail (TRACKER_IS_CONFIG (config), FALSE);

	return g_settings_get_boolean (G_SETTINGS (config), "wait-for-miner-fs");
}


/*
 * Convenience functions
 */
GSList *
tracker_config_get_text_allowlist_patterns (TrackerConfig *config)
{
	return config->text_allowlist_patterns;
}
