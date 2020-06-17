/*
 * Copyright (C) 2008, Nokia <ivan.frade@nokia.com>
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

#include <sys/statvfs.h>
#include <fcntl.h>
#ifdef __linux__
#include <sys/ioctl.h>
#include <linux/msdos_fs.h>
#endif /* __linux__ */
#include <unistd.h>

#include <glib/gi18n.h>
#include <glib/gstdio.h>

#include <gio/gio.h>
#include <gio/gunixfdlist.h>
#include <gio/gunixinputstream.h>

#include <libtracker-miners-common/tracker-common.h>
#include <libtracker-sparql/tracker-ontologies.h>
#include <libtracker-extract/tracker-extract.h>
#include <libtracker-miner/tracker-miner.h>

#include "tracker-power.h"
#include "tracker-miner-files.h"
#include "tracker-config.h"
#include "tracker-storage.h"
#include "tracker-extract-watchdog.h"

#define DISK_SPACE_CHECK_FREQUENCY 10
#define SECONDS_PER_DAY 86400

/* Stamp files to know crawling/indexing state */
#define FIRST_INDEX_FILENAME          "first-index.txt"
#define LAST_CRAWL_FILENAME           "last-crawl.txt"
#define NEED_MTIME_CHECK_FILENAME     "no-need-mtime-check.txt"

#define DEFAULT_GRAPH "tracker:FileSystem"

#define TRACKER_MINER_FILES_GET_PRIVATE(o) (tracker_miner_files_get_instance_private (TRACKER_MINER_FILES (o)))

static GQuark miner_files_error_quark = 0;

typedef struct ProcessFileData ProcessFileData;

struct ProcessFileData {
	TrackerMinerFiles *miner;
	GCancellable *cancellable;
	GFile *file;
	gchar *mime_type;
	GTask *task;
};

struct TrackerMinerFilesPrivate {
	TrackerConfig *config;
	TrackerStorage *storage;

	TrackerExtractWatchdog *extract_watchdog;
	guint grace_period_timeout_id;

	GVolumeMonitor *volume_monitor;

	GSList *index_recursive_directories;
	GSList *index_single_directories;

	gchar *domain;
	TrackerDomainOntology *domain_ontology;

	guint disk_space_check_id;
	gboolean disk_space_pause;

	gboolean low_battery_pause;

	gboolean start_extractor;

#if defined(HAVE_UPOWER) || defined(HAVE_HAL)
	TrackerPower *power;
#endif /* defined(HAVE_UPOWER) || defined(HAVE_HAL) */
	gulong finished_handler;

	GDBusConnection *connection;

	GQuark quark_mount_point_uuid;

	guint force_recheck_id;

	gboolean mtime_check;
	gboolean index_removable_devices;
	gboolean index_optical_discs;
	guint volumes_changed_id;

	GSList *application_dirs;
	guint applications_changed_id;

	gboolean mount_points_initialized;

	guint stale_volumes_check_id;

	GList *extraction_queue;
};

enum {
	VOLUME_MOUNTED_IN_STORE = 1 << 0,
	VOLUME_MOUNTED = 1 << 1
};

enum {
	PROP_0,
	PROP_CONFIG,
	PROP_DOMAIN,
};

static void        miner_files_set_property             (GObject              *object,
                                                         guint                 param_id,
                                                         const GValue         *value,
                                                         GParamSpec           *pspec);
static void        miner_files_get_property             (GObject              *object,
                                                         guint                 param_id,
                                                         GValue               *value,
                                                         GParamSpec           *pspec);
static void        miner_files_finalize                 (GObject              *object);
static void        miner_files_initable_iface_init      (GInitableIface       *iface);
static gboolean    miner_files_initable_init            (GInitable            *initable,
                                                         GCancellable         *cancellable,
                                                         GError              **error);
static void        mount_pre_unmount_cb                 (GVolumeMonitor       *volume_monitor,
                                                         GMount               *mount,
                                                         TrackerMinerFiles    *mf);

static void        mount_point_added_cb                 (TrackerStorage       *storage,
                                                         const gchar          *uuid,
                                                         const gchar          *mount_point,
                                                         const gchar          *mount_name,
                                                         gboolean              removable,
                                                         gboolean              optical,
                                                         gpointer              user_data);
static void        mount_point_removed_cb               (TrackerStorage       *storage,
                                                         const gchar          *uuid,
                                                         const gchar          *mount_point,
                                                         gpointer              user_data);
#if defined(HAVE_UPOWER) || defined(HAVE_HAL)
static void        check_battery_status                 (TrackerMinerFiles    *fs);
static void        battery_status_cb                    (GObject              *object,
                                                         GParamSpec           *pspec,
                                                         gpointer              user_data);
static void        index_on_battery_cb                  (GObject    *object,
                                                         GParamSpec *pspec,
                                                         gpointer    user_data);
#endif /* defined(HAVE_UPOWER) || defined(HAVE_HAL) */
static void        init_mount_points                    (TrackerMinerFiles    *miner);
static void        init_stale_volume_removal            (TrackerMinerFiles    *miner);
static void        disk_space_check_start               (TrackerMinerFiles    *mf);
static void        disk_space_check_stop                (TrackerMinerFiles    *mf);
static void        low_disk_space_limit_cb              (GObject              *gobject,
                                                         GParamSpec           *arg1,
                                                         gpointer              user_data);
static void        index_recursive_directories_cb       (GObject              *gobject,
                                                         GParamSpec           *arg1,
                                                         gpointer              user_data);
static void        index_single_directories_cb          (GObject              *gobject,
                                                         GParamSpec           *arg1,
                                                         gpointer              user_data);
static gboolean    miner_files_force_recheck_idle       (gpointer user_data);
static void        trigger_recheck_cb                   (GObject              *gobject,
                                                         GParamSpec           *arg1,
                                                         gpointer              user_data);
static void        index_volumes_changed_cb             (GObject              *gobject,
                                                         GParamSpec           *arg1,
                                                         gpointer              user_data);
static void        set_up_application_indexing          (TrackerMinerFiles    *mf);
static void        index_applications_changed_cb        (GObject              *gobject,
                                                         GParamSpec           *arg1,
                                                         gpointer              user_data);
static gboolean    miner_files_process_file             (TrackerMinerFS       *fs,
                                                         GFile                *file,
                                                         GTask                *task);
static gboolean    miner_files_process_file_attributes  (TrackerMinerFS       *fs,
                                                         GFile                *file,
                                                         GTask                *task);
static gchar *     miner_files_remove_children          (TrackerMinerFS       *fs,
                                                         GFile                *file);
static gchar *     miner_files_remove_file              (TrackerMinerFS       *fs,
                                                         GFile                *file);
static gchar *     miner_files_move_file                (TrackerMinerFS       *fs,
                                                         GFile                *file,
                                                         GFile                *source_file,
                                                         gboolean              recursive);
static void        miner_files_finished                 (TrackerMinerFS       *fs,
                                                         gdouble               elapsed,
                                                         gint                  directories_found,
                                                         gint                  directories_ignored,
                                                         gint                  files_found,
                                                         gint                  files_ignored);
static void        miner_finished_cb                    (TrackerMinerFS *fs,
                                                         gdouble         seconds_elapsed,
                                                         guint           total_directories_found,
                                                         guint           total_directories_ignored,
                                                         guint           total_files_found,
                                                         guint           total_files_ignored,
                                                         gpointer        user_data);

static gboolean    miner_files_in_removable_media_remove_by_type  (TrackerMinerFiles  *miner,
                                                                   TrackerStorageType  type);
static void        miner_files_in_removable_media_remove_by_date  (TrackerMinerFiles  *miner,
                                                                   const gchar        *date);

static void        miner_files_add_removable_or_optical_directory (TrackerMinerFiles *mf,
                                                                   const gchar       *mount_path,
                                                                   const gchar       *uuid);

static void        miner_files_update_filters                     (TrackerMinerFiles *files);


static GInitableIface* miner_files_initable_parent_iface;

G_DEFINE_TYPE_WITH_CODE (TrackerMinerFiles, tracker_miner_files, TRACKER_TYPE_MINER_FS,
                         G_ADD_PRIVATE (TrackerMinerFiles)
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE,
                                                miner_files_initable_iface_init));

