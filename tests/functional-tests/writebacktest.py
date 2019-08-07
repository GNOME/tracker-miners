#
# Copyright (C) 2010, Nokia (ivan.frade@nokia.com)
# Copyright (C) 2019, Sam Thursfield <sam@afuera.me.uk>
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

from gi.repository import GLib

import logging
import os
import pathlib
import shutil
import time
import unittest as ut

import trackertestutils.dconf
import trackertestutils.helpers

import configuration as cfg
from minerfshelper import MinerFsHelper


TEST_FILE_JPEG = "writeback-test-1.jpeg"
TEST_FILE_TIFF = "writeback-test-2.tif"
TEST_FILE_PNG = "writeback-test-4.png"

log = logging.getLogger(__name__)


def ensure_dir_exists(dirname):
    if not os.path.exists(dirname):
        os.makedirs(dirname)


class CommonTrackerWritebackTest (ut.TestCase):
    """
    Superclass to share methods. Shouldn't be run by itself.
    Start all processes including writeback, miner pointing to WRITEBACK_TMP_DIR
    """

    def setUp(self):
        self.workdir = cfg.create_monitored_test_dir()

        self.indexed_dir = os.path.join(self.workdir, 'test-monitored')

        # It's important that this directory exists BEFORE we start Tracker:
        # it won't monitor an indexing root for changes if it doesn't exist,
        # it'll silently ignore it instead. See the tracker_crawler_start()
        # function.
        ensure_dir_exists(self.indexed_dir)

        try:
            self.extra_env = cfg.test_environment(self.workdir)

            self.sandbox = trackertestutils.helpers.TrackerDBusSandbox(
                dbus_daemon_config_file=cfg.TEST_DBUS_DAEMON_CONFIG_FILE, extra_env=self.extra_env)

            self.sandbox.start()

            try:
                settings = {
                    'org.freedesktop.Tracker.Store': {
                        'graphupdated-delay': GLib.Variant('i', 100)
                    },
                    'org.freedesktop.Tracker.Miner.Files': {
                        'index-recursive-directories': GLib.Variant.new_strv([self.indexed_dir]),
                        'index-single-directories': GLib.Variant.new_strv([]),
                        'index-optical-discs': GLib.Variant.new_boolean(False),
                        'index-removable-devices': GLib.Variant.new_boolean(False),
                        'throttle': GLib.Variant.new_int32(5),
                    }
                }

                for schema_name, contents in settings.items():
                    dconf = trackertestutils.dconf.DConfClient(self.sandbox)
                    for key, value in contents.items():
                        dconf.write(schema_name, key, value)

                self.tracker = trackertestutils.helpers.StoreHelper(
                    self.sandbox.get_connection())
                self.tracker.start_and_wait_for_ready()
                self.tracker.start_watching_updates()

                self.miner_fs = MinerFsHelper(
                    self.sandbox.get_connection())
                self.miner_fs.start()
            except Exception:
                self.sandbox.stop()
                raise
        except Exception:
            self.remove_test_data()
            cfg.remove_monitored_test_dir(self.workdir)
            raise

    def tearDown(self):
        self.sandbox.stop()

        for test_file in pathlib.Path(self.indexed_dir).iterdir():
            test_file.unlink()

        cfg.remove_monitored_test_dir(self.workdir)

    def datadir_path(self, filename):
        """Returns the full path to a writeback test file."""
        datadir = os.path.join(os.path.dirname(__file__), 'test-writeback-data')
        return pathlib.Path(os.path.join(datadir, filename))

    def prepare_test_file(self, path, expect_mime_type, expect_property):
        """Copies a file into the test working directory.

        The function waits until the file has been seen by the Tracker
        miner before returning.

        """
        log.debug("Copying %s -> %s", path, self.indexed_dir)
        shutil.copy(path, self.indexed_dir)

        output_path = pathlib.Path(os.path.join(self.indexed_dir, os.path.basename(path)))

        # Make sure a resource has been crawled by the FS miner and by
        # tracker-extract. The extractor adds nie:contentCreated for
        # image resources, so know once this property is set the
        # extraction is complete.
        self.tracker.await_resource_inserted(expect_mime_type, url=output_path.as_uri(), required_property=expect_property)
        return output_path

    def prepare_test_audio(self, filename):
        return self.prepare_test_file(filename, 'http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#Audio', 'nfo:duration')

    def prepare_test_image(self, filename):
        return self.prepare_test_file(filename, 'http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#Image', 'nfo:width')

    def uri(self, filename):
        return pathlib.Path(filename).as_uri()

    def get_mtime(self, filename):
        return os.stat(filename).st_mtime

    def wait_for_file_change(self, filename, initial_mtime):
        start = time.time()
        while time.time() < start + 5:
            mtime = os.stat(filename).st_mtime
            if mtime > initial_mtime:
                return
            time.sleep(0.2)

        raise Exception(
            "Timeout waiting for %s to be updated (mtime has not changed)" %
            filename)
