#!/usr/bin/env python
#
# Copyright (C) 2010, Nokia <ivan.frade@nokia.com>
# Copyright (C) 2018, Sam Thursfield <sam@afuera.me.uk>
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
from common.utils.helpers import log
from common.utils.system import TrackerSystemAbstraction
import unittest2 as ut

from gi.repository import GLib

import os
import shutil
import tempfile
from itertools import chain

DEFAULT_TEXT = "Some stupid content, to have a test file"


def ensure_dir_exists(dirname):
    if not os.path.exists(dirname):
        os.makedirs(dirname)


class CommonTrackerMinerTest (ut.TestCase):
    def setUp (self):
        ensure_dir_exists(cfg.TEST_MONITORED_TMP_DIR)

        # It's important that this directory is NOT inside /tmp, because
        # monitoring files in /tmp usually doesn't work.
        self.datadir = tempfile.mkdtemp(dir=cfg.TEST_MONITORED_TMP_DIR)

        self.indexed_dir = os.path.join(self.datadir, 'test-monitored')

        # It's important that this directory exists BEFORE we start Tracker:
        # it won't monitor an indexing root for changes if it doesn't exist,
        # it'll silently ignore it instead. See the tracker_crawler_start()
        # function.
        ensure_dir_exists(self.indexed_dir)

        self.system = TrackerSystemAbstraction (
            settings={
                'org.freedesktop.Tracker.Store': {
                    'graphupdated-delay': GLib.Variant('i', 100)
                }
            }
        )

        config = {
            cfg.DCONF_MINER_SCHEMA: {
                'index-recursive-directories': GLib.Variant.new_strv([self.indexed_dir]),
                'index-single-directories': GLib.Variant.new_strv([]),
                'index-optical-discs': GLib.Variant.new_boolean(False),
                'index-removable-devices': GLib.Variant.new_boolean(False),
                'throttle': GLib.Variant.new_int32(5),
            }
        }

        try:
            self.system.tracker_miner_fs_testing_start (config)
        except RuntimeError as e:
            self.fail(e)

        self.tracker = self.system.store

        try:
            self.create_test_data ()
            self.tracker.reset_graph_updates_tracking ()
        except Exception as e:
            self.tearDown ()
            raise

    def tearDown (self):
        self.remove_test_data ()
        self.system.tracker_miner_fs_testing_stop ()

    def path (self, filename):
        return os.path.join (self.datadir, filename)

    def uri (self, filename):
        return "file://" + os.path.join (self.datadir, filename)

    def create_test_data (self):
        monitored_files = [
            'test-monitored/file1.txt',
            'test-monitored/dir1/file2.txt',
            'test-monitored/dir1/dir2/file3.txt'
        ]

        unmonitored_files = [
            'test-no-monitored/file0.txt'
        ]

        for tf in chain(monitored_files, unmonitored_files):
            testfile = self.path(tf)
            ensure_dir_exists (os.path.dirname(testfile))
            with open (testfile, 'w') as f:
                f.write (DEFAULT_TEXT)

        for tf in monitored_files:
            self.tracker.await_resource_inserted(
                'nfo:TextDocument', url=self.uri(tf))

    def remove_test_data(self):
        try:
            shutil.rmtree(self.datadir)
        except Exception as e:
            log("Failed to remove temporary data dir: %s" % e)

    def assertResourceExists (self, urn):
        if self.tracker.ask ("ASK { <%s> a rdfs:Resource }" % urn) == False:
            self.fail ("Resource <%s> does not exist" % urn)

    def assertResourceMissing (self, urn):
        if self.tracker.ask ("ASK { <%s> a rdfs:Resource }" % urn) == True:
            self.fail ("Resource <%s> should not exist" % urn)


class CommonTrackerMinerFTSTest (CommonTrackerMinerTest):
    """
    Superclass to share methods. Shouldn't be run by itself.
    """
    def prepare_directories (self):
        # Override content from the base class
        pass

    def setUp (self):
        super(CommonTrackerMinerFTSTest, self).setUp()

        self.testfile = "test-monitored/miner-fts-test.txt"

    def set_text (self, text):
        exists = os.path.exists(self.path(self.testfile))

        f = open (self.path (self.testfile), "w")
        f.write (text)
        f.close ()

        if exists:
            subject_id = self.tracker.get_resource_id(self.uri(self.testfile))
            self.tracker.await_property_changed(
                subject_id=subject_id, property_uri='nie:plainTextContent')
        else:
            self.tracker.await_resource_inserted(
                rdf_class='nfo:Document', url=self.uri(self.testfile),
                required_property='nie:plainTextContent')

        self.tracker.reset_graph_updates_tracking()

    def search_word (self, word):
        """
        Return list of URIs with the word in them
        """
        log ("Search for: %s" % word)
        results = self.tracker.query ("""
                SELECT ?url WHERE {
                  ?u a nfo:TextDocument ;
                      nie:url ?url ;
                      fts:match '%s'.
                 }
                 """ % (word))
        return [r[0] for r in results]

    def basic_test (self, text, word):
        """
        Save the text on the testfile, search the word
        and assert the testfile is only result.

        Be careful with the default contents of the text files
        ( see common/utils/minertest.py DEFAULT_TEXT )
        """
        self.set_text (text)
        results = self.search_word (word)
        self.assertEquals (len (results), 1)
        self.assertIn ( self.uri (self.testfile), results)

    def _query_id (self, uri):
        query = "SELECT tracker:id(?urn) WHERE { ?urn nie:url \"%s\". }" % uri
        result = self.tracker.query (query)
        assert len (result) == 1
        return int (result[0][0])
