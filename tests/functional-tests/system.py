#
# Copyright (C) 2010, Nokia (ivan.frade@nokia.com)
# Copyright (C) 2019, Sam Thursfield (sam@afuera.me.uk)
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the
# Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA  02110-1301, USA.
#


import logging
import os
import shutil
import tempfile

from gi.repository import Gio
from gi.repository import GLib

import trackertestutils.dconf
import trackertestutils.helpers
import configuration as cfg

TEST_ENV_VARS = {"LC_COLLATE": "en_GB.utf8"}

REASONABLE_TIMEOUT = 5

log = logging.getLogger(__name__)


class WakeupCycleTimeoutException(RuntimeError):
    pass


class UnableToBootException (Exception):
    pass


class MinerFsHelper (trackertestutils.helpers.Helper):

    MINERFS_BUSNAME = "org.freedesktop.Tracker1.Miner.Files"
    MINERFS_OBJ_PATH = "/org/freedesktop/Tracker1/Miner/Files"
    MINER_IFACE = "org.freedesktop.Tracker1.Miner"
    MINERFS_INDEX_OBJ_PATH = "/org/freedesktop/Tracker1/Miner/Files/Index"
    MINER_INDEX_IFACE = "org.freedesktop.Tracker1.Miner.Files.Index"

    def __init__(self, process_path):
        trackertestutils.helpers.Helper.__init__(self, "tracker-miner-fs", self.MINERFS_BUSNAME, process_path)
        self._progress_handler_id = 0
        self._wakeup_count = 0
        self._previous_status = None
        self._target_wakeup_count = None

    def start(self):
        trackertestutils.helpers.Helper.start(self, ['--initial-sleep=0'])

        self.miner_fs = Gio.DBusProxy.new_sync(
            self.bus, Gio.DBusProxyFlags.DO_NOT_AUTO_START, None,
            self.MINERFS_BUSNAME, self.MINERFS_OBJ_PATH, self.MINER_IFACE)
        self.index = Gio.DBusProxy.new_sync(
            self.bus, Gio.DBusProxyFlags.DO_NOT_AUTO_START, None,
            self.MINERFS_BUSNAME, self.MINERFS_INDEX_OBJ_PATH, self.MINER_INDEX_IFACE)

        def signal_handler(proxy, sender_name, signal_name, parameters):
            if signal_name == 'Progress':
                self._progress_cb(*parameters.unpack())

        self._progress_handler_id = self.miner_fs.connect('g-signal', signal_handler)

    def stop(self):
        trackertestutils.helpers.Helper.stop(self)

        if self._progress_handler_id != 0:
            self.miner_fs.disconnect(self._progress_handler_id)

    def _progress_cb(self, status, progress, remaining_time):
        if self._previous_status is None:
            self._previous_status = status
        if self._previous_status != 'Idle' and status == 'Idle':
            self._wakeup_count += 1

        if self._target_wakeup_count is not None and self._wakeup_count >= self._target_wakeup_count:
            self.loop.quit()

    def wakeup_count(self):
        """Return the number of wakeup-to-idle cycles the miner-fs completed."""
        return self._wakeup_count

    def await_wakeup_count(self, target_wakeup_count, timeout=REASONABLE_TIMEOUT):
        """Block until the miner has completed N wakeup-and-idle cycles.

        This function is for use by miner-fs tests that should trigger an
        operation in the miner, but which do not cause a new resource to be
        inserted. These tests can instead wait for the status to change from
        Idle to Processing... and then back to Idle.

        The miner may change its status any number of times, but you can use
        this function reliably as follows:

            wakeup_count = miner_fs.wakeup_count()
            # Trigger a miner-fs operation somehow ...
            miner_fs.await_wakeup_count(wakeup_count + 1)
            # The miner has probably finished processing the operation now.

        If the timeout is reached before enough wakeup cycles complete, an
        exception will be raised.

        """

        assert self._target_wakeup_count is None

        if self._wakeup_count >= target_wakeup_count:
            log.debug("miner-fs wakeup count is at %s (target is %s). No need to wait", self._wakeup_count, target_wakeup_count)
        else:
            def _timeout_cb():
                raise WakeupCycleTimeoutException()
            timeout_id = GLib.timeout_add_seconds(timeout, _timeout_cb)

            log.debug("Waiting for miner-fs wakeup count of %s (currently %s)", target_wakeup_count, self._wakeup_count)
            self._target_wakeup_count = target_wakeup_count
            self.loop.run_checked()

            self._target_wakeup_count = None
            GLib.source_remove(timeout_id)

    def index_file(self, uri):
        return self.index.IndexFile('(s)', uri)


class ExtractorHelper (trackertestutils.helpers.Helper):

    BUSNAME = "org.freedesktop.Tracker1.Miner.Extract"

    def __init__(self, process_path):
        trackertestutils.helpers.Helper.__init__(self, "tracker-extract", self.BUSNAME, process_path)


class WritebackHelper (trackertestutils.helpers.Helper):

    BUSNAME = "org.freedesktop.Tracker1.Writeback"

    def __init__(self, process_path):
        trackertestutils.helpers.Helper.__init__(self, "tracker-writeback", self.BUSNAME, process_path)


