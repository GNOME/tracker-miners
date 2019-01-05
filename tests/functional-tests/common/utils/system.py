#!/usr/bin/python
import os
import subprocess
import shutil
import tempfile
import configuration as cfg

from gi.repository import GObject
from gi.repository import GLib
import time

import options
from dconf import DConfClient

import helpers

TEST_ENV_VARS = {  "LC_COLLATE": "en_GB.utf8" }

REASONABLE_TIMEOUT = 5

class UnableToBootException (Exception):
    pass


class TrackerSystemAbstraction (object):
    def __init__(self, settings=None, ontodir=None):
        self._basedir = None

        self.extractor = None
        self.miner_fs = None
        self.store = None
        self.writeback = None

        self.set_up_environment (settings=settings, ontodir=ontodir)
        self.store = None

    def xdg_data_home(self):
        return os.path.join(self._basedir, 'data')

    def xdg_cache_home(self):
        return os.path.join(self._basedir, 'cache')

    def set_up_environment (self, settings=None, ontodir=None):
        """
        Sets up the XDG_*_HOME variables and make sure the directories exist

        Settings should be a dict mapping schema names to dicts that hold the
        settings that should be changed in those schemas. The contents dicts
        should map key->value, where key is a key name and value is a suitable
        GLib.Variant instance.
        """

        self._basedir = tempfile.mkdtemp()

        self._dirs = {
            "XDG_DATA_HOME" : self.xdg_data_home(),
            "XDG_CACHE_HOME": self.xdg_cache_home()
        }

        for var, directory in self._dirs.items():
            os.makedirs (directory)
            os.makedirs (os.path.join(directory, 'tracker'))
            os.environ [var] = directory

        if ontodir:
            os.environ ["TRACKER_DB_ONTOLOGIES_DIR"] = ontodir

        for var, value in TEST_ENV_VARS.iteritems ():
            os.environ [var] = value

        # Previous loop should have set DCONF_PROFILE to the test location
        if settings is not None:
            self._apply_settings(settings)

    def _apply_settings(self, settings):
        for schema_name, contents in settings.iteritems():
            dconf = DConfClient(schema_name)
            dconf.reset()
            for key, value in contents.iteritems():
                dconf.write(key, value)

    def tracker_store_testing_start (self, confdir=None, ontodir=None):
        """
        Stops any previous instance of the store, calls set_up_environment,
        and starts a new instances of the store
        """
        self.set_up_environment (confdir, ontodir)

        self.store = helpers.StoreHelper ()
        self.store.start ()

    def tracker_store_start (self):
        self.store.start ()

    def tracker_store_restart_with_new_ontologies (self, ontodir):
        self.store.stop ()
        if ontodir:
            helpers.log ("[Conf] Setting %s - %s" % ("TRACKER_DB_ONTOLOGIES_DIR", ontodir))
            os.environ ["TRACKER_DB_ONTOLOGIES_DIR"] = ontodir
        try:
            self.store.start ()
        except GLib.Error:
            raise UnableToBootException ("Unable to boot the store \n(" + str(e) + ")")

    def tracker_store_prepare_journal_replay (self):
        db_location = os.path.join (self.xdg_cache_home(), "tracker", "meta.db")
        os.unlink (db_location)

        lockfile = os.path.join (self.xdg_data_home(), "tracker", "data", ".ismeta.running")
        f = open (lockfile, 'w')
        f.write (" ")
        f.close ()

    def tracker_store_corrupt_dbs (self):
        for filename in ["meta.db", "meta.db-wal"]:
            db_path = os.path.join (self.xdg_cache_home(), "tracker", filename)
            f = open (db_path, "w")
            for i in range (0, 100):
                f.write ("Some stupid content... hohohoho, not a sqlite file anymore!\n")
            f.close ()

    def tracker_store_remove_journal (self):
        db_location = os.path.join (self.xdg_data_home(), "tracker", "data")
        shutil.rmtree (db_location)
        os.mkdir (db_location)

    def tracker_store_remove_dbs (self):
        db_location = os.path.join (self.xdg_cache_home(), "tracker")
        shutil.rmtree (db_location)
        os.mkdir (db_location)

    def tracker_miner_fs_testing_start (self, confdir=None):
        """
        Stops any previous instance of the store and miner, calls set_up_environment,
        and starts a new instance of the store and miner-fs
        """
        self.set_up_environment (confdir, None)

        # Start also the store. DBus autoactivation ignores the env variables.
        self.store = helpers.StoreHelper ()
        self.store.start ()

        self.extractor = helpers.ExtractorHelper ()
        self.extractor.start ()

        self.miner_fs = helpers.MinerFsHelper ()
        self.miner_fs.start ()

    def tracker_writeback_testing_start (self, confdir=None):
        # Start the miner-fs (and store) and then the writeback process
        self.tracker_miner_fs_testing_start (confdir)
        self.writeback = helpers.WritebackHelper ()
        self.writeback.start ()

    def tracker_all_testing_start (self, confdir=None):
        # This will start all miner-fs, store and writeback
        self.tracker_writeback_testing_start (confdir)

    def finish (self):
        """
        Stop all running processes and remove all test data.
        """

        if self.writeback:
            self.writeback.stop ()

        if self.extractor:
            self.extractor.stop ()

        if self.miner_fs:
            self.miner_fs.stop ()

        if self.store:
            self.store.stop ()

        for path in self._dirs.values():
            shutil.rmtree(path)
        os.rmdir(self._basedir)