static void
tracker_miner_files_class_init (TrackerMinerFilesClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	TrackerMinerFSClass *miner_fs_class = TRACKER_MINER_FS_CLASS (klass);

	object_class->finalize = miner_files_finalize;
	object_class->get_property = miner_files_get_property;
	object_class->set_property = miner_files_set_property;

	miner_fs_class->process_file = miner_files_process_file;
	miner_fs_class->process_file_attributes = miner_files_process_file_attributes;
	miner_fs_class->finished = miner_files_finished;
	miner_fs_class->remove_file = miner_files_remove_file;
	miner_fs_class->remove_children = miner_files_remove_children;
	miner_fs_class->move_file = miner_files_move_file;

	g_object_class_install_property (object_class,
	                                 PROP_CONFIG,
	                                 g_param_spec_object ("config",
	                                                      "Config",
	                                                      "Config",
	                                                      TRACKER_TYPE_CONFIG,
	                                                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (object_class,
	                                 PROP_DOMAIN,
	                                 g_param_spec_string ("domain",
	                                                      "Domain",
	                                                      "Domain",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

	miner_files_error_quark = g_quark_from_static_string ("TrackerMinerFiles");
}

static void
tracker_miner_files_check_unextracted (TrackerMinerFiles *mf)
{
	if (!mf->private->start_extractor)
		return;

	mf->private->start_extractor = FALSE;
	g_debug ("Starting extractor");
	tracker_extract_watchdog_ensure_started (mf->private->extract_watchdog);
}

static gboolean
extractor_lost_timeout_cb (gpointer user_data)
{
	TrackerMinerFiles *mf = user_data;

	tracker_miner_files_check_unextracted (mf);
	mf->private->grace_period_timeout_id = 0;
	return G_SOURCE_REMOVE;
}


static void
on_extractor_lost (TrackerExtractWatchdog *watchdog,
                   TrackerMinerFiles      *mf)
{
	g_debug ("tracker-extract vanished, maybe restarting.");

	/* Give a period of grace before restarting, so we allow replacing
	 * from eg. a terminal.
	 */
	mf->private->grace_period_timeout_id =
		g_timeout_add_seconds (1, extractor_lost_timeout_cb, mf);
}

static void
tracker_miner_files_init (TrackerMinerFiles *mf)
{
	TrackerMinerFilesPrivate *priv;
	gchar *rdf_types_str;
	GStrv rdf_types;

	priv = mf->private = TRACKER_MINER_FILES_GET_PRIVATE (mf);

	priv->storage = tracker_storage_new ();

	g_signal_connect (priv->storage, "mount-point-added",
	                  G_CALLBACK (mount_point_added_cb),
	                  mf);

	g_signal_connect (priv->storage, "mount-point-removed",
	                  G_CALLBACK (mount_point_removed_cb),
	                  mf);

#if defined(HAVE_UPOWER) || defined(HAVE_HAL)
	priv->power = tracker_power_new ();

	if (priv->power) {
		g_signal_connect (priv->power, "notify::on-low-battery",
		                  G_CALLBACK (battery_status_cb),
		                  mf);
		g_signal_connect (priv->power, "notify::on-battery",
		                  G_CALLBACK (battery_status_cb),
		                  mf);
	}
#endif /* defined(HAVE_UPOWER) || defined(HAVE_HAL) */

	priv->finished_handler = g_signal_connect_after (mf, "finished",
	                                                 G_CALLBACK (miner_finished_cb),
	                                                 NULL);

	priv->volume_monitor = g_volume_monitor_get ();
	g_signal_connect (priv->volume_monitor, "mount-pre-unmount",
	                  G_CALLBACK (mount_pre_unmount_cb),
	                  mf);

	priv->mtime_check = TRUE;
	priv->quark_mount_point_uuid = g_quark_from_static_string ("tracker-mount-point-uuid");

	rdf_types = tracker_extract_module_manager_get_all_rdf_types ();
	rdf_types_str = g_strjoinv (",", rdf_types);
	g_strfreev (rdf_types);

	g_free (rdf_types_str);
}

static void
miner_files_initable_iface_init (GInitableIface *iface)
{
	miner_files_initable_parent_iface = g_type_interface_peek_parent (iface);
	iface->init = miner_files_initable_init;
}

static gboolean
miner_files_initable_init (GInitable     *initable,
                           GCancellable  *cancellable,
                           GError       **error)
{
	TrackerMinerFiles *mf;
	TrackerMinerFS *fs;
	TrackerIndexingTree *indexing_tree;
	TrackerDirectoryFlags flags;
	GError *inner_error = NULL;
	GSList *mounts = NULL;
	GSList *dirs;
	GSList *m;

	/* Chain up parent's initable callback before calling child's one */
	if (!miner_files_initable_parent_iface->init (initable, cancellable, &inner_error)) {
		g_propagate_error (error, inner_error);
		return FALSE;
	}

	mf = TRACKER_MINER_FILES (initable);
	fs = TRACKER_MINER_FS (initable);
	indexing_tree = tracker_miner_fs_get_indexing_tree (fs);
	tracker_indexing_tree_set_filter_hidden (indexing_tree, TRUE);

	miner_files_update_filters (mf);

	mf->private->domain_ontology = tracker_domain_ontology_new (mf->private->domain, NULL, &inner_error);
	if (!mf->private->domain_ontology) {
		g_propagate_error (error, inner_error);
		return FALSE;
	}

	/* Set up extractor and signals */
	mf->private->connection =  g_bus_get_sync (TRACKER_IPC_BUS, NULL, &inner_error);
	if (!mf->private->connection) {
		g_propagate_error (error, inner_error);
		g_prefix_error (error,
		                "Could not connect to the D-Bus session bus. ");
		return FALSE;
	}

	/* We must have a configuration setup here */
	if (G_UNLIKELY (!mf->private->config)) {
		g_set_error (error,
		             TRACKER_MINER_ERROR,
		             0,
		             "No config set for miner %s",
		             G_OBJECT_TYPE_NAME (mf));
		return FALSE;
	}

	/* Setup mount points, we MUST have config set up before we
	 * init mount points because the config is used in that
	 * function.
	 */
	mf->private->index_removable_devices = tracker_config_get_index_removable_devices (mf->private->config);

	/* Note that if removable devices not indexed, optical discs
	 * will also never be indexed */
	mf->private->index_optical_discs = (mf->private->index_removable_devices ?
	                                    tracker_config_get_index_optical_discs (mf->private->config) :
	                                    FALSE);

	init_mount_points (mf);

	/* If this happened AFTER we have initialized mount points, initialize
	 * stale volume removal now. */
	if (mf->private->mount_points_initialized) {
		init_stale_volume_removal (mf);
	}

	if (mf->private->index_removable_devices) {
		/* Get list of roots for removable devices (excluding optical) */
		mounts = tracker_storage_get_device_roots (mf->private->storage,
		                                           TRACKER_STORAGE_REMOVABLE,
		                                           TRUE);
	}

	if (mf->private->index_optical_discs) {
		/* Get list of roots for removable+optical devices */
		m = tracker_storage_get_device_roots (mf->private->storage,
		                                      TRACKER_STORAGE_OPTICAL | TRACKER_STORAGE_REMOVABLE,
		                                      TRUE);
		mounts = g_slist_concat (mounts, m);
	}

#if defined(HAVE_UPOWER) || defined(HAVE_HAL)
	check_battery_status (mf);
#endif /* defined(HAVE_UPOWER) || defined(HAVE_HAL) */

	TRACKER_NOTE (CONFIG, g_message ("Setting up directories to iterate from config (IndexSingleDirectory)"));

	/* Fill in directories to inspect */
	dirs = tracker_config_get_index_single_directories (mf->private->config);

	/* Copy in case of config changes */
	mf->private->index_single_directories = tracker_gslist_copy_with_string_data (dirs);

	for (; dirs; dirs = dirs->next) {
		GFile *file;

		/* Do some simple checks for silly locations */
		if (strcmp (dirs->data, "/dev") == 0 ||
		    strcmp (dirs->data, "/lib") == 0 ||
		    strcmp (dirs->data, "/proc") == 0 ||
		    strcmp (dirs->data, "/sys") == 0) {
			continue;
		}

		if (g_str_has_prefix (dirs->data, g_get_tmp_dir ())) {
			continue;
		}

		/* Make sure we don't crawl volumes. */
		if (mounts) {
			gboolean found = FALSE;

			for (m = mounts; m && !found; m = m->next) {
				found = strcmp (m->data, dirs->data) == 0;
			}

			if (found) {
				g_debug ("  Duplicate found:'%s' - same as removable device path",
				           (gchar*) dirs->data);
				continue;
			}
		}

		g_debug ("  Adding:'%s'", (gchar*) dirs->data);

		file = g_file_new_for_path (dirs->data);

		flags = TRACKER_DIRECTORY_FLAG_NONE;

		if (tracker_config_get_enable_monitors (mf->private->config)) {
			flags |= TRACKER_DIRECTORY_FLAG_MONITOR;
		}

		if (mf->private->mtime_check) {
			flags |= TRACKER_DIRECTORY_FLAG_CHECK_MTIME;
		}

		tracker_indexing_tree_add (indexing_tree, file, flags);
		g_object_unref (file);
	}

	TRACKER_NOTE (CONFIG, g_message ("Setting up directories to iterate from config (IndexRecursiveDirectory)"));

	dirs = tracker_config_get_index_recursive_directories (mf->private->config);

	/* Copy in case of config changes */
	mf->private->index_recursive_directories = tracker_gslist_copy_with_string_data (dirs);

	for (; dirs; dirs = dirs->next) {
		GFile *file;

		/* Do some simple checks for silly locations */
		if (strcmp (dirs->data, "/dev") == 0 ||
		    strcmp (dirs->data, "/lib") == 0 ||
		    strcmp (dirs->data, "/proc") == 0 ||
		    strcmp (dirs->data, "/sys") == 0) {
			continue;
		}

		if (g_str_has_prefix (dirs->data, g_get_tmp_dir ())) {
			continue;
		}

		/* Make sure we don't crawl volumes. */
		if (mounts) {
			gboolean found = FALSE;

			for (m = mounts; m && !found; m = m->next) {
				found = strcmp (m->data, dirs->data) == 0;
			}

			if (found) {
				g_debug ("  Duplicate found:'%s' - same as removable device path",
				           (gchar*) dirs->data);
				continue;
			}
		}

		g_debug ("  Adding:'%s'", (gchar*) dirs->data);

		file = g_file_new_for_path (dirs->data);

		flags = TRACKER_DIRECTORY_FLAG_RECURSE;

		if (tracker_config_get_enable_monitors (mf->private->config)) {
			flags |= TRACKER_DIRECTORY_FLAG_MONITOR;
		}

		if (mf->private->mtime_check) {
			flags |= TRACKER_DIRECTORY_FLAG_CHECK_MTIME;
		}

		tracker_indexing_tree_add (indexing_tree, file, flags);
		g_object_unref (file);
	}

	/* Add mounts */
	TRACKER_NOTE (CONFIG, g_message ("Setting up directories to iterate from devices/discs"));

	if (!mf->private->index_removable_devices) {
		TRACKER_NOTE (CONFIG, g_message ("  Removable devices are disabled in the config"));

		/* Make sure we don't have any resource in a volume of the given type */
		miner_files_in_removable_media_remove_by_type (mf, TRACKER_STORAGE_REMOVABLE);
	}

	if (!mf->private->index_optical_discs) {
		TRACKER_NOTE (CONFIG, g_message ("  Optical discs are disabled in the config"));

		/* Make sure we don't have any resource in a volume of the given type */
		miner_files_in_removable_media_remove_by_type (mf, TRACKER_STORAGE_REMOVABLE | TRACKER_STORAGE_OPTICAL);
	}

	for (m = mounts; m; m = m->next) {
		miner_files_add_removable_or_optical_directory (mf,
		                                                (gchar *) m->data,
		                                                NULL);
	}

	/* Initialize application indexing */
	set_up_application_indexing (mf);

	/* We want to get notified when config changes */

	g_signal_connect (mf->private->config, "notify::low-disk-space-limit",
	                  G_CALLBACK (low_disk_space_limit_cb),
	                  mf);
	g_signal_connect (mf->private->config, "notify::index-recursive-directories",
	                  G_CALLBACK (index_recursive_directories_cb),
	                  mf);
	g_signal_connect (mf->private->config, "notify::index-single-directories",
	                  G_CALLBACK (index_single_directories_cb),
	                  mf);
	g_signal_connect (mf->private->config, "notify::ignored-directories",
	                  G_CALLBACK (trigger_recheck_cb),
	                  mf);
	g_signal_connect (mf->private->config, "notify::ignored-directories-with-content",
	                  G_CALLBACK (trigger_recheck_cb),
	                  mf);
	g_signal_connect (mf->private->config, "notify::ignored-files",
	                  G_CALLBACK (trigger_recheck_cb),
	                  mf);
	g_signal_connect (mf->private->config, "notify::enable-monitors",
	                  G_CALLBACK (trigger_recheck_cb),
	                  mf);
	g_signal_connect (mf->private->config, "notify::index-removable-devices",
	                  G_CALLBACK (index_volumes_changed_cb),
	                  mf);
	g_signal_connect (mf->private->config, "notify::index-optical-discs",
	                  G_CALLBACK (index_volumes_changed_cb),
	                  mf);
	g_signal_connect (mf->private->config, "notify::index-applications",
	                  G_CALLBACK (index_applications_changed_cb),
	                  mf);
	g_signal_connect (mf->private->config, "notify::removable-days-threshold",
	                  G_CALLBACK (index_volumes_changed_cb),
	                  mf);

#if defined(HAVE_UPOWER) || defined(HAVE_HAL)

	g_signal_connect (mf->private->config, "notify::index-on-battery",
	                  G_CALLBACK (index_on_battery_cb),
	                  mf);
	g_signal_connect (mf->private->config, "notify::index-on-battery-first-time",
	                  G_CALLBACK (index_on_battery_cb),
	                  mf);

#endif /* defined(HAVE_UPOWER) || defined(HAVE_HAL) */

	g_slist_foreach (mounts, (GFunc) g_free, NULL);
	g_slist_free (mounts);

	disk_space_check_start (mf);

	mf->private->extract_watchdog = tracker_extract_watchdog_new (mf->private->domain);
	g_signal_connect (mf->private->extract_watchdog, "lost",
	                  G_CALLBACK (on_extractor_lost), mf);

	return TRUE;
}

static void
miner_files_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
	TrackerMinerFilesPrivate *priv;

	priv = TRACKER_MINER_FILES_GET_PRIVATE (object);

	switch (prop_id) {
	case PROP_CONFIG:
		priv->config = g_value_dup_object (value);
		break;
	case PROP_DOMAIN:
		priv->domain = g_value_dup_string (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
miner_files_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
	TrackerMinerFilesPrivate *priv;

	priv = TRACKER_MINER_FILES_GET_PRIVATE (object);

	switch (prop_id) {
	case PROP_CONFIG:
		g_value_set_object (value, priv->config);
		break;
	case PROP_DOMAIN:
		g_value_set_string (value, priv->domain);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
miner_files_finalize (GObject *object)
{
	TrackerMinerFiles *mf;
	TrackerMinerFilesPrivate *priv;

	mf = TRACKER_MINER_FILES (object);
	priv = mf->private;

	g_free (priv->domain);

	if (priv->grace_period_timeout_id != 0) {
		g_source_remove (priv->grace_period_timeout_id);
		priv->grace_period_timeout_id = 0;
	}

	g_signal_handlers_disconnect_by_func (priv->extract_watchdog,
	                                      on_extractor_lost,
	                                      NULL);
	g_clear_object (&priv->extract_watchdog);

	if (priv->config) {
		g_signal_handlers_disconnect_by_func (priv->config,
		                                      low_disk_space_limit_cb,
		                                      NULL);
		g_object_unref (priv->config);
	}

	disk_space_check_stop (TRACKER_MINER_FILES (object));

	g_slist_free_full (mf->private->application_dirs, g_object_unref);

	if (priv->index_recursive_directories) {
		g_slist_foreach (priv->index_recursive_directories, (GFunc) g_free, NULL);
		g_slist_free (priv->index_recursive_directories);
	}

	if (priv->index_single_directories) {
		g_slist_foreach (priv->index_single_directories, (GFunc) g_free, NULL);
		g_slist_free (priv->index_single_directories);
	}

#if defined(HAVE_UPOWER) || defined(HAVE_HAL)
	if (priv->power) {
		g_object_unref (priv->power);
	}
#endif /* defined(HAVE_UPOWER) || defined(HAVE_HAL) */

	tracker_domain_ontology_unref (priv->domain_ontology);

	if (priv->storage) {
		g_object_unref (priv->storage);
	}

	if (priv->volume_monitor) {
		g_signal_handlers_disconnect_by_func (priv->volume_monitor,
		                                      mount_pre_unmount_cb,
		                                      object);
		g_object_unref (priv->volume_monitor);
	}

	if (priv->force_recheck_id) {
		g_source_remove (priv->force_recheck_id);
		priv->force_recheck_id = 0;
	}

	if (priv->stale_volumes_check_id) {
		g_source_remove (priv->stale_volumes_check_id);
		priv->stale_volumes_check_id = 0;
	}

	g_list_free (priv->extraction_queue);

	G_OBJECT_CLASS (tracker_miner_files_parent_class)->finalize (object);
}

static void
set_up_mount_point_cb (GObject      *source,
                       GAsyncResult *result,
                       gpointer      user_data)
{
	TrackerSparqlConnection *connection = TRACKER_SPARQL_CONNECTION (source);
	GError *error = NULL;

	tracker_sparql_connection_update_finish (connection, result, &error);

	if (error) {
		g_critical ("Could not set mount point in database, %s",
		            error->message);
		g_error_free (error);
	}
}

static void
set_up_mount_point (TrackerMinerFiles *miner,
                    GFile             *mount_point,
                    gboolean           mounted,
                    GString           *accumulator)
{
	GString *queries;
	gchar *uri;

	queries = g_string_new (NULL);
	uri = g_file_get_uri (mount_point);

	if (mounted) {
		g_debug ("Mount point state (MOUNTED) being set in DB for mount_point '%s'",
		         uri);

		g_string_append_printf (queries,
		                        "DELETE { ?u tracker:unmountDate ?d } "
		                        "WHERE { <%s> a nfo:FileDataObject/"
		                        "               nie:interpretedAs/"
		                        "               nie:rootElementOf ?u"
		                        "}",
		                        uri);
	} else {
		gchar *now;

		g_debug ("Mount point state (UNMOUNTED) being set in DB for URI '%s'",
		         uri);

		now = tracker_date_to_string (time (NULL));

		g_string_append_printf (queries,
		                        "DELETE { ?u tracker:unmountDate ?unknown1 ;"
		                        "            tracker:available ?unknown2 } "
		                        "INSERT { ?u tracker:unmountDate \"%s\" } "
		                        "WHERE { <%s> a nfo:FileDataObject/"
		                        "               nie:interpretedAs/"
		                        "               nie:rootElementOf ?u"
		                        "}",
		                        now, uri);

		g_free (now);
	}

	g_free (uri);

	if (accumulator) {
		g_string_append_printf (accumulator, "%s ", queries->str);
	} else {
		tracker_sparql_connection_update_async (tracker_miner_get_connection (TRACKER_MINER (miner)),
		                                        queries->str,
		                                        G_PRIORITY_LOW,
		                                        NULL,
		                                        set_up_mount_point_cb,
		                                        NULL);
	}

	g_string_free (queries, TRUE);
}

static void
init_mount_points_cb (GObject      *source,
                      GAsyncResult *result,
                      gpointer      user_data)
{
	GError *error = NULL;

	tracker_sparql_connection_update_finish (TRACKER_SPARQL_CONNECTION (source),
	                                         result,
	                                         &error);

	if (error) {
		g_critical ("Could not initialize currently active mount points: %s",
		            error->message);
		g_error_free (error);
	} else {
		/* Mount points correctly initialized */
		(TRACKER_MINER_FILES (user_data))->private->mount_points_initialized = TRUE;
		/* If this happened AFTER we have a proper config, initialize
		 * stale volume removal now. */
		if ((TRACKER_MINER_FILES (user_data))->private->config) {
			init_stale_volume_removal (TRACKER_MINER_FILES (user_data));
		}
	}
}

static void
init_mount_points (TrackerMinerFiles *miner_files)
{
	TrackerMiner *miner = TRACKER_MINER (miner_files);
	TrackerMinerFilesPrivate *priv;
	GHashTable *volumes;
	GHashTableIter iter;
	gpointer key, value;
	GString *accumulator;
	GError *error = NULL;
	TrackerSparqlCursor *cursor;
	GSList *mounts, *l;

	g_debug ("Initializing mount points...");

	/* First, get all mounted volumes, according to tracker-store (SYNC!) */
	cursor = tracker_sparql_connection_query (tracker_miner_get_connection (miner),
	                                          "SELECT ?f WHERE { "
	                                          "  ?v a tracker:IndexedFolder ; "
	                                          "     tracker:isRemovable true; "
	                                          "     tracker:available true . "
	                                          "  ?f a nfo:FileDataObject ; "
	                                          "     nie:interpretedAs/nie:rootElementOf ?v . "
	                                          "}",
	                                          NULL, &error);
	if (error) {
		g_critical ("Could not obtain the mounted volumes: %s", error->message);
		g_error_free (error);
		return;
	}

	priv = TRACKER_MINER_FILES_GET_PRIVATE (miner);

	volumes = g_hash_table_new_full (g_str_hash, g_str_equal,
	                                 (GDestroyNotify) g_free,
	                                 NULL);

	while (tracker_sparql_cursor_next (cursor, NULL, NULL)) {
		gint state;
		const gchar *urn;

		state = VOLUME_MOUNTED_IN_STORE;

		urn = tracker_sparql_cursor_get_string (cursor, 0, NULL);

		if (!urn)
			continue;

		if (strcmp (urn, TRACKER_DATASOURCE_URN_NON_REMOVABLE_MEDIA) == 0) {
			/* Report non-removable media to be mounted by HAL as well */
			state |= VOLUME_MOUNTED;
		}

		g_hash_table_replace (volumes, g_strdup (urn), GINT_TO_POINTER (state));
	}

	g_object_unref (cursor);

	/* Then, get all currently mounted non-REMOVABLE volumes, according to GIO */
	mounts = tracker_storage_get_device_uuids (priv->storage, 0, TRUE);
	for (l = mounts; l; l = l->next) {
		gchar *path;
		gint state;

		path = g_strdup (l->data);
		state = GPOINTER_TO_INT (g_hash_table_lookup (volumes, path));
		state |= VOLUME_MOUNTED;

		g_hash_table_replace (volumes, path, GINT_TO_POINTER (state));
	}

	g_slist_foreach (mounts, (GFunc) g_free, NULL);
	g_slist_free (mounts);

	/* Then, get all currently mounted REMOVABLE volumes, according to GIO */
	if (priv->index_removable_devices) {
		mounts = tracker_storage_get_device_roots (priv->storage, TRACKER_STORAGE_REMOVABLE, FALSE);
		for (l = mounts; l; l = l->next) {
			gchar *path;
			gint state;

			path = g_strdup (l->data);

			state = GPOINTER_TO_INT (g_hash_table_lookup (volumes, path));
			state |= VOLUME_MOUNTED;

			g_hash_table_replace (volumes, path, GINT_TO_POINTER (state));
		}

		g_slist_foreach (mounts, (GFunc) g_free, NULL);
		g_slist_free (mounts);
	}

	accumulator = g_string_new (NULL);
	g_hash_table_iter_init (&iter, volumes);

	/* Finally, set up volumes based on the composed info */
	while (g_hash_table_iter_next (&iter, &key, &value)) {
		const gchar *mount_point = key;
		gint state = GPOINTER_TO_INT (value);
		GFile *file = g_file_new_for_path (mount_point);

		if ((state & VOLUME_MOUNTED) &&
		    !(state & VOLUME_MOUNTED_IN_STORE)) {
			g_debug ("Mount point state incorrect in DB for mount '%s', "
			         "currently it is mounted",
			         mount_point);

			/* Set mount point state */
			set_up_mount_point (TRACKER_MINER_FILES (miner),
			                    file,
			                    TRUE,
			                    accumulator);

			if (mount_point) {
				TrackerIndexingTree *indexing_tree;
				TrackerDirectoryFlags flags;

				indexing_tree = tracker_miner_fs_get_indexing_tree (TRACKER_MINER_FS (miner));
				flags = TRACKER_DIRECTORY_FLAG_RECURSE |
					TRACKER_DIRECTORY_FLAG_CHECK_MTIME |
					TRACKER_DIRECTORY_FLAG_PRESERVE;

				if (tracker_config_get_enable_monitors (miner_files->private->config)) {
					flags |= TRACKER_DIRECTORY_FLAG_MONITOR;
				}

				/* Add the current mount point as reported to have incorrect
				 * state. We will force mtime checks on this mount points,
				 * even if no-mtime-check-needed was set. */
				if (tracker_miner_files_is_file_eligible (miner_files, file)) {
					tracker_indexing_tree_add (indexing_tree,
					                           file,
					                           flags);
				}
			}
		} else if (!(state & VOLUME_MOUNTED) &&
		           (state & VOLUME_MOUNTED_IN_STORE)) {
			g_debug ("Mount point state incorrect in DB for mount '%s', "
			         "currently it is NOT mounted",
			         mount_point);
			set_up_mount_point (TRACKER_MINER_FILES (miner),
			                    file,
			                    FALSE,
			                    accumulator);
			/* There's no need to force mtime check in these inconsistent
			 * mount points, as they are not mounted right now. */
		}

		g_object_unref (file);
	}

	if (accumulator->str[0] != '\0') {
		tracker_sparql_connection_update_async (tracker_miner_get_connection (miner),
		                                        accumulator->str,
		                                        G_PRIORITY_LOW,
		                                        NULL,
		                                        init_mount_points_cb,
		                                        miner);
	} else {
		/* Note. Not initializing stale volume removal timeout because
		 * we do not have the configuration setup yet */
		(TRACKER_MINER_FILES (miner))->private->mount_points_initialized = TRUE;
	}

	g_string_free (accumulator, TRUE);
	g_hash_table_unref (volumes);
}

static gboolean
cleanup_stale_removable_volumes_cb (gpointer user_data)
{
	TrackerMinerFiles *miner = TRACKER_MINER_FILES (user_data);
	gint n_days_threshold;
	time_t n_days_ago;
	gchar *n_days_ago_as_string;

	n_days_threshold = tracker_config_get_removable_days_threshold (miner->private->config);

	if (n_days_threshold == 0)
		return TRUE;

	n_days_ago = (time (NULL) - (SECONDS_PER_DAY * n_days_threshold));
	n_days_ago_as_string = tracker_date_to_string (n_days_ago);

	g_debug ("Running stale volumes check...");

	miner_files_in_removable_media_remove_by_date (miner, n_days_ago_as_string);

	g_free (n_days_ago_as_string);

	return TRUE;
}

static void
init_stale_volume_removal (TrackerMinerFiles *miner)
{
	/* If disabled, make sure we don't do anything */
	if (tracker_config_get_removable_days_threshold (miner->private->config) == 0) {
		g_debug ("Stale volume check is disabled");
		return;
	}

	/* Run right away the first check */
	cleanup_stale_removable_volumes_cb (miner);

	g_debug ("Initializing stale volume check timeout...");

	/* Then, setup new timeout event every day */
	miner->private->stale_volumes_check_id =
		g_timeout_add_seconds (SECONDS_PER_DAY + 1,
		                       cleanup_stale_removable_volumes_cb,
		                       miner);
}


static void
mount_point_removed_cb (TrackerStorage *storage,
                        const gchar    *uuid,
                        const gchar    *mount_point,
                        gpointer        user_data)
{
	TrackerMinerFiles *miner = user_data;
	TrackerIndexingTree *indexing_tree;
	GFile *mount_point_file;

	g_debug ("Mount point removed for path '%s'", mount_point);

	mount_point_file = g_file_new_for_path (mount_point);

	/* Tell TrackerMinerFS to skip monitoring everything under the mount
	 *  point (in case there was no pre-unmount notification) */
	indexing_tree = tracker_miner_fs_get_indexing_tree (TRACKER_MINER_FS (miner));
	tracker_indexing_tree_remove (indexing_tree, mount_point_file);

	/* Set mount point status in tracker-store */
	set_up_mount_point (miner, mount_point_file, FALSE, NULL);

	g_object_unref (mount_point_file);
}

static void
mount_point_added_cb (TrackerStorage *storage,
                      const gchar    *uuid,
                      const gchar    *mount_point,
                      const gchar    *mount_name,
                      gboolean        removable,
                      gboolean        optical,
                      gpointer        user_data)
{
	TrackerMinerFiles *miner = user_data;
	TrackerMinerFilesPrivate *priv;
	GFile *mount_point_file;

	priv = TRACKER_MINER_FILES_GET_PRIVATE (miner);

	g_debug ("Mount point added for path '%s'", mount_point);
	mount_point_file = g_file_new_for_path (mount_point);

	if (removable && !priv->index_removable_devices) {
		g_debug ("  Not crawling, removable devices disabled in config");
	} else if (optical && !priv->index_optical_discs) {
		g_debug ("  Not crawling, optical devices discs disabled in config");
	} else if (!removable && !optical) {
		TrackerIndexingTree *indexing_tree;
		TrackerDirectoryFlags flags;
		GSList *l;

		indexing_tree = tracker_miner_fs_get_indexing_tree (TRACKER_MINER_FS (miner));

		/* Check if one of the recursively indexed locations is in
		 *   the mounted path, or if the mounted path is inside
		 *   a recursively indexed directory... */
		for (l = tracker_config_get_index_recursive_directories (miner->private->config);
		     l;
		     l = g_slist_next (l)) {
			GFile *config_file;

			config_file = g_file_new_for_path (l->data);
			flags = TRACKER_DIRECTORY_FLAG_RECURSE |
				TRACKER_DIRECTORY_FLAG_CHECK_MTIME |
				TRACKER_DIRECTORY_FLAG_PRESERVE;

			if (tracker_config_get_enable_monitors (miner->private->config)) {
				flags |= TRACKER_DIRECTORY_FLAG_MONITOR;
			}

			if (g_file_equal (config_file, mount_point_file) ||
			    g_file_has_prefix (config_file, mount_point_file)) {
				/* If the config path is contained inside the mount path,
				 *  then add the config path to re-check */
				g_debug ("  Re-check of configured path '%s' needed (recursively)",
				         (gchar *) l->data);
				tracker_indexing_tree_add (indexing_tree,
							   config_file,
							   flags);
			} else if (g_file_has_prefix (mount_point_file, config_file)) {
				/* If the mount path is contained inside the config path,
				 *  then add the mount path to re-check */
				g_debug ("  Re-check of path '%s' needed (inside configured path '%s')",
				         mount_point,
				         (gchar *) l->data);
				tracker_indexing_tree_add (indexing_tree,
							   config_file,
							   flags);
			}
			g_object_unref (config_file);
		}

		/* Check if one of the non-recursively indexed locations is in
		 *  the mount path... */
		for (l = tracker_config_get_index_single_directories (miner->private->config);
		     l;
		     l = g_slist_next (l)) {
			GFile *config_file;

			flags = TRACKER_DIRECTORY_FLAG_CHECK_MTIME;

			if (tracker_config_get_enable_monitors (miner->private->config)) {
				flags |= TRACKER_DIRECTORY_FLAG_MONITOR;
			}

			config_file = g_file_new_for_path (l->data);
			if (g_file_equal (config_file, mount_point_file) ||
			    g_file_has_prefix (config_file, mount_point_file)) {
				g_debug ("  Re-check of configured path '%s' needed (non-recursively)",
				         (gchar *) l->data);
				tracker_indexing_tree_add (indexing_tree,
							   config_file,
							   flags);
			}
			g_object_unref (config_file);
		}
	} else {
		g_debug ("  Adding directories in removable/optical media to crawler's queue");
		miner_files_add_removable_or_optical_directory (miner,
		                                                mount_point,
		                                                uuid);
	}

	set_up_mount_point (miner, mount_point_file, TRUE, NULL);
	g_object_unref (mount_point_file);
}

#if defined(HAVE_UPOWER) || defined(HAVE_HAL)

static void
set_up_throttle (TrackerMinerFiles *mf,
                 gboolean           enable)
{
	gdouble throttle;
	gint config_throttle;

	config_throttle = tracker_config_get_throttle (mf->private->config);
	throttle = (1.0 / 20) * config_throttle;

	if (enable) {
		throttle += 0.25;
	}

	throttle = CLAMP (throttle, 0, 1);

	g_debug ("Setting new throttle to %0.3f", throttle);
	tracker_miner_fs_set_throttle (TRACKER_MINER_FS (mf), throttle);
}

static void
check_battery_status (TrackerMinerFiles *mf)
{
	gboolean on_battery, on_low_battery;
	gboolean should_pause = FALSE;
	gboolean should_throttle = FALSE;

	if (mf->private->power == NULL) {
		return;
	}

	on_low_battery = tracker_power_get_on_low_battery (mf->private->power);
	on_battery = tracker_power_get_on_battery (mf->private->power);

	if (!on_battery) {
		g_message ("Running on AC power");
		should_pause = FALSE;
		should_throttle = FALSE;
	} else if (on_low_battery) {
		g_message ("Running on LOW Battery, pausing");
		should_pause = TRUE;
		should_throttle = TRUE;
	} else {
		should_throttle = TRUE;

		/* Check if miner should be paused based on configuration */
		if (!tracker_config_get_index_on_battery (mf->private->config)) {
			if (!tracker_config_get_index_on_battery_first_time (mf->private->config)) {
				g_message ("Running on battery, but not enabled, pausing");
				should_pause = TRUE;
			} else if (tracker_miner_files_get_first_index_done (mf)) {
				g_message ("Running on battery and first-time index "
				           "already done, pausing");
				should_pause = TRUE;
			} else {
				g_message ("Running on battery, but first-time index not "
				           "already finished, keeping on");
			}
		} else {
			g_message ("Running on battery");
		}
	}

	if (should_pause) {
		/* Don't try to pause again */
		if (!mf->private->low_battery_pause) {
			mf->private->low_battery_pause = TRUE;
			tracker_miner_pause (TRACKER_MINER (mf));
		}
	} else {
		/* Don't try to resume again */
		if (mf->private->low_battery_pause) {
			tracker_miner_resume (TRACKER_MINER (mf));
			mf->private->low_battery_pause = FALSE;
		}
	}

	set_up_throttle (mf, should_throttle);
}

/* Called when battery status change is detected */
static void
battery_status_cb (GObject    *object,
                   GParamSpec *pspec,
                   gpointer    user_data)
{
	TrackerMinerFiles *mf = user_data;

	check_battery_status (mf);
}

/* Called when battery-related configuration change is detected */
static void
index_on_battery_cb (GObject    *object,
                     GParamSpec *pspec,
                     gpointer    user_data)
{
	TrackerMinerFiles *mf = user_data;

	check_battery_status (mf);
}

#endif /* defined(HAVE_UPOWER) || defined(HAVE_HAL) */

/* Called when mining has finished the first time */
static void
miner_finished_cb (TrackerMinerFS *fs,
                   gdouble         seconds_elapsed,
                   guint           total_directories_found,
                   guint           total_directories_ignored,
                   guint           total_files_found,
                   guint           total_files_ignored,
                   gpointer        user_data)
{
	TrackerMinerFiles *mf = TRACKER_MINER_FILES (fs);

	/* Create stamp file if not already there */
	if (!tracker_miner_files_get_first_index_done (mf)) {
		tracker_miner_files_set_first_index_done (mf, TRUE);
	}

	/* And remove the signal handler so that it's not
	 *  called again */
	if (mf->private->finished_handler) {
		g_signal_handler_disconnect (fs, mf->private->finished_handler);
		mf->private->finished_handler = 0;
	}

#if defined(HAVE_UPOWER) || defined(HAVE_HAL)
	check_battery_status (mf);
#endif /* defined(HAVE_UPOWER) || defined(HAVE_HAL) */
}

static void
mount_pre_unmount_cb (GVolumeMonitor    *volume_monitor,
                      GMount            *mount,
                      TrackerMinerFiles *mf)
{
	TrackerIndexingTree *indexing_tree;
	GFile *mount_root;
	gchar *uri;

	mount_root = g_mount_get_root (mount);
	uri = g_file_get_uri (mount_root);
	g_debug ("Pre-unmount requested for '%s'", uri);

	indexing_tree = tracker_miner_fs_get_indexing_tree (TRACKER_MINER_FS (mf));
	tracker_indexing_tree_remove (indexing_tree, mount_root);
	g_object_unref (mount_root);

	g_free (uri);
}

static GFile *
get_cache_dir (TrackerMinerFiles *mf)
{
	GFile *cache;

	cache = tracker_domain_ontology_get_cache (mf->private->domain_ontology);
	return g_file_get_child (cache, "files");
}


static gboolean
disk_space_check (TrackerMinerFiles *mf)
{
	GFile *file;
	gint limit;
	gchar *data_dir;
	gdouble remaining;

	limit = tracker_config_get_low_disk_space_limit (mf->private->config);

	if (limit < 1) {
		return FALSE;
	}

	/* Get % of remaining space in the partition where the cache is */
	file = get_cache_dir (mf);
	data_dir = g_file_get_path (file);
	remaining = tracker_file_system_get_remaining_space_percentage (data_dir);
	g_free (data_dir);
	g_object_unref (file);

	if (remaining <= limit) {
		g_message ("WARNING: Available disk space (%lf%%) is below "
		           "configured threshold for acceptable working (%d%%)",
		           remaining, limit);
		return TRUE;
	}

	return FALSE;
}

static gboolean
disk_space_check_cb (gpointer user_data)
{
	TrackerMinerFiles *mf = user_data;

	if (disk_space_check (mf)) {
		/* Don't try to pause again */
		if (!mf->private->disk_space_pause) {
			mf->private->disk_space_pause = TRUE;
			tracker_miner_pause (TRACKER_MINER (mf));
		}
	} else {
		/* Don't try to resume again */
		if (mf->private->disk_space_pause) {
			tracker_miner_resume (TRACKER_MINER (mf));
			mf->private->disk_space_pause = FALSE;
		}
	}

	return TRUE;
}

static void
disk_space_check_start (TrackerMinerFiles *mf)
{
	gint limit;

	if (mf->private->disk_space_check_id != 0) {
		return;
	}

	limit = tracker_config_get_low_disk_space_limit (mf->private->config);

	if (limit != -1) {
		TRACKER_NOTE (CONFIG, g_message ("Starting disk space check for every %d seconds",
		                      DISK_SPACE_CHECK_FREQUENCY));
		mf->private->disk_space_check_id =
			g_timeout_add_seconds (DISK_SPACE_CHECK_FREQUENCY,
			                       disk_space_check_cb,
			                       mf);

		/* Call the function now too to make sure we have an
		 * initial value too!
		 */
		disk_space_check_cb (mf);
	} else {
		TRACKER_NOTE (CONFIG, g_message ("Not setting disk space, configuration is set to -1 (disabled)"));
	}
}

static void
disk_space_check_stop (TrackerMinerFiles *mf)
{
	if (mf->private->disk_space_check_id) {
		TRACKER_NOTE (CONFIG, g_message ("Stopping disk space check"));
		g_source_remove (mf->private->disk_space_check_id);
		mf->private->disk_space_check_id = 0;
	}
}

static void
low_disk_space_limit_cb (GObject    *gobject,
                         GParamSpec *arg1,
                         gpointer    user_data)
{
	TrackerMinerFiles *mf = user_data;

	disk_space_check_cb (mf);
}

static void
indexing_tree_update_filter (TrackerIndexingTree *indexing_tree,
			     TrackerFilterType    filter,
			     GSList              *new_elems)
{
	tracker_indexing_tree_clear_filters (indexing_tree, filter);

	while (new_elems) {
		tracker_indexing_tree_add_filter (indexing_tree, filter,
						  new_elems->data);
		new_elems = new_elems->next;
	}
}

static void
miner_files_update_filters (TrackerMinerFiles *files)
{
	TrackerIndexingTree *indexing_tree;
	GSList *list;

	indexing_tree = tracker_miner_fs_get_indexing_tree (TRACKER_MINER_FS (files));

	/* Ignored files */
	list = tracker_config_get_ignored_files (files->private->config);
	indexing_tree_update_filter (indexing_tree, TRACKER_FILTER_FILE, list);

	/* Ignored directories */
	list = tracker_config_get_ignored_directories (files->private->config);
	indexing_tree_update_filter (indexing_tree,
				     TRACKER_FILTER_DIRECTORY,
				     list);

	/* Directories with content */
	list = tracker_config_get_ignored_directories_with_content (files->private->config);
	indexing_tree_update_filter (indexing_tree,
				     TRACKER_FILTER_PARENT_DIRECTORY,
				     list);
}

static void
update_directories_from_new_config (TrackerMinerFS *mf,
                                    GSList         *new_dirs,
                                    GSList         *old_dirs,
                                    gboolean        recurse)
{
	TrackerMinerFilesPrivate *priv;
	TrackerDirectoryFlags flags = 0;
	TrackerIndexingTree *indexing_tree;
	GSList *sl;

	priv = TRACKER_MINER_FILES_GET_PRIVATE (mf);
	indexing_tree = tracker_miner_fs_get_indexing_tree (mf);

	TRACKER_NOTE (CONFIG, g_message ("Updating %s directories changed from configuration",
	                      recurse ? "recursive" : "single"));

	/* First remove all directories removed from the config */
	for (sl = old_dirs; sl; sl = sl->next) {
		const gchar *path;

		path = sl->data;

		/* If we are not still in the list, remove the dir */
		if (!tracker_string_in_gslist (path, new_dirs)) {
			GFile *file;

			TRACKER_NOTE (CONFIG, g_message ("  Removing directory: '%s'", path));

			file = g_file_new_for_path (path);

			/* First, remove the preserve flag, it might be
			 * set on configuration directories within mount
			 * points, as data should be persistent across
			 * unmounts.
			 */
			tracker_indexing_tree_get_root (indexing_tree,
							file, &flags);

			if ((flags & TRACKER_DIRECTORY_FLAG_PRESERVE) != 0) {
				flags &= ~(TRACKER_DIRECTORY_FLAG_PRESERVE);
				tracker_indexing_tree_add (indexing_tree,
							   file, flags);
			}

			/* Fully remove item (monitors and from store),
			 * now that there's no preserve flag.
			 */
			tracker_indexing_tree_remove (indexing_tree, file);
			g_object_unref (file);
		}
	}

	flags = TRACKER_DIRECTORY_FLAG_NONE;

	if (recurse) {
		flags |= TRACKER_DIRECTORY_FLAG_RECURSE;
	}

	if (tracker_config_get_enable_monitors (priv->config)) {
		flags |= TRACKER_DIRECTORY_FLAG_MONITOR;
	}

	if (priv->mtime_check) {
		flags |= TRACKER_DIRECTORY_FLAG_CHECK_MTIME;
	}

	/* Second add directories which are new */
	for (sl = new_dirs; sl; sl = sl->next) {
		const gchar *path;

		path = sl->data;

		/* If we are now in the list, add the dir */
		if (!tracker_string_in_gslist (path, old_dirs)) {
			GFile *file;

			TRACKER_NOTE (CONFIG, g_message ("  Adding directory:'%s'", path));

			file = g_file_new_for_path (path);
			tracker_indexing_tree_add (indexing_tree, file, flags);
			g_object_unref (file);
		}
	}
}

static void
index_recursive_directories_cb (GObject    *gobject,
                                GParamSpec *arg1,
                                gpointer    user_data)
{
	TrackerMinerFilesPrivate *private;
	GSList *new_dirs, *old_dirs;

	private = TRACKER_MINER_FILES_GET_PRIVATE (user_data);

	new_dirs = tracker_config_get_index_recursive_directories (private->config);
	old_dirs = private->index_recursive_directories;

	update_directories_from_new_config (TRACKER_MINER_FS (user_data),
	                                    new_dirs,
	                                    old_dirs,
	                                    TRUE);

	/* Re-set the stored config in case it changes again */
	if (private->index_recursive_directories) {
		g_slist_foreach (private->index_recursive_directories, (GFunc) g_free, NULL);
		g_slist_free (private->index_recursive_directories);
	}

	private->index_recursive_directories = tracker_gslist_copy_with_string_data (new_dirs);
}

static void
index_single_directories_cb (GObject    *gobject,
                             GParamSpec *arg1,
                             gpointer    user_data)
{
	TrackerMinerFilesPrivate *private;
	GSList *new_dirs, *old_dirs;

	private = TRACKER_MINER_FILES_GET_PRIVATE (user_data);

	new_dirs = tracker_config_get_index_single_directories (private->config);
	old_dirs = private->index_single_directories;

	update_directories_from_new_config (TRACKER_MINER_FS (user_data),
	                                    new_dirs,
	                                    old_dirs,
	                                    FALSE);

	/* Re-set the stored config in case it changes again */
	if (private->index_single_directories) {
		g_slist_foreach (private->index_single_directories, (GFunc) g_free, NULL);
		g_slist_free (private->index_single_directories);
	}

	private->index_single_directories = tracker_gslist_copy_with_string_data (new_dirs);
}

static gboolean
miner_files_force_recheck_idle (gpointer user_data)
{
	TrackerMinerFiles *miner_files = user_data;
	TrackerIndexingTree *indexing_tree;
	GList *roots, *l;

	miner_files_update_filters (miner_files);

	indexing_tree = tracker_miner_fs_get_indexing_tree (TRACKER_MINER_FS (miner_files));
	roots = tracker_indexing_tree_list_roots (indexing_tree);

	for (l = roots; l; l = l->next)	{
		GFile *root = l->data;

		tracker_indexing_tree_notify_update (indexing_tree, root, FALSE);
	}

	miner_files->private->force_recheck_id = 0;
	g_list_free (roots);

	return FALSE;
}

static void
trigger_recheck_cb (GObject    *gobject,
                    GParamSpec *arg1,
                    gpointer    user_data)
{
	TrackerMinerFiles *mf = user_data;

	TRACKER_NOTE (CONFIG, g_message ("Ignored content related configuration changed, checking index..."));

	if (mf->private->force_recheck_id == 0) {
		/* Set idle so multiple changes in the config lead to one recheck */
		mf->private->force_recheck_id =
			g_idle_add (miner_files_force_recheck_idle, mf);
	}
}

static gboolean
index_volumes_changed_idle (gpointer user_data)
{
	TrackerMinerFiles *mf = user_data;
	GSList *mounts_removed = NULL;
	GSList *mounts_added = NULL;
	gboolean new_index_removable_devices;
	gboolean new_index_optical_discs;

	TRACKER_NOTE (CONFIG, g_message ("Volume related configuration changed, updating..."));

	/* Read new config values. Note that if removable devices is FALSE,
	 * optical discs will also always be FALSE. */
	new_index_removable_devices = tracker_config_get_index_removable_devices (mf->private->config);
	new_index_optical_discs = (new_index_removable_devices ?
	                           tracker_config_get_index_optical_discs (mf->private->config) :
	                           FALSE);

	/* Removable devices config changed? */
	if (mf->private->index_removable_devices != new_index_removable_devices) {
		GSList *m;

		/* Get list of roots for currently mounted removable devices
		 * (excluding optical) */
		m = tracker_storage_get_device_roots (mf->private->storage,
		                                      TRACKER_STORAGE_REMOVABLE,
		                                      TRUE);
		/* Set new config value */
		mf->private->index_removable_devices = new_index_removable_devices;

		if (mf->private->index_removable_devices) {
			/* If previously not indexing and now indexing, need to re-check
			 * current mounted volumes, add new monitors and index new files
			 */
			mounts_added = m;
		} else {
			/* If previously indexing and now not indexing, need to re-check
			 * current mounted volumes, remove monitors and remove all resources
			 * from the store belonging to a removable device
			 */
			mounts_removed = m;

			/* And now, single sparql update to remove all resources
			 * corresponding to removable devices (includes those
			 * not currently mounted) */
			miner_files_in_removable_media_remove_by_type (mf, TRACKER_STORAGE_REMOVABLE);
		}
	}

	/* Optical discs config changed? */
	if (mf->private->index_optical_discs != new_index_optical_discs) {
		GSList *m;

		/* Get list of roots for removable devices (excluding optical) */
		m = tracker_storage_get_device_roots (mf->private->storage,
		                                      TRACKER_STORAGE_REMOVABLE | TRACKER_STORAGE_OPTICAL,
		                                      TRUE);

		/* Set new config value */
		mf->private->index_optical_discs = new_index_optical_discs;

		if (mf->private->index_optical_discs) {
			/* If previously not indexing and now indexing, need to re-check
			 * current mounted volumes, add new monitors and index new files
			 */
			mounts_added = g_slist_concat (mounts_added, m);
		} else {
			/* If previously indexing and now not indexing, need to re-check
			 * current mounted volumes, remove monitors and remove all resources
			 * from the store belonging to a optical disc
			 */
			mounts_removed = g_slist_concat (mounts_removed, m);

			/* And now, single sparql update to remove all resources
			 * corresponding to removable+optical devices (includes those
			 * not currently mounted) */
			miner_files_in_removable_media_remove_by_type (mf, TRACKER_STORAGE_REMOVABLE | TRACKER_STORAGE_OPTICAL);
		}
	}

	/* Tell TrackerMinerFS to stop monitoring the given removed mount paths, if any */
	if (mounts_removed) {
		TrackerIndexingTree *indexing_tree;
		GSList *sl;

		indexing_tree = tracker_miner_fs_get_indexing_tree (TRACKER_MINER_FS (mf));

		for (sl = mounts_removed; sl; sl = g_slist_next (sl)) {
			GFile *mount_point_file;

			mount_point_file = g_file_new_for_path (sl->data);
			tracker_indexing_tree_remove (indexing_tree,
						      mount_point_file);
			g_object_unref (mount_point_file);
		}

		g_slist_foreach (mounts_removed, (GFunc) g_free, NULL);
		g_slist_free (mounts_removed);
	}

	/* Tell TrackerMinerFS to start monitoring the given added mount paths, if any */
	if (mounts_added) {
		GSList *sl;

		for (sl = mounts_added; sl; sl = g_slist_next (sl)) {
			miner_files_add_removable_or_optical_directory (mf,
			                                                (gchar *) sl->data,
			                                                NULL);
		}

		g_slist_foreach (mounts_added, (GFunc) g_free, NULL);
		g_slist_free (mounts_added);
	}

	mf->private->volumes_changed_id = 0;

	/* Check if the stale volume removal configuration changed from enabled to disabled
	 * or from disabled to enabled */
	if (tracker_config_get_removable_days_threshold (mf->private->config) == 0 &&
	    mf->private->stale_volumes_check_id != 0) {
		/* From having the check enabled to having it disabled, remove the timeout */
		g_debug ("  Stale volume removal now disabled, removing timeout");
		g_source_remove (mf->private->stale_volumes_check_id);
		mf->private->stale_volumes_check_id = 0;
	} else if (tracker_config_get_removable_days_threshold (mf->private->config) > 0 &&
	           mf->private->stale_volumes_check_id == 0) {
		g_debug ("  Stale volume removal now enabled, initializing timeout");
		/* From having the check disabled to having it enabled, so fire up the
		 * timeout. */
		init_stale_volume_removal (TRACKER_MINER_FILES (mf));
	}

	return FALSE;
}

static void
index_volumes_changed_cb (GObject    *gobject,
                          GParamSpec *arg1,
                          gpointer    user_data)
{
	TrackerMinerFiles *miner_files = user_data;

	if (miner_files->private->volumes_changed_id == 0) {
		/* Set idle so multiple changes in the config lead to one check */
		miner_files->private->volumes_changed_id =
			g_idle_add (index_volumes_changed_idle, miner_files);
	}
}

static void
miner_files_add_application_dir (TrackerMinerFiles   *mf,
                                 TrackerIndexingTree *indexing_tree,
                                 const gchar         *dir)
{
	GFile *file;
	gchar *path;

	/* Add $dir/applications */
	path = g_build_filename (dir, "applications", NULL);
	file = g_file_new_for_path (path);
	TRACKER_NOTE (CONFIG, g_message ("  Adding:'%s'", path));

	tracker_indexing_tree_add (indexing_tree, file,
				   TRACKER_DIRECTORY_FLAG_RECURSE |
				   TRACKER_DIRECTORY_FLAG_MONITOR |
				   TRACKER_DIRECTORY_FLAG_CHECK_MTIME);
	g_free (path);

	mf->private->application_dirs = g_slist_prepend(mf->private->application_dirs, file);
}

static void
set_up_application_indexing (TrackerMinerFiles *mf)
{
	TrackerIndexingTree *indexing_tree;
	const gchar *user_data_dir;
	const gchar * const *xdg_dirs;
	GSList *n;
	int i;

	indexing_tree = tracker_miner_fs_get_indexing_tree (TRACKER_MINER_FS (mf));

	if (tracker_config_get_index_applications (mf->private->config)) {
		TRACKER_NOTE (CONFIG, g_message ("Setting up applications to iterate from XDG system directories"));
		xdg_dirs = g_get_system_data_dirs ();

		for (i = 0; xdg_dirs[i]; i++) {
			miner_files_add_application_dir (mf, indexing_tree, xdg_dirs[i]);
		}

		user_data_dir = g_get_user_data_dir ();
		if (user_data_dir) {
			miner_files_add_application_dir (mf, indexing_tree, user_data_dir);
		}
	} else {
		TRACKER_NOTE (CONFIG, g_message ("Removing configured application directories from indexing tree"));

		for (n = mf->private->application_dirs; n != NULL; n = n->next) {
			tracker_indexing_tree_remove (indexing_tree, G_FILE (n->data));
		};

		g_slist_free_full (mf->private->application_dirs, g_object_unref);
		mf->private->application_dirs = NULL;
	}
}

static gboolean
index_applications_changed_idle (gpointer user_data)
{
	TrackerMinerFiles *mf;

	mf = TRACKER_MINER_FILES (user_data);

	set_up_application_indexing (mf);

	return FALSE;
}

static void
index_applications_changed_cb (GObject    *gobject,
                               GParamSpec *arg1,
                               gpointer    user_data)
{
	TrackerMinerFiles *miner_files = user_data;

	TRACKER_NOTE (CONFIG, g_message ("Application related configuration changed, updating..."));

	if (miner_files->private->applications_changed_id == 0) {
		/* Set idle so multiple changes in the config lead to one check */
		miner_files->private->applications_changed_id =
			g_idle_add (index_applications_changed_idle, miner_files);
	}
}

static void
miner_files_add_to_datasource (TrackerMinerFiles *mf,
                               GFile             *file,
                               TrackerResource   *resource,
                               TrackerResource   *element_resource)
{
	TrackerIndexingTree *indexing_tree;

	indexing_tree = tracker_miner_fs_get_indexing_tree (TRACKER_MINER_FS (mf));

	if (tracker_indexing_tree_file_is_root (indexing_tree, file)) {
		tracker_resource_set_relation (resource, "nie:dataSource", element_resource);
	} else {
		const gchar *root_urn = NULL;
		GFile *root;

		root = tracker_indexing_tree_get_root (indexing_tree, file, NULL);

		if (root)
			root_urn = tracker_miner_fs_query_urn (TRACKER_MINER_FS (mf), root);

		if (root_urn)
			tracker_resource_set_uri (resource, "nie:dataSource", root_urn);
	}
}

static void
miner_files_add_mount_info (TrackerMinerFiles *miner,
                            TrackerResource   *resource,
                            GFile             *file)
{
	TrackerMinerFilesPrivate *priv = TRACKER_MINER_FILES_GET_PRIVATE (miner);
	TrackerStorageType storage_type;
	const gchar *uuid;

	uuid = tracker_storage_get_uuid_for_file (priv->storage, file);
	if (!uuid)
		return;

	storage_type = tracker_storage_get_type_for_uuid (priv->storage, uuid);

	tracker_resource_set_boolean (resource, "tracker:isRemovable",
	                              (storage_type & TRACKER_STORAGE_REMOVABLE) != 0);
	tracker_resource_set_boolean (resource, "tracker:isOptical",
	                              (storage_type & TRACKER_STORAGE_OPTICAL) != 0);
}

static TrackerResource *
miner_files_create_folder_information_element (TrackerMinerFiles *miner,
					       GFile             *file,
					       const gchar       *mime_type,
					       gboolean           is_directory)
{
	TrackerResource *resource, *file_resource;
	const gchar *urn = NULL;
	gchar *uri;

	/* Preserve URN for nfo:Folders */
	urn = tracker_miner_fs_get_urn (TRACKER_MINER_FS (miner), file);

	resource = tracker_resource_new (urn);
	tracker_resource_set_string (resource, "nie:mimeType", mime_type);
	tracker_resource_add_uri (resource, "rdf:type", "nie:InformationElement");

	if (is_directory) {
		TrackerIndexingTree *indexing_tree;

		tracker_resource_add_uri (resource, "rdf:type", "nfo:Folder");
		indexing_tree = tracker_miner_fs_get_indexing_tree (TRACKER_MINER_FS (miner));

		if (tracker_indexing_tree_file_is_root (indexing_tree, file)) {
			tracker_resource_add_uri (resource, "rdf:type", "tracker:IndexedFolder");
			tracker_resource_set_boolean (resource, "tracker:available", TRUE);
			tracker_resource_set_relation (resource, "nie:rootElementOf", resource);

			miner_files_add_mount_info (miner, resource, file);
		}
	}

	uri = g_file_get_uri (file);
	file_resource = tracker_resource_new (uri);
	tracker_resource_add_uri (file_resource, "rdf:type", "nfo:FileDataObject");
	g_free (uri);

	/* Laying the link between the IE and the DO */
	tracker_resource_add_take_relation (resource, "nie:isStoredAs", file_resource);
	tracker_resource_add_uri (file_resource, "nie:interpretedAs",
				  tracker_resource_get_identifier (resource));

	return resource;
}

static void
process_file_data_free (ProcessFileData *data)
{
	g_object_unref (data->miner);
	g_object_unref (data->cancellable);
	g_object_unref (data->file);
	g_object_unref (data->task);
	g_free (data->mime_type);
	g_slice_free (ProcessFileData, data);
}

static gchar *
update_mount_point_sparql (ProcessFileData *data)
{
	const gchar *uuid;

	uuid = g_object_get_qdata (G_OBJECT (data->file),
	                           data->miner->private->quark_mount_point_uuid);

	/* File represents a mount point */
	if (G_UNLIKELY (uuid)) {
		GString *queries;
		gchar *removable_device_urn, *uri;

		removable_device_urn = g_strdup_printf (TRACKER_PREFIX_DATASOURCE_URN "%s", uuid);
		uri = g_file_get_uri (G_FILE (data->file));
		queries = g_string_new ("");

		g_string_append_printf (queries,
		                        "DELETE { "
		                        "  <%s> tracker:mountPoint ?unknown "
		                        "} WHERE { "
		                        "  <%s> a tracker:IndexedFolder; "
		                        "       tracker:mountPoint ?unknown "
		                        "} ",
		                        removable_device_urn, removable_device_urn);

		g_string_append_printf (queries,
		                        "INSERT { "
		                        "  <%s> a tracker:IndexedFolder; "
		                        "       tracker:mountPoint ?u "
		                        "} WHERE { "
		                        "  ?u a nfo:FileDataObject; "
		                        "     nie:url \"%s\" "
		                        "} ",
		                        removable_device_urn, uri);

		g_free (removable_device_urn);
		g_free (uri);

		return g_string_free (queries, FALSE);
	}

	return NULL;
}

static void
process_file_cb (GObject      *object,
                 GAsyncResult *result,
                 gpointer      user_data)
{
	TrackerMinerFilesPrivate *priv;
	TrackerResource *resource, *folder_resource = NULL;
	ProcessFileData *data;
	const gchar *mime_type, *graph;
	gchar *parent_urn;
	gchar *delete_properties_sparql = NULL, *mount_point_sparql;
	GFileInfo *file_info;
	guint64 time_;
	GFile *file, *parent;
	gchar *uri, *sparql_str, *sparql_update_str, *time_str, *ie_update_str = NULL, *graph_file_str = NULL;
	GError *error = NULL;
	gboolean is_special;
	gboolean is_directory;

	data = user_data;
	file = G_FILE (object);
	file_info = g_file_query_info_finish (file, result, &error);
	priv = TRACKER_MINER_FILES (data->miner)->private;

	if (error) {
		/* Something bad happened, notify about the error */
		tracker_miner_fs_notify_finish (TRACKER_MINER_FS (data->miner), data->task, NULL, error);
		priv->extraction_queue = g_list_remove (priv->extraction_queue, data);
		process_file_data_free (data);
		return;
	}

	is_special = (g_file_info_get_file_type (file_info) == G_FILE_TYPE_SPECIAL ?
	                TRUE : FALSE);
	if (is_special) {
		error = g_error_new (TRACKER_MINER_ERROR,
		                     0,
		                     "File is a device, socket or pipe. Refusing to index it.");
		tracker_miner_fs_notify_finish (TRACKER_MINER_FS (data->miner), data->task, NULL, error);
		priv->extraction_queue = g_list_remove (priv->extraction_queue, data);
		process_file_data_free (data);
		return;
	}

	uri = g_file_get_uri (file);
	mime_type = g_file_info_get_content_type (file_info);

	data->mime_type = g_strdup (mime_type);
	is_directory = (g_file_info_get_file_type (file_info) == G_FILE_TYPE_DIRECTORY ?
	                TRUE : FALSE);

	if (!is_directory &&
	    tracker_miner_fs_get_urn (TRACKER_MINER_FS (data->miner), file)) {
		/* Update: delete all information elements for the given data object
		 * and delete dataSources, so we ensure the file is extracted again.
		 */
		delete_properties_sparql =
			g_strdup_printf ("DELETE {"
			                 "  GRAPH ?g {"
			                 "    <%s> nie:interpretedAs ?ie ; "
			                 "         nie:dataSource ?ds . "
			                 "    ?ie a rdfs:Resource . "
			                 "  }"
			                 "} WHERE {"
			                 "  GRAPH ?g {"
			                 "    <%s> nie:interpretedAs ?ie ."
			                 "    OPTIONAL { <%s> nie:dataSource ?ds } "
			                 "  }"
			                 "} ",
			                 uri, uri, uri);
	}

	resource = tracker_resource_new (uri);

	tracker_resource_add_uri (resource, "rdf:type", "nfo:FileDataObject");

	parent = g_file_get_parent (file);
	parent_urn = tracker_miner_fs_query_urn (TRACKER_MINER_FS (data->miner), parent);
	g_object_unref (parent);

	if (parent_urn) {
		tracker_resource_set_uri (resource, "nfo:belongsToContainer", parent_urn);
		g_free (parent_urn);
	}

	tracker_resource_set_string (resource, "nfo:fileName",
	                             g_file_info_get_display_name (file_info));
	tracker_resource_set_int64 (resource, "nfo:fileSize",
	                            g_file_info_get_size (file_info));

	time_ = g_file_info_get_attribute_uint64 (file_info, G_FILE_ATTRIBUTE_TIME_MODIFIED);
	time_str = tracker_date_to_string (time_);
	tracker_resource_set_string (resource, "nfo:fileLastModified", time_str);
	g_free (time_str);

	time_ = g_file_info_get_attribute_uint64 (file_info, G_FILE_ATTRIBUTE_TIME_ACCESS);
	time_str = tracker_date_to_string (time_);
	tracker_resource_set_string (resource, "nfo:fileLastAccessed", time_str);
	g_free (time_str);

	/* The URL of the DataObject (because IE = DO, this is correct) */
	tracker_resource_set_string (resource, "nie:url", uri);

	if (is_directory) {
		folder_resource =
			miner_files_create_folder_information_element (data->miner,
								       file,
								       mime_type,
								       is_directory);
	}

	miner_files_add_to_datasource (data->miner, file, resource, folder_resource);

	mount_point_sparql = update_mount_point_sparql (data);
	sparql_update_str = tracker_resource_print_sparql_update (resource, NULL, DEFAULT_GRAPH);

	if (folder_resource) {
		ie_update_str = tracker_resource_print_sparql_update (folder_resource,
		                                                      NULL,
		                                                      DEFAULT_GRAPH);
		g_object_unref (folder_resource);
	}

	graph = tracker_extract_module_manager_get_graph (mime_type);

	if (graph) {
		TrackerResource *graph_file;

		/* This mimetype will be extracted by some module, pre-fill the
		 * nfo:FileDataObject in that graph.
		 */
		graph_file = tracker_resource_new (uri);
		tracker_resource_add_uri (graph_file, "rdf:type", "nfo:FileDataObject");
		graph_file_str = tracker_resource_print_sparql_update (graph_file,
								       NULL, graph);
	}

	sparql_str = g_strdup_printf ("%s %s %s %s %s",
	                              delete_properties_sparql ? delete_properties_sparql : "",
	                              sparql_update_str,
	                              ie_update_str ? ie_update_str : "",
				      graph_file_str ? graph_file_str : "",
	                              mount_point_sparql ? mount_point_sparql : "");
	g_free (ie_update_str);
	g_free (delete_properties_sparql);
	g_free (mount_point_sparql);

	tracker_miner_fs_notify_finish (TRACKER_MINER_FS (data->miner), data->task,
	                                sparql_str, NULL);

	priv->extraction_queue = g_list_remove (priv->extraction_queue, data);
	process_file_data_free (data);

	g_object_run_dispose (G_OBJECT (resource));
	g_object_unref (resource);
	g_object_unref (file_info);
	g_free (sparql_str);
	g_free (uri);
	g_free (sparql_update_str);
}

static gboolean
miner_files_process_file (TrackerMinerFS *fs,
                          GFile          *file,
                          GTask          *task)
{
	TrackerMinerFilesPrivate *priv;
	ProcessFileData *data;
	const gchar *attrs;

	data = g_slice_new0 (ProcessFileData);
	data->miner = TRACKER_MINER_FILES (g_object_ref (fs));
	data->cancellable = g_object_ref (g_task_get_cancellable (task));
	data->file = g_object_ref (file);
	data->task = g_object_ref (task);

	priv = TRACKER_MINER_FILES (fs)->private;
	priv->extraction_queue = g_list_prepend (priv->extraction_queue, data);
	priv->start_extractor = TRUE;

	attrs = G_FILE_ATTRIBUTE_STANDARD_TYPE ","
		G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE ","
		G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME ","
		G_FILE_ATTRIBUTE_STANDARD_SIZE ","
		G_FILE_ATTRIBUTE_TIME_MODIFIED ","
		G_FILE_ATTRIBUTE_TIME_ACCESS;

	g_file_query_info_async (file,
	                         attrs,
	                         G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
	                         G_PRIORITY_DEFAULT,
	                         data->cancellable,
	                         process_file_cb,
	                         data);

	return TRUE;
}

static void
process_file_attributes_cb (GObject      *object,
                            GAsyncResult *result,
                            gpointer      user_data)
{
	TrackerResource *resource;
	ProcessFileData *data;
	GFileInfo *file_info;
	guint64 time_;
	GFile *file;
	gchar *uri, *time_str, *sparql_str;
	GError *error = NULL;

	data = user_data;
	file = G_FILE (object);
	file_info = g_file_query_info_finish (file, result, &error);

	if (error) {
		/* Something bad happened, notify about the error */
		tracker_miner_fs_notify_finish (TRACKER_MINER_FS (data->miner), data->task, NULL, error);
		process_file_data_free (data);
		return;
	}

	uri = g_file_get_uri (file);
	resource = tracker_resource_new (uri);

	/* Update nfo:fileLastModified */
	time_ = g_file_info_get_attribute_uint64 (file_info, G_FILE_ATTRIBUTE_TIME_MODIFIED);
	time_str = tracker_date_to_string (time_);
	tracker_resource_set_string (resource, "nfo:fileLastModified", time_str);
	g_free (time_str);

	/* Update nfo:fileLastAccessed */
	time_ = g_file_info_get_attribute_uint64 (file_info, G_FILE_ATTRIBUTE_TIME_ACCESS);
	time_str = tracker_date_to_string (time_);
	tracker_resource_set_string (resource, "nfo:fileLastAccessed", time_str);
	g_free (time_str);

	g_object_unref (file_info);
	g_free (uri);

	/* Notify about the success */
	sparql_str = tracker_resource_print_sparql_update (resource, NULL, DEFAULT_GRAPH);
	tracker_miner_fs_notify_finish (TRACKER_MINER_FS (data->miner), data->task,
	                                sparql_str, NULL);

	process_file_data_free (data);
	g_object_unref (resource);
	g_free (sparql_str);
}

static gboolean
miner_files_process_file_attributes (TrackerMinerFS *fs,
                                     GFile          *file,
                                     GTask          *task)
{
	ProcessFileData *data;
	const gchar *attrs;

	data = g_slice_new0 (ProcessFileData);
	data->miner = TRACKER_MINER_FILES (g_object_ref (fs));
	data->cancellable = g_object_ref (g_task_get_cancellable (task));
	data->file = g_object_ref (file);
	data->task = g_object_ref (task);

	/* Query only attributes that may change in an ATTRIBUTES_UPDATED event */
	attrs = G_FILE_ATTRIBUTE_TIME_MODIFIED ","
		G_FILE_ATTRIBUTE_TIME_ACCESS;

	g_file_query_info_async (file,
	                         attrs,
	                         G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
	                         G_PRIORITY_DEFAULT,
	                         data->cancellable,
	                         process_file_attributes_cb,
	                         data);

	return TRUE;
}

static void
miner_files_finished (TrackerMinerFS *fs,
                      gdouble         elapsed,
                      gint            directories_found,
                      gint            directories_ignored,
                      gint            files_found,
                      gint            files_ignored)
{
	tracker_miner_files_set_last_crawl_done (TRACKER_MINER_FILES (fs), TRUE);

	tracker_miner_files_check_unextracted (TRACKER_MINER_FILES (fs));
}

static gchar *
create_delete_sparql (GFile    *file,
		      gboolean  delete_self,
		      gboolean  delete_children)
{
	GString *sparql;
	gchar *uri;

	g_return_val_if_fail (delete_self || delete_children, NULL);

	uri = g_file_get_uri (file);
	sparql = g_string_new ("DELETE { "
	                       "  GRAPH " DEFAULT_GRAPH " {"
	                       "    ?f a rdfs:Resource . "
	                       "  }"
	                       "  GRAPH ?g {"
	                       "    ?f a rdfs:Resource . "
	                       "    ?ie a rdfs:Resource . "
	                       "  }"
	                       "} WHERE {"
	                       "  GRAPH " DEFAULT_GRAPH " {"
	                       "    ?f a rdfs:Resource ; "
	                       "       nie:url ?u . "
	                       "  }"
	                       "  GRAPH ?g {"
	                       "    ?ie nie:isStoredAs ?f . "
	                       "  }"
	                       "  FILTER (");

	if (delete_self)
		g_string_append_printf (sparql, "?u = \"%s\" ", uri);

	if (delete_children) {
		if (delete_self)
			g_string_append (sparql, " || ");

		g_string_append_printf (sparql, "STRSTARTS (?u, \"%s/\")", uri);
	}

	g_string_append (sparql, ")}");
	g_free (uri);

	return g_string_free (sparql, FALSE);
}

static gchar *
miner_files_remove_children (TrackerMinerFS *fs,
                             GFile          *file)
{
	return create_delete_sparql (file, FALSE, TRUE);
}

static gchar *
miner_files_remove_file (TrackerMinerFS *fs,
                         GFile          *file)
{
	return create_delete_sparql (file, TRUE, TRUE);
}

static gchar *
miner_files_move_file (TrackerMinerFS *fs,
                       GFile          *file,
                       GFile          *source_file,
                       gboolean        recursive)
{
	GString *sparql = g_string_new (NULL);
	const gchar *new_parent_iri = NULL;
	gchar *uri, *source_uri, *display_name, *container_clause = NULL;
	gchar *path, *basename;
	GFile *new_parent;

	uri = g_file_get_uri (file);
	source_uri = g_file_get_uri (source_file);

	path = g_file_get_path (file);
	basename = g_filename_display_basename (path);
	display_name = tracker_sparql_escape_string (basename);
	g_free (basename);
	g_free (path);

	/* Get new parent information */
	new_parent = g_file_get_parent (file);
	if (new_parent)
		new_parent_iri = tracker_miner_fs_query_urn (fs, new_parent);
	if (new_parent_iri)
		container_clause = g_strdup_printf ("; nfo:belongsToContainer <%s>", new_parent_iri);

	g_string_append_printf (sparql,
	                        "DELETE { "
	                        "  GRAPH ?g {"
	                        "    ?ie nie:isStoredAs <%s> "
	                        "  }"
	                        "} INSERT {"
	                        "  GRAPH ?g {"
	                        "    ?ie nie:isStoredAs <%s> "
	                        "  }"
	                        "} WHERE {"
	                        "  GRAPH ?g {"
	                        "    ?ie nie:isStoredAs <%s> "
	                        "  }"
	                        "} "
	                        "DELETE { "
	                        "  <%s> a rdfs:Resource . "
	                        "} INSERT { "
	                        "  <%s> a nfo:FileDataObject ; "
	                        "       nfo:fileName \"%s\" ; "
	                        "       nie:url \"%s\" "
	                        "       %s ; "
	                        "       ?p ?o . "
	                        "} WHERE { "
	                        "  <%s> ?p ?o ; "
	                        "  FILTER (?p != nfo:fileName && ?p != nie:url && ?p != nfo:belongsToContainer) . "
	                        "} ",
	                        source_uri, uri, source_uri,
	                        source_uri,
	                        uri, display_name, uri, container_clause,
	                        source_uri);
	g_free (container_clause);

	if (recursive) {
		g_string_append_printf (sparql,
		                        " DELETE { "
		                        "  ?u a rdfs:Resource . "
		                        "  ?ie1 nie:isStoredAs ?u "
		                        "  GRAPH ?g {"
		                        "    ?ie2 nie:isStoredAs ?u "
		                        "  }"
		                        "} INSERT { "
		                        "  ?new_url a nfo:FileDataObject ; "
		                        "           nie:url ?new_url ; "
		                        "           ?p ?o ."
		                        "  ?ie1 nie:isStoredAs ?new_url . "
		                        "  GRAPH ?g {"
		                        "    ?ie2 nie:isStoredAs ?new_url . "
		                        "  }"
		                        "} WHERE { "
		                        "  ?u a rdfs:Resource; "
		                        "     nie:url ?url ; "
		                        "     ?p ?o . "
		                        "  OPTIONAL { ?u nie:interpretedAs ?ie1 } "
		                        "  OPTIONAL { GRAPH ?g {"
		                        "    ?u nie:interpretedAs ?ie2 "
		                        "  }}"
		                        "  BIND (CONCAT (\"%s/\", SUBSTR (?url, STRLEN (\"%s/\") + 1)) AS ?new_url) ."
		                        "  FILTER (STRSTARTS (?url, \"%s/\")) . "
		                        "  FILTER (?p != nie:url) . "
		                        "} ",
		                        uri, source_uri, source_uri);
	}

	g_free (uri);
	g_free (source_uri);
	g_free (display_name);
	g_clear_object (&new_parent);

	return g_string_free (sparql, FALSE);
}

TrackerMiner *
tracker_miner_files_new (TrackerSparqlConnection  *connection,
                         TrackerConfig            *config,
                         const gchar              *domain,
                         GError                  **error)
{
	return g_initable_new (TRACKER_TYPE_MINER_FILES,
	                       NULL,
	                       error,
	                       "connection", connection,
	                       "root", NULL,
	                       "config", config,
	                       "domain", domain,
	                       "processing-pool-wait-limit", 10,
	                       "processing-pool-ready-limit", 100,
	                       NULL);
}

gboolean
tracker_miner_files_check_file (GFile  *file,
                                GSList *ignored_file_paths,
                                GSList *ignored_file_patterns)
{
	GSList *l;
	gchar *basename;
	gchar *path;
	gboolean should_process;

	should_process = FALSE;
	basename = NULL;
	path = NULL;

	if (tracker_file_is_hidden (file)) {
		/* Ignore hidden files */
		goto done;
	}

	path = g_file_get_path (file);

	for (l = ignored_file_paths; l; l = l->next) {
		if (strcmp (l->data, path) == 0) {
			goto done;
		}
	}

	basename = g_file_get_basename (file);

	for (l = ignored_file_patterns; l; l = l->next) {
		if (g_pattern_match_string (l->data, basename)) {
			goto done;
		}
	}

	should_process = TRUE;

done:
	g_free (basename);
	g_free (path);

	return should_process;
}

gboolean
tracker_miner_files_check_directory (GFile  *file,
                                     GSList *index_recursive_directories,
                                     GSList *index_single_directories,
                                     GSList *ignored_directory_paths,
                                     GSList *ignored_directory_patterns)
{
	GSList *l;
	gchar *basename;
	gchar *path;
	gboolean should_process;
	gboolean is_hidden;

	should_process = FALSE;
	basename = NULL;

	path = g_file_get_path (file);

	/* First we check the GIO hidden check. This does a number of
	 * things for us which is good (like checking ".foo" dirs).
	 */
	is_hidden = tracker_file_is_hidden (file);

#ifdef __linux__
	/* Second we check if the file is on FAT and if the hidden
	 * attribute is set. GIO does this but ONLY on a Windows OS,
	 * not for Windows files under a Linux OS, so we have to check
	 * anyway.
	 */
	if (!is_hidden) {
		int fd;

		fd = g_open (path, O_RDONLY, 0);
		if (fd != -1) {
			__u32 attrs;

			if (ioctl (fd, FAT_IOCTL_GET_ATTRIBUTES, &attrs) == 0) {
				is_hidden = attrs & ATTR_HIDDEN ? TRUE : FALSE;
			}

			close (fd);
		}
	}
#endif /* __linux__ */

	if (is_hidden) {
		/* FIXME: We need to check if the file is actually a
		 * config specified location before blanket ignoring
		 * all hidden files.
		 */
		if (tracker_string_in_gslist (path, index_recursive_directories)) {
			should_process = TRUE;
		}

		if (tracker_string_in_gslist (path, index_single_directories)) {
			should_process = TRUE;
		}

		/* Ignore hidden dirs */
		goto done;
	}

	for (l = ignored_directory_paths; l; l = l->next) {
		if (strcmp (l->data, path) == 0) {
			goto done;
		}
	}

	basename = g_file_get_basename (file);

	for (l = ignored_directory_patterns; l; l = l->next) {
		if (g_pattern_match_string (l->data, basename)) {
			goto done;
		}
	}

	/* Check module directory ignore patterns */
	should_process = TRUE;

done:
	g_free (basename);
	g_free (path);

	return should_process;
}

gboolean
tracker_miner_files_check_directory_contents (GFile  *parent,
                                              GList  *children,
                                              GSList *ignored_content)
{
	GSList *l;

	if (!ignored_content) {
		return TRUE;
	}

	while (children) {
		gchar *basename;

		basename = g_file_get_basename (children->data);

		for (l = ignored_content; l; l = l->next) {
			if (g_strcmp0 (basename, l->data) == 0) {
				gchar *parent_uri;

				parent_uri = g_file_get_uri (parent);
				/* g_debug ("Directory '%s' ignored since it contains a file named '%s'", */
				/*          parent_uri, basename); */

				g_free (parent_uri);
				g_free (basename);

				return FALSE;
			}
		}

		children = children->next;
		g_free (basename);
	}

	return TRUE;
}

gboolean
tracker_miner_files_monitor_directory (GFile    *file,
                                       gboolean  enable_monitors,
                                       GSList   *directories_to_check)
{
	if (!enable_monitors) {
		return FALSE;
	}

	/* We'll only get this signal for the directories where check_directory()
	 * and check_directory_contents() returned TRUE, so by default we want
	 * these directories to be indexed. */

	return TRUE;
}

static void
remove_files_in_removable_media_cb (GObject      *object,
                                    GAsyncResult *result,
                                    gpointer      user_data)
{
	GError *error = NULL;

	tracker_sparql_connection_update_finish (TRACKER_SPARQL_CONNECTION (object), result, &error);

	if (error) {
		g_critical ("Could not remove files in volumes: %s", error->message);
		g_error_free (error);
	}
}

static gboolean
miner_files_in_removable_media_remove_by_type (TrackerMinerFiles  *miner,
                                               TrackerStorageType  type)
{
	gboolean removable;
	gboolean optical;

	removable = TRACKER_STORAGE_TYPE_IS_REMOVABLE (type);
	optical = TRACKER_STORAGE_TYPE_IS_OPTICAL (type);

	/* Only remove if any of the flags was TRUE */
	if (removable || optical) {
		GString *queries;

		g_debug ("  Removing all resources in store from %s ",
		         optical ? "optical discs" : "removable devices");

		queries = g_string_new ("");

		/* Delete all resources where nie:dataSource is a volume
		 * of the given type */
		g_string_append_printf (queries,
		                        "DELETE { "
		                        "  ?f a rdfs:Resource . "
		                        "  GRAPH ?g {"
		                        "    ?ie a rdfs:Resource "
		                        "  }"
		                        "} WHERE { "
		                        "  ?v a tracker:IndexedFolder ; "
		                        "     tracker:isRemovable %s ; "
		                        "     tracker:isOptical %s . "
		                        "  ?f nie:dataSource ?v . "
		                        "  GRAPH ?g {"
		                        "    ?ie nie:isStoredAs ?f "
		                        "  }"
		                        "}",
		                        removable ? "true" : "false",
		                        optical ? "true" : "false");

		tracker_sparql_connection_update_async (tracker_miner_get_connection (TRACKER_MINER (miner)),
		                                        queries->str,
		                                        G_PRIORITY_LOW,
		                                        NULL,
		                                        remove_files_in_removable_media_cb,
		                                        NULL);

		g_string_free (queries, TRUE);

		return TRUE;
	}

	return FALSE;
}

static void
miner_files_in_removable_media_remove_by_date (TrackerMinerFiles  *miner,
                                               const gchar        *date)
{
	GString *queries;

	g_debug ("  Removing all resources in store from removable or "
	         "optical devices not mounted after '%s'",
	         date);

	queries = g_string_new ("");

	/* Delete all resources where nie:dataSource is a volume
	 * which was last unmounted before the given date */
	g_string_append_printf (queries,
	                        "DELETE { "
	                        "  ?f a rdfs:Resource . "
	                        "  GRAPH ?g {"
	                        "    ?ie a rdfs:Resource "
	                        "  }"
	                        "} WHERE { "
	                        "  ?v a tracker:IndexedFolder ; "
	                        "     tracker:isRemovable true ; "
	                        "     tracker:available false ; "
	                        "     tracker:unmountDate ?d . "
	                        "  ?f nie:dataSource ?v . "
	                        "  GRAPH ?g {"
	                        "    ?ie nie:isStoredAs ?f "
	                        "  }"
	                        "  FILTER ( ?d < \"%s\") "
	                        "}",
	                        date);

	tracker_sparql_connection_update_async (tracker_miner_get_connection (TRACKER_MINER (miner)),
	                                        queries->str,
	                                        G_PRIORITY_LOW,
	                                        NULL,
	                                        remove_files_in_removable_media_cb,
	                                        NULL);

	g_string_free (queries, TRUE);
}

static void
miner_files_add_removable_or_optical_directory (TrackerMinerFiles *mf,
                                                const gchar       *mount_path,
                                                const gchar       *uuid)
{
	TrackerIndexingTree *indexing_tree;
	TrackerDirectoryFlags flags;
	GFile *mount_point_file;

	mount_point_file = g_file_new_for_path (mount_path);

	/* UUID may be NULL, and if so, get it */
	if (!uuid) {
		uuid = tracker_storage_get_uuid_for_file (mf->private->storage,
		                                          mount_point_file);
		if (!uuid) {
			g_critical ("Couldn't get UUID for mount point '%s'",
			            mount_path);
			g_object_unref (mount_point_file);
			return;
		}
	}

	indexing_tree = tracker_miner_fs_get_indexing_tree (TRACKER_MINER_FS (mf));
	flags = TRACKER_DIRECTORY_FLAG_RECURSE |
		TRACKER_DIRECTORY_FLAG_CHECK_MTIME |
		TRACKER_DIRECTORY_FLAG_PRESERVE |
		TRACKER_DIRECTORY_FLAG_PRIORITY;

	if (tracker_config_get_enable_monitors (mf->private->config)) {
		flags |= TRACKER_DIRECTORY_FLAG_MONITOR;
	}

	g_object_set_qdata_full (G_OBJECT (mount_point_file),
	                         mf->private->quark_mount_point_uuid,
	                         g_strdup (uuid),
	                         (GDestroyNotify) g_free);

	g_debug ("  Adding removable/optical: '%s'", mount_path);
	tracker_indexing_tree_add (indexing_tree,
				   mount_point_file,
				   flags);
	g_object_unref (mount_point_file);
}

gboolean
tracker_miner_files_is_file_eligible (TrackerMinerFiles *miner,
                                      GFile             *file)
{
	TrackerConfig *config;
	GFile *dir;
	GFileInfo *file_info;
	gboolean is_dir;

	file_info = g_file_query_info (file,
	                               G_FILE_ATTRIBUTE_STANDARD_TYPE,
	                               G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
	                               NULL, NULL);

	if (!file_info) {
		/* file does not exist */
		return FALSE;
	}

	is_dir = (g_file_info_get_file_type (file_info) == G_FILE_TYPE_DIRECTORY);
	g_object_unref (file_info);

	g_object_get (miner,
	              "config", &config,
	              NULL);

	if (is_dir) {
		dir = g_object_ref (file);
	} else {
		if (!tracker_miner_files_check_file (file,
		                                     tracker_config_get_ignored_file_paths (config),
		                                     tracker_config_get_ignored_file_patterns (config))) {
			/* file is not eligible to be indexed */
			g_object_unref (config);
			return FALSE;
		}

		dir = g_file_get_parent (file);
	}

	if (dir) {
		gboolean found = FALSE;
		GSList *l;

		if (!tracker_miner_files_check_directory (dir,
		                                          tracker_config_get_index_recursive_directories (config),
		                                          tracker_config_get_index_single_directories (config),
		                                          tracker_config_get_ignored_directory_paths (config),
		                                          tracker_config_get_ignored_directory_patterns (config))) {
			/* file is not eligible to be indexed */
			g_object_unref (dir);
			g_object_unref (config);
			return FALSE;
		}

		l = tracker_config_get_index_recursive_directories (config);

		while (l && !found) {
			GFile *config_dir;

			config_dir = g_file_new_for_path ((gchar *) l->data);

			if (g_file_equal (dir, config_dir) ||
			    g_file_has_prefix (dir, config_dir)) {
				found = TRUE;
			}

			g_object_unref (config_dir);
			l = l->next;
		}

		l = tracker_config_get_index_single_directories (config);

		while (l && !found) {
			GFile *config_dir;

			config_dir = g_file_new_for_path ((gchar *) l->data);

			if (g_file_equal (dir, config_dir)) {
				found = TRUE;
			}

			g_object_unref (config_dir);
			l = l->next;
		}

		g_object_unref (dir);

		if (!found) {
			/* file is not eligible to be indexed */
			g_object_unref (config);
			return FALSE;
		}
	}

	g_object_unref (config);

	/* file is eligible to be indexed */
	return TRUE;
}

inline static gchar *
get_first_index_filename (TrackerMinerFiles *mf)
{
	GFile *file;
	gchar *prefix, *path;

	file = get_cache_dir (mf);
	prefix = g_file_get_path (file);

	path = g_build_filename (prefix,
	                         FIRST_INDEX_FILENAME,
	                         NULL);
	g_free (prefix);
	g_object_unref (file);

	return path;
}

/**
 * tracker_miner_files_get_first_index_done:
 *
 * Check if first full index of files was already done.
 *
 * Returns: %TRUE if a first full index have been done, %FALSE otherwise.
 **/
gboolean
tracker_miner_files_get_first_index_done (TrackerMinerFiles *mf)
{
	gboolean exists;
	gchar *filename;

	filename = get_first_index_filename (mf);
	exists = g_file_test (filename, G_FILE_TEST_EXISTS);
	g_free (filename);

	return exists;
}

/**
 * tracker_miner_files_set_first_index_done:
 *
 * Set the status of the first full index of files. Should be set to
 *  %FALSE if the index was never done or if a reindex is needed. When
 *  the index is completed, should be set to %TRUE.
 **/
void
tracker_miner_files_set_first_index_done (TrackerMinerFiles *mf,
					  gboolean           done)
{
	gboolean already_exists;
	gchar *filename;

	filename = get_first_index_filename (mf);
	already_exists = g_file_test (filename, G_FILE_TEST_EXISTS);

	if (done && !already_exists) {
		GError *error = NULL;

		/* If done, create stamp file if not already there */
		if (!g_file_set_contents (filename, PACKAGE_VERSION, -1, &error)) {
			g_warning ("  Could not create file:'%s' failed, %s",
			           filename,
			           error->message);
			g_error_free (error);
		} else {
			g_info ("  First index file:'%s' created", filename);
		}
	} else if (!done && already_exists) {
		/* If NOT done, remove stamp file */
		g_info ("  Removing first index file:'%s'", filename);

		if (g_remove (filename)) {
			g_warning ("    Could not remove file:'%s': %m",
			           filename);
		}
	}

	g_free (filename);
}

static inline gchar *
get_last_crawl_filename (TrackerMinerFiles *mf)
{
	GFile *file;
	gchar *prefix, *path;

	file = get_cache_dir (mf);
	prefix = g_file_get_path (file);

	path = g_build_filename (prefix,
	                         LAST_CRAWL_FILENAME,
	                         NULL);
	g_free (prefix);
	g_object_unref (file);

	return path;
}

/**
 * tracker_miner_files_get_last_crawl_done:
 *
 * Check when last crawl was performed.
 *
 * Returns: time_t() value when last crawl occurred, otherwise 0.
 **/
guint64
tracker_miner_files_get_last_crawl_done (TrackerMinerFiles *mf)
{
	gchar *filename;
	gchar *content;
	guint64 then;

	filename = get_last_crawl_filename (mf);

	if (!g_file_get_contents (filename, &content, NULL, NULL)) {
		g_info ("  No previous timestamp, crawling forced");
		return 0;
	}

	then = g_ascii_strtoull (content, NULL, 10);
	g_free (content);

	return then;
}

/**
 * tracker_miner_files_set_last_crawl_done:
 *
 * Set the time stamp of the last full index of files.
 **/
void
tracker_miner_files_set_last_crawl_done (TrackerMinerFiles *mf,
					 gboolean           done)
{
	gboolean already_exists;
	gchar *filename;

	filename = get_last_crawl_filename (mf);
	already_exists = g_file_test (filename, G_FILE_TEST_EXISTS);

	if (done) {
		GError *error = NULL;
		gchar *content;
		content = g_strdup_printf ("%" G_GUINT64_FORMAT, (guint64) time (NULL));
		if (already_exists) {
			g_info ("  Overwriting last crawl file:'%s'", filename);
		} else {
			g_info ("  Creating last crawl file:'%s'", filename);
		}
		/* Create/update time stamp file */
		if (!g_file_set_contents (filename, content, -1, &error)) {
			g_warning ("  Could not create/overwrite file:'%s' failed, %s",
			           filename,
			           error->message);
			g_error_free (error);
		} else {
			g_info ("  Last crawl file:'%s' updated", filename);
		}

		g_free (content);
	} else {
		g_info ("  Crawl not done yet, doesn't update last crawl file.");
	}
	g_free (filename);
}

inline static gchar *
get_need_mtime_check_filename (TrackerMinerFiles *mf)
{
	GFile *file;
	gchar *prefix, *path;

	file = get_cache_dir (mf);
	prefix = g_file_get_path (file);

	path = g_build_filename (prefix,
	                         NEED_MTIME_CHECK_FILENAME,
	                         NULL);
	g_free (prefix);
	g_object_unref (file);

	return path;
}

/**
 * tracker_miner_files_get_need_mtime_check:
 *
 * Check if the miner-fs was cleanly shutdown or not.
 *
 * Returns: %TRUE if we need to check mtimes for directories against
 * the database on the next start for the miner-fs, %FALSE otherwise.
 **/
gboolean
tracker_miner_files_get_need_mtime_check (TrackerMinerFiles *mf)
{
	gboolean exists;
	gchar *filename;

	filename = get_need_mtime_check_filename (mf);
	exists = g_file_test (filename, G_FILE_TEST_EXISTS);
	g_free (filename);

	/* Existence of the file means we cleanly shutdown before and
	 * don't need to do the mtime check again on this start.
	 */
	return !exists;
}

/**
 * tracker_miner_files_set_need_mtime_check:
 * @needed: a #gboolean
 *
 * If the next start of miner-fs should perform a full mtime check
 * against each directory found and those in the database (for
 * complete synchronisation), then @needed should be #TRUE, otherwise
 * #FALSE.
 *
 * Creates a file in $HOME/.cache/tracker/ if an mtime check is not
 * needed. The idea behind this is that a check is forced if the file
 * is not cleaned up properly on shutdown (i.e. due to a crash or any
 * other uncontrolled shutdown reason).
 **/
void
tracker_miner_files_set_need_mtime_check (TrackerMinerFiles *mf,
					  gboolean           needed)
{
	gboolean already_exists;
	gchar *filename;

	filename = get_need_mtime_check_filename (mf);
	already_exists = g_file_test (filename, G_FILE_TEST_EXISTS);

	/* !needed = add file
	 *  needed = remove file
	 */
	if (!needed && !already_exists) {
		GError *error = NULL;

		/* Create stamp file if not already there */
		if (!g_file_set_contents (filename, PACKAGE_VERSION, -1, &error)) {
			g_warning ("  Could not create file:'%s' failed, %s",
			           filename,
			           error->message);
			g_error_free (error);
		} else {
			g_info ("  Need mtime check file:'%s' created", filename);
		}
	} else if (needed && already_exists) {
		/* Remove stamp file */
		g_info ("  Removing need mtime check file:'%s'", filename);

		if (g_remove (filename)) {
			g_warning ("    Could not remove file:'%s': %m",
			           filename);
		}
	}

	g_free (filename);
}

void
tracker_miner_files_set_mtime_checking (TrackerMinerFiles *mf,
                                        gboolean           mtime_check)
{
	mf->private->mtime_check = mtime_check;
}
