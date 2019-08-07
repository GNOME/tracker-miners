#!/usr/bin/env python3
#
# Copyright (C) 2010, Nokia <ivan.frade@nokia.com>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.
#
from common.utils import configuration as cfg
from common.utils.system import TrackerSystemAbstraction
import unittest as ut

from gi.repository import GLib

import logging
import os
import shutil
import time

# Copy rate, 10KBps (1024b/100ms)
SLOWCOPY_RATE = 1024

log = logging.getLogger(__name__)


class CommonTrackerApplicationTest (ut.TestCase):

    def get_urn_count_by_url(self, url):
        select = """
        SELECT ?u WHERE { ?u nie:url \"%s\" }
        """ % (url)
        return len(self.tracker.query(select))

    def get_test_image(self):
        TEST_IMAGE = "test-image-1.jpg"
        return TEST_IMAGE

    def get_test_video(self):
        TEST_VIDEO = "test-video-1.mp4"
        return TEST_VIDEO

    def get_test_music(self):
        TEST_AUDIO = "test-music-1.mp3"
        return TEST_AUDIO

    def get_data_dir(self):
        return self.datadir

    def get_dest_dir(self):
        return self.workdir

    def slowcopy_file_fd(self, src, fdest, rate=SLOWCOPY_RATE):
        """
        @rate: bytes per 100ms
        """
        log.debug("Copying slowly\n '%s' to\n '%s'", src, fdest.name)
        fsrc = open(src, 'rb')
        buffer_ = fsrc.read(rate)
        while (buffer_ != b""):
            fdest.write(buffer_)
            time.sleep(0.1)
            buffer_ = fsrc.read(rate)
        fsrc.close()

    def slowcopy_file(self, src, dst, rate=SLOWCOPY_RATE):
        """
        @rate: bytes per 100ms
        """
        fdest = open(dst, 'wb')
        self.slowcopy_file_fd(src, fdest, rate)
        fdest.close()

    @classmethod
    def setUp(self):
        self.workdir = cfg.create_monitored_test_dir()

        index_dirs = [self.workdir]

        CONF_OPTIONS = {
            cfg.DCONF_MINER_SCHEMA: {
                'index-recursive-directories': GLib.Variant.new_strv(index_dirs),
                'index-single-directories': GLib.Variant.new_strv([]),
                'index-optical-discs': GLib.Variant.new_boolean(False),
                'index-removable-devices': GLib.Variant.new_boolean(False),
            }
        }

        # Use local directory if available. Installation otherwise.
        if os.path.exists(os.path.join(os.getcwd(),
                                       "test-apps-data")):
            self.datadir = os.path.join(os.getcwd(),
                                        "test-apps-data")
        else:
            self.datadir = os.path.join(cfg.DATADIR,
                                        "tracker-tests",
                                        "test-apps-data")

        self.system = TrackerSystemAbstraction()
        self.system.tracker_all_testing_start(CONF_OPTIONS)
        self.tracker = self.system.store

    @classmethod
    def tearDown(self):
        self.system.finish()

        cfg.remove_monitored_test_dir(self.workdir)
