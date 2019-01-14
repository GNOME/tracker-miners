#!/usr/bin/python

# Copyright (C) 2010, Nokia (ivan.frade@nokia.com)
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

from common.utils.system import TrackerSystemAbstraction
import shutil
import unittest as ut
import os
from common.utils import configuration as cfg
from common.utils.helpers import log
import time

TEST_FILE_JPEG = "writeback-test-1.jpeg"
TEST_FILE_TIFF = "writeback-test-2.tif"
TEST_FILE_PNG = "writeback-test-4.png"

NFO_IMAGE = 'http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#Image'


class CommonTrackerWritebackTest (ut.TestCase):
    """
    Superclass to share methods. Shouldn't be run by itself.
    Start all processes including writeback, miner pointing to WRITEBACK_TMP_DIR
    """

    def __prepare_directories (self):
        if (os.path.exists (os.getcwd() + "/test-writeback-data")):
            # Use local directory if available
            datadir = os.getcwd() + "/test-writeback-data"
        else:
            datadir = os.path.join (cfg.DATADIR, "tracker-tests",
                                    "test-writeback-data")

        for testfile in [TEST_FILE_JPEG, TEST_FILE_PNG,TEST_FILE_TIFF]:
            origin = os.path.join (datadir, testfile)
            log ("Copying %s -> %s" % (origin, self.workdir))
            shutil.copy (origin, self.workdir)


    def setUp(self):
        self.workdir = cfg.create_monitored_test_dir()

        index_dirs = [self.workdir]

        CONF_OPTIONS = {
            cfg.DCONF_MINER_SCHEMA: {
                'index-recursive-directories': GLib.Variant.new_strv(index_dirs),
                'index-single-directories': GLib.Variant.new_strv([]),
                'index-optical-discs': GLib.Variant.new_boolean(False),
                'index-removable-devices': GLib.Variant.new_boolean(False),
            },
            'org.freedesktop.Tracker.Store': {
                'graphupdated-delay': GLib.Variant.new_int32(100)
            }
        }

        self.__prepare_directories ()

        self.system = TrackerSystemAbstraction ()
        self.system.tracker_writeback_testing_start (CONF_OPTIONS)

        def await_resource_extraction(url):
            # Make sure a resource has been crawled by the FS miner and by
            # tracker-extract. The extractor adds nie:contentCreated for
            # image resources, so know once this property is set the
            # extraction is complete.
            self.system.store.await_resource_inserted(NFO_IMAGE, url=url, required_property='nfo:width')

        await_resource_extraction (self.get_test_filename_jpeg())
        await_resource_extraction (self.get_test_filename_tiff())
        await_resource_extraction (self.get_test_filename_png())

        self.tracker = self.system.store
        self.extractor = self.system.extractor

    def tearDown (self):
        self.system.finish ()

        for testfile in [TEST_FILE_JPEG, TEST_FILE_PNG,TEST_FILE_TIFF]:
            os.remove(os.path.join(self.workdir, testfile))

        cfg.remove_monitored_test_dir(self.workdir)

    def uri (self, filename):
        return "file://" + os.path.join (self.workdir, filename)

    def get_test_filename_jpeg (self):
        return self.uri (TEST_FILE_JPEG)

    def get_test_filename_tiff (self):
        return self.uri (TEST_FILE_TIFF)

    def get_test_filename_png (self):
        return self.uri (TEST_FILE_PNG)

    def get_mtime (self, filename):
        return os.stat(filename).st_mtime

    def wait_for_file_change (self, filename, initial_mtime):
        start = time.time()
        while time.time() < start + 5:
            mtime = os.stat(filename).st_mtime
            if mtime > initial_mtime:
                return
            time.sleep(0.2)

        raise Exception(
            "Timeout waiting for %s to be updated (mtime has not changed)" %
            filename)