class TrackerSystemAbstraction (object):
    def __init__(self, settings=None, ontodir=None):
        self._basedir = None

        self.extractor = None
        self.miner_fs = None
        self.store = None
        self.writeback = None

        self.set_up_environment(settings=settings, ontodir=ontodir)
        self.store = None

    def xdg_data_home(self):
        return os.path.join(self._basedir, 'data')

    def xdg_cache_home(self):
        return os.path.join(self._basedir, 'cache')

    def set_up_environment(self, settings=None, ontodir=None):
        """
        Sets up the XDG_*_HOME variables and make sure the directories exist

        Settings should be a dict mapping schema names to dicts that hold the
        settings that should be changed in those schemas. The contents dicts
        should map key->value, where key is a key name and value is a suitable
        GLib.Variant instance.
        """

        self._basedir = tempfile.mkdtemp()

        self._dirs = {
            "XDG_DATA_HOME": self.xdg_data_home(),
            "XDG_CACHE_HOME": self.xdg_cache_home()
        }

        for var, directory in list(self._dirs.items()):
            os.makedirs(directory)
            os.makedirs(os.path.join(directory, 'tracker'))
            os.environ[var] = directory

        if ontodir:
            os.environ["TRACKER_DB_ONTOLOGIES_DIR"] = ontodir

        for var, value in TEST_ENV_VARS.items():
            os.environ[var] = value

        # Previous loop should have set DCONF_PROFILE to the test location
        if settings is not None:
            self._apply_settings(settings)

    def _apply_settings(self, settings):
        for schema_name, contents in settings.items():
            dconf = trackertestutils.dconf.DConfClient(schema_name)
            dconf.reset()
            for key, value in contents.items():
                dconf.write(key, value)

    def tracker_store_testing_start(self, confdir=None, ontodir=None):
        """
        Stops any previous instance of the store, calls set_up_environment,
        and starts a new instances of the store
        """
        self.set_up_environment(confdir, ontodir)

        self.store = trackertestutils.helpers.StoreHelper(cfg.TRACKER_STORE_PATH)
        self.store.start()

    def tracker_store_start(self):
        self.store.start()

    def tracker_store_restart_with_new_ontologies(self, ontodir):
        self.store.stop()
        if ontodir:
            log.debug("[Conf] Setting %s - %s", "TRACKER_DB_ONTOLOGIES_DIR", ontodir)
            os.environ["TRACKER_DB_ONTOLOGIES_DIR"] = ontodir
        try:
            self.store.start()
        except GLib.Error as e:
            raise UnableToBootException("Unable to boot the store \n(" + str(e) + ")")

    def tracker_store_prepare_journal_replay(self):
        db_location = os.path.join(self.xdg_cache_home(), "tracker", "meta.db")
        os.unlink(db_location)

        lockfile = os.path.join(self.xdg_data_home(), "tracker", "data", ".ismeta.running")
        f = open(lockfile, 'w')
        f.write(" ")
        f.close()

    def tracker_store_corrupt_dbs(self):
        for filename in ["meta.db", "meta.db-wal"]:
            db_path = os.path.join(self.xdg_cache_home(), "tracker", filename)
            f = open(db_path, "w")
            for i in range(0, 100):
                f.write("Some stupid content... hohohoho, not a sqlite file anymore!\n")
            f.close()

    def tracker_store_remove_journal(self):
        db_location = os.path.join(self.xdg_data_home(), "tracker", "data")
        shutil.rmtree(db_location)
        os.mkdir(db_location)

    def tracker_store_remove_dbs(self):
        db_location = os.path.join(self.xdg_cache_home(), "tracker")
        shutil.rmtree(db_location)
        os.mkdir(db_location)

    def tracker_miner_fs_testing_start(self, confdir=None):
        """
        Stops any previous instance of the store and miner, calls set_up_environment,
        and starts a new instance of the store and miner-fs
        """
        self.set_up_environment(confdir, None)

        # Start also the store. DBus autoactivation ignores the env variables.
        self.store = trackertestutils.helpers.StoreHelper(cfg.TRACKER_STORE_PATH)
        self.store.start()

        self.extractor = ExtractorHelper(cfg.TRACKER_EXTRACT_PATH)
        self.extractor.start()

        self.miner_fs = MinerFsHelper(cfg.TRACKER_MINER_FS_PATH)
        self.miner_fs.start()

    def tracker_writeback_testing_start(self, confdir=None):
        # Start the miner-fs (and store) and then the writeback process
        self.tracker_miner_fs_testing_start(confdir)
        self.writeback = WritebackHelper(cfg.TRACKER_WRITEBACK_PATH)
        self.writeback.start()

    def tracker_all_testing_start(self, confdir=None):
        # This will start all miner-fs, store and writeback
        self.tracker_writeback_testing_start(confdir)

    def finish(self):
        """
        Stop all running processes and remove all test data.
        """

        if self.writeback:
            self.writeback.stop()

        if self.extractor:
            self.extractor.stop()

        if self.miner_fs:
            self.miner_fs.stop()

        if self.store:
            self.store.stop()

        for path in list(self._dirs.values()):
            shutil.rmtree(path)
        os.rmdir(self._basedir)
