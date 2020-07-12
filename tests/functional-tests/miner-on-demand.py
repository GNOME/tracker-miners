# Copyright (C) 2020, Sam Thursfield (sam@afuera.me.uk)
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

from gi.repository import Gio, GLib, GObject

import logging
import pathlib

import trackertestutils

import configuration
import helpers
import fixtures

log = logging.getLogger(__file__)


class MinerOnDemandTest(fixtures.TrackerMinerTest):
    """
    Tests on-demand indexing and signals that report indexing status.

    This covers the IndexFile and IndexFileForProcess D-Bus methods, and the
    FileProcessed signal.
    """

    def setUp(self):
        fixtures.TrackerMinerTest.setUp(self)

    def create_test_file(self, path):
        testfile = pathlib.Path(self.workdir).joinpath(path)
        testfile.parent.mkdir(parents=True, exist_ok=True)
        testfile.write_text("Hello, I'm a test file.")
        return testfile

    def create_test_directory_tree(self):
        testdir = pathlib.Path(self.workdir).joinpath('test-not-monitored')
        for dirname in ['a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j']:
            subdir = testdir.joinpath(dirname)
            subdir.mkdir(parents=True)
            subdir_file = subdir.joinpath('content.txt')
            subdir_file.write_text("Hello, I'm a test file in a subdirectory")
        return testdir

    def test_index_file_basic(self):
        """
        Test on-demand indexing of a file.
        """

        testfile = self.create_test_file('test-not-monitored/on-demand.txt')
        self.assertFileNotIndexed(testfile.as_uri())
        with self.await_document_inserted(testfile):
            with self.extractor.await_file_processed(testfile):
                with self.miner_fs.await_file_processed(testfile):
                    self.miner_fs.index_file(testfile.as_uri())
        self.assertFileIndexed(testfile.as_uri())

    def test_index_file_not_found(self):
        """
        On-demand indexing of a file, but it's missing.
        """

        self.assertFileNotIndexed('file:///test-missing')
        with self.assertRaises(GLib.GError) as e:
            self.miner_fs.index_file('file:///test-missing')
        assert e.exception.message.startswith('GDBus.Error:org.freedesktop.Tracker.Miner.Files.Index.Error.FileNotFound:')

    def await_failsafe_marker_inserted(self, graph, path, timeout=configuration.AWAIT_TIMEOUT):
        url = path.as_uri()
        expected = [
            f'a rdfs:Resource. <{url}> nie:dataSource tracker:extractor-failure-data-source'
        ]

        return self.tracker.await_insert(graph, '; '.join(expected), timeout=timeout)

    def test_index_extractor_error(self):
        """
        On-demand indexing of a file, but the extractor can't extract it.
        """

        # This file will be processed by the mp3 or gstreamer extractor due to
        # its extension, but it's not a valid MP3.
        testfile = self.create_test_file('test-not-monitored/invalid.mp3')

        with self.extractor.await_file_processed(testfile, False, timeout=configuration.AWAIT_TIMEOUT):
            self.miner_fs.index_file(testfile.as_uri())

    def test_index_directory_basic(self):
        """
        Test on-demand indexing of a directory with different types of file.

        One file is eligible for indexing, the others are not for various
        reasons.
        """

        testdir = pathlib.Path(self.workdir).joinpath('test-not-monitored')

        test_eligible = self.create_test_file('test-not-monitored/eligible.txt')
        test_not_eligible_tmp = self.create_test_file('test-not-monitored/not-eligible.tmp')
        test_not_eligible_hidden = self.create_test_file('test-not-monitored/.not-eligible')

        test_not_eligible_dir = testdir.joinpath('.not-eligible-dir')
        test_not_eligible_dir.mkdir()

        expected = [
            helpers.FileProcessedResult(test_eligible, True),
            helpers.FileProcessedResult(test_not_eligible_dir, False),
            helpers.FileProcessedResult(test_not_eligible_hidden, False),
            helpers.FileProcessedResult(test_not_eligible_tmp, False),
        ]

        with self.miner_fs.await_files_processed(expected):
            self.miner_fs.index_file(testdir.as_uri())

    def test_index_for_process_miner_shutdown(self):
        """
        Test on-demand indexing linked to a D-Bus connection.

        Similar to test_index_for_process, only this tests what happens in the
        unusual case that the tracker-miner-fs process terminates before the
        process that triggered indexing. Data should be removed from the
        store in the same way.
        """

        testdir = self.create_test_directory_tree()

        process_conn = self.sandbox.daemon.create_connection()
        process_miner_fs = helpers.MinerFsHelper(process_conn)

        with self.await_document_inserted(testdir.joinpath('g/content.txt')) as resource:
            process_miner_fs.index_file_for_process(testdir.as_uri())

        self.assertFileIndexed(testdir.joinpath('g/content.txt'))

        self.sandbox.stop_miner_fs()

        # This query should cause the miner-fs to restart, and the content
        # should now be gone.
        log.info("Checking that removal was processed.")
        self.assertFileNotIndexed(testdir.joinpath('g/content.txt'))


if __name__ == "__main__":
    fixtures.tracker_test_main()
