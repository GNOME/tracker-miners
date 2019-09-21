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
import configuration as cfg
import unittest as ut

from gi.repository import GLib

import logging
import os
import shutil
from itertools import chain

import trackertestutils.dconf
import trackertestutils.helpers

from minerfshelper import MinerFsHelper

DEFAULT_TEXT = "Some stupid content, to have a test file"

NFO_DOCUMENT = 'http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#Document'

log = logging.getLogger(__name__)

REASONABLE_TIMEOUT = 5


def ensure_dir_exists(dirname):
    if not os.path.exists(dirname):
        os.makedirs(dirname)


class CommonTrackerMinerTest(ut.TestCase):
    def setUp(self):
        self.workdir = cfg.create_monitored_test_dir()

        self.indexed_dir = os.path.join(self.workdir, 'test-monitored')

        # It's important that this directory exists BEFORE we start Tracker:
        # it won't monitor an indexing root for changes if it doesn't exist,
        # it'll silently ignore it instead. See the tracker_crawler_start()
        # function.
        ensure_dir_exists(self.indexed_dir)

        try:
            extra_env = cfg.test_environment(self.workdir)
            extra_env['LANG'] = 'en_GB.utf8'

            self.sandbox = trackertestutils.helpers.TrackerDBusSandbox(
                dbus_daemon_config_file=cfg.TEST_DBUS_DAEMON_CONFIG_FILE, extra_env=extra_env)

            self.sandbox.start()

            try:
                settings = {
                    'org.freedesktop.Tracker.Store': {
                        'graphupdated-delay': GLib.Variant('i', 100)
                    },
                    'org.freedesktop.Tracker.Miner.Files': {
                        'enable-writeback': GLib.Variant.new_boolean(False),
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

                # We must create the test data before the miner does its
                # initial crawl, or it may miss some files due
                # https://gitlab.gnome.org/GNOME/tracker-miners/issues/79.
                monitored_files = self.create_test_data()

                self.miner_fs = MinerFsHelper(
                    self.sandbox.get_connection())
                self.miner_fs.start()
                self.miner_fs.start_watching_progress()

                for tf in monitored_files:
                    self.tracker.await_resource_inserted(NFO_DOCUMENT, url=self.uri(tf))

                # We reset update-tracking, so that updates for data created in the
                # fixture can't be mixed up with updates created by the test case.
                self.tracker.stop_watching_updates()
                self.tracker.start_watching_updates()
            except Exception:
                self.sandbox.stop()
                raise
        except Exception:
            self.remove_test_data()
            cfg.remove_monitored_test_dir(self.workdir)
            raise

    def tearDown(self):
        self.sandbox.stop()
        self.remove_test_data()
        cfg.remove_monitored_test_dir(self.workdir)

    def path(self, filename):
        return os.path.join(self.workdir, filename)

    def uri(self, filename):
        return "file://" + os.path.join(self.workdir, filename)

    def create_test_data(self):
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
            ensure_dir_exists(os.path.dirname(testfile))
            with open(testfile, 'w') as f:
                f.write(DEFAULT_TEXT)

        return monitored_files

    def remove_test_data(self):
        try:
            shutil.rmtree(os.path.join(self.workdir, 'test-monitored'))
            shutil.rmtree(os.path.join(self.workdir, 'test-no-monitored'))
        except Exception as e:
            log.warning("Failed to remove temporary data dir: %s", e)

    def assertResourceExists(self, urn):
        if self.tracker.ask("ASK { <%s> a rdfs:Resource }" % urn) == False:
            self.fail("Resource <%s> does not exist" % urn)

    def assertResourceMissing(self, urn):
        if self.tracker.ask("ASK { <%s> a rdfs:Resource }" % urn) == True:
            self.fail("Resource <%s> should not exist" % urn)


class CommonTrackerMinerFTSTest (CommonTrackerMinerTest):
    """
    Superclass to share methods. Shouldn't be run by itself.
    """

    def prepare_directories(self):
        # Override content from the base class
        pass

    def setUp(self):
        super(CommonTrackerMinerFTSTest, self).setUp()

        self.testfile = "test-monitored/miner-fts-test.txt"

    def set_text(self, text):
        exists = os.path.exists(self.path(self.testfile))

        f = open(self.path(self.testfile), "w")
        f.write(text)
        f.close()

        if exists:
            subject_id = self.tracker.get_resource_id(self.uri(self.testfile))
            self.tracker.await_property_changed(NFO_DOCUMENT,
                                                subject_id=subject_id, property_uri='nie:plainTextContent')
        else:
            self.tracker.await_resource_inserted(
                rdf_class=NFO_DOCUMENT, url=self.uri(self.testfile),
                required_property='nie:plainTextContent')

        self.tracker.reset_graph_updates_tracking()

    def search_word(self, word):
        """
        Return list of URIs with the word in them
        """
        log.info("Search for: %s", word)
        results = self.tracker.query("""
                SELECT ?url WHERE {
                  ?u a nfo:TextDocument ;
                      nie:url ?url ;
                      fts:match '%s'.
                 }
                 """ % (word))
        return [r[0] for r in results]

    def basic_test(self, text, word):
        """
        Save the text on the testfile, search the word
        and assert the testfile is only result.

        Be careful with the default contents of the text files
        ( see minertest.py DEFAULT_TEXT )
        """
        self.set_text(text)
        results = self.search_word(word)
        self.assertEqual(len(results), 1)
        self.assertIn(self.uri(self.testfile), results)

    def _query_id(self, uri):
        query = "SELECT tracker:id(?urn) WHERE { ?urn nie:url \"%s\". }" % uri
        result = self.tracker.query(query)
        assert len(result) == 1
        return int(result[0][0])
