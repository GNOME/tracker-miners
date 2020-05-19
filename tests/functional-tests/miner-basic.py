# Copyright (C) 2010, Nokia (ivan.frade@nokia.com)
# Copyright (C) 2019-2020, Sam Thursfield (sam@afuera.me.uk)
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
# TODO:
#     These tests are for files... we need to write them for folders!
#

"""
Monitor a test directory and copy/move/remove/update files and folders there.
Check the basic data of the files is updated accordingly in tracker.
"""


import itertools
import logging
import os
import shutil
import time
import unittest as ut

import configuration as cfg
import fixtures


log = logging.getLogger(__name__)

DEFAULT_TEXT = "Some stupid content, to have a test file"


class MinerCrawlTest(fixtures.TrackerMinerTest):
    """
    Tests crawling and monitoring of configured content locations.
    """

    def setUp(self):
        # We must create the test data before the miner does its
        # initial crawl, or it may miss some files due
        # https://gitlab.gnome.org/GNOME/tracker-miners/issues/79.
        monitored_files = self.create_test_data()

        try:
            # Start the miner.
            fixtures.TrackerMinerTest.setUp(self)

            for tf in monitored_files:
                url = self.uri(tf)
                self.tracker.ensure_resource(fixtures.DOCUMENTS_GRAPH,
                                             f"a nfo:Document ; nie:isStoredAs <{url}>")
        except Exception:
            cfg.remove_monitored_test_dir(self.workdir)
            raise

        logging.info("%s.setUp(): complete", self)

    def create_test_data(self):
        monitored_files = [
            'test-monitored/file1.txt',
            'test-monitored/dir1/file2.txt',
            'test-monitored/dir1/dir2/file3.txt'
        ]

        unmonitored_files = [
            'test-no-monitored/file0.txt'
        ]

        for tf in itertools.chain(monitored_files, unmonitored_files):
            testfile = self.path(tf)
            os.makedirs(os.path.dirname(testfile), exist_ok=True)
            with open(testfile, 'w') as f:
                f.write(DEFAULT_TEXT)

        return monitored_files

    def __get_text_documents(self):
        return self.tracker.query("""
          SELECT DISTINCT ?url WHERE {
              ?u a nfo:TextDocument ;
                 nie:isStoredAs/nie:url ?url.
          }
          """)

    def __get_parent_urn(self, filepath):
        result = self.tracker.query("""
          SELECT DISTINCT ?p WHERE {
              ?u a nfo:FileDataObject ;
                 nie:url \"%s\" ;
                 nfo:belongsToContainer ?p
          }
          """ % (self.uri(filepath)))
        self.assertEqual(len(result), 1)
        return result[0][0]

    def __get_file_urn(self, filepath):
        result = self.tracker.query("""
          SELECT nie:interpretedAs(?u) WHERE {
              ?u a nfo:FileDataObject ;
                 nie:url \"%s\" .
          }
          """ % (self.uri(filepath)))
        self.assertEqual(len(result), 1)
        return result[0][0]

    """
    Boot the miner with the correct configuration and check everything is fine
    """

    def test_01_initial_crawling(self):
        """
        The precreated files and folders should be there
        """
        result = self.__get_text_documents()
        self.assertEqual(len(result), 3)
        unpacked_result = [r[0] for r in result]
        self.assertIn(self.uri("test-monitored/file1.txt"), unpacked_result)
        self.assertIn(self.uri("test-monitored/dir1/file2.txt"), unpacked_result)
        self.assertIn(self.uri("test-monitored/dir1/dir2/file3.txt"), unpacked_result)

        # We don't check (yet) folders, because Applications module is injecting results


# class copy(TestUpdate):
# FIXME all tests in one class because the miner-fs restarting takes some time (~5 sec)
# Maybe we can move the miner-fs initialization to setUpModule and then move these
# tests to different classes


    def test_02_copy_from_unmonitored_to_monitored(self):
        """
        Copy an file from unmonitored directory to monitored directory
        and verify if data base is updated accordingly
        """
        source = os.path.join(self.workdir, "test-no-monitored", "file0.txt")
        dest = os.path.join(self.workdir, "test-monitored", "file0.txt")

        with self.await_document_inserted(dest) as resource:
            shutil.copyfile(source, dest)
        dest_id = resource.id

        # Verify if miner indexed this file.
        result = self.__get_text_documents()
        self.assertEqual(len(result), 4)
        unpacked_result = [r[0] for r in result]
        self.assertIn(self.uri("test-monitored/file1.txt"), unpacked_result)
        self.assertIn(self.uri("test-monitored/dir1/file2.txt"), unpacked_result)
        self.assertIn(self.uri("test-monitored/dir1/dir2/file3.txt"), unpacked_result)
        self.assertIn(self.uri("test-monitored/file0.txt"), unpacked_result)

        # Clean the new file so the test directory is as before
        with self.tracker.await_delete(fixtures.DOCUMENTS_GRAPH, dest_id):
            os.remove(dest)

    def test_03_copy_from_monitored_to_unmonitored(self):
        """
        Copy an file from a monitored location to an unmonitored location
        Nothing should change
        """

        # Copy from monitored to unmonitored
        source = os.path.join(self.workdir, "test-monitored", "file1.txt")
        dest = os.path.join(self.workdir, "test-no-monitored", "file1.txt")
        shutil.copyfile(source, dest)

        time.sleep(1)
        # Nothing changed
        result = self.__get_text_documents()
        self.assertEqual(len(result), 3, "Results:" + str(result))
        unpacked_result = [r[0] for r in result]
        self.assertIn(self.uri("test-monitored/file1.txt"), unpacked_result)
        self.assertIn(self.uri("test-monitored/dir1/file2.txt"), unpacked_result)
        self.assertIn(self.uri("test-monitored/dir1/dir2/file3.txt"), unpacked_result)

        # Clean the file
        os.remove(dest)

    def test_04_copy_from_monitored_to_monitored(self):
        """
        Copy a file between monitored directories
        """
        source = os.path.join(self.workdir, "test-monitored", "file1.txt")
        dest = os.path.join(self.workdir, "test-monitored", "dir1", "dir2", "file-test04.txt")

        with self.await_document_inserted(dest) as resource:
            shutil.copyfile(source, dest)
        dest_id = resource.id

        result = self.__get_text_documents()
        self.assertEqual(len(result), 4)
        unpacked_result = [r[0] for r in result]
        self.assertIn(self.uri("test-monitored/file1.txt"), unpacked_result)
        self.assertIn(self.uri("test-monitored/dir1/file2.txt"), unpacked_result)
        self.assertIn(self.uri("test-monitored/dir1/dir2/file3.txt"), unpacked_result)
        self.assertIn(self.uri("test-monitored/dir1/dir2/file-test04.txt"), unpacked_result)

        with self.tracker.await_delete(fixtures.DOCUMENTS_GRAPH, dest_id):
            os.remove(dest)

        self.assertEqual(3, self.tracker.count_instances("nfo:TextDocument"))

    @ut.skip("https://gitlab.gnome.org/GNOME/tracker-miners/issues/56")
    def test_05_move_from_unmonitored_to_monitored(self):
        """
        Move a file from unmonitored to monitored directory
        """
        source = os.path.join(self.workdir, "test-no-monitored", "file0.txt")
        dest = os.path.join(self.workdir, "test-monitored", "dir1", "file-test05.txt")

        with self.await_document_inserted(dest) as resource:
            shutil.move(source, dest)
        dest_id = resource.id

        result = self.__get_text_documents()
        self.assertEqual(len(result), 4)
        unpacked_result = [r[0] for r in result]
        self.assertIn(self.uri("test-monitored/file1.txt"), unpacked_result)
        self.assertIn(self.uri("test-monitored/dir1/file2.txt"), unpacked_result)
        self.assertIn(self.uri("test-monitored/dir1/dir2/file3.txt"), unpacked_result)
        self.assertIn(self.uri("test-monitored/dir1/file-test05.txt"), unpacked_result)

        with self.tracker.await_delete(fixtures.DOCUMENTS_GRAPH, dest_id):
            os.remove(dest)

        self.assertEqual(3, self.tracker.count_instances("nfo:TextDocument"))

## """ move operation and tracker-miner response test cases """
# class move(TestUpdate):

    def test_06_move_from_monitored_to_unmonitored(self):
        """
        Move a file from monitored to unmonitored directory
        """
        source = self.path("test-monitored/dir1/file2.txt")
        dest = self.path("test-no-monitored/file2.txt")
        source_id = self.tracker.get_content_resource_id(self.uri(source))
        with self.tracker.await_delete(fixtures.DOCUMENTS_GRAPH, source_id):
            shutil.move(source, dest)

        result = self.__get_text_documents()
        self.assertEqual(len(result), 2)
        unpacked_result = [r[0] for r in result]
        self.assertIn(self.uri("test-monitored/file1.txt"), unpacked_result)
        self.assertIn(self.uri("test-monitored/dir1/dir2/file3.txt"), unpacked_result)

        with self.await_document_inserted(source):
            shutil.move(dest, source)
        self.assertEqual(3, self.tracker.count_instances("nfo:TextDocument"))

    def test_07_move_from_monitored_to_monitored(self):
        """
        Move a file between monitored directories
        """

        source = self.path("test-monitored/dir1/file2.txt")
        dest = self.path("test-monitored/file2.txt")

        source_dir_urn = self.__get_file_urn(os.path.dirname(source))
        parent_before = self.__get_parent_urn(source)
        self.assertEqual(source_dir_urn, parent_before)

        resource_id = self.tracker.get_content_resource_id(url=self.uri(source))
        with self.await_document_uri_change(resource_id, source, dest):
            shutil.move(source, dest)

        # Checking fix for NB#214413: After a move operation, nfo:belongsToContainer
        # should be changed to the new one
        dest_dir_urn = self.__get_file_urn(os.path.dirname(dest))
        parent_after = self.__get_parent_urn(dest)
        self.assertNotEqual(parent_before, parent_after)
        self.assertEqual(dest_dir_urn, parent_after)

        result = self.__get_text_documents()
        self.assertEqual(len(result), 3)
        unpacked_result = [r[0] for r in result]
        self.assertIn(self.uri("test-monitored/file1.txt"), unpacked_result)
        self.assertIn(self.uri("test-monitored/file2.txt"), unpacked_result)
        self.assertIn(self.uri("test-monitored/dir1/dir2/file3.txt"), unpacked_result)

        # Restore the file
        with self.await_document_uri_change(resource_id, dest, source):
            shutil.move(dest, source)

        result = self.__get_text_documents()
        self.assertEqual(len(result), 3)
        unpacked_result = [r[0] for r in result]
        self.assertIn(self.uri("test-monitored/dir1/file2.txt"), unpacked_result)

    def test_08_deletion_single_file(self):
        """
        Delete one of the files
        """
        victim = self.path("test-monitored/dir1/file2.txt")
        victim_id = self.tracker.get_content_resource_id(self.uri(victim))
        with self.tracker.await_delete(fixtures.DOCUMENTS_GRAPH, victim_id):
            os.remove(victim)

        result = self.__get_text_documents()
        self.assertEqual(len(result), 2)
        unpacked_result = [r[0] for r in result]
        self.assertIn(self.uri("test-monitored/file1.txt"), unpacked_result)
        self.assertIn(self.uri("test-monitored/dir1/dir2/file3.txt"), unpacked_result)

        # Restore the file
        with self.await_document_inserted(victim):
            with open(victim, "w") as f:
                f.write("Don't panic, everything is fine")

    def test_09_deletion_directory(self):
        """
        Delete a directory
        """
        victim = self.path("test-monitored/dir1")

        file_inside_victim_url = self.uri(os.path.join(victim, "file2.txt"))
        file_inside_victim_id = self.tracker.get_content_resource_id(file_inside_victim_url)

        with self.tracker.await_delete(fixtures.DOCUMENTS_GRAPH, file_inside_victim_id):
            shutil.rmtree(victim)

        result = self.__get_text_documents()
        self.assertEqual(len(result), 1)
        unpacked_result = [r[0] for r in result]
        self.assertIn(self.uri("test-monitored/file1.txt"), unpacked_result)

        # Restore the dirs
        os.makedirs(self.path("test-monitored/dir1"))
        os.makedirs(self.path("test-monitored/dir1/dir2"))
        for f in ["test-monitored/dir1/file2.txt",
                  "test-monitored/dir1/dir2/file3.txt"]:
            filename = self.path(f)
            with self.await_document_inserted(filename):
                with open(filename, "w") as f:
                    f.write("Don't panic, everything is fine")

        # Check everything is fine
        result = self.__get_text_documents()
        self.assertEqual(len(result), 3)

    def test_10_folder_update(self):
        """
        Check that updating a folder updates nfo:belongsToContainer on its children
        """

        directory_uri = self.uri("test-monitored")
        directory = self.path("test-monitored")
        document = self.path("test-monitored/unrelated.txt")
        resource_id = self.tracker.get_content_resource_id(directory_uri)
        urn = self.__get_file_urn(directory)

        with self.await_document_inserted(document) as resource:
            # Force an update on the monitored folder, it needs both
            # a explicit request, and an attribute change (obtained
            # indirectly by the new file)
            with open(document, 'w') as f:
                f.write(DEFAULT_TEXT)
            self.miner_fs.index_file(directory_uri)

        new_urn = self.__get_file_urn(directory)
        # After the update, the InformationElement should be brand new
        assert(urn != new_urn)
        # Ensure that children remain consistent, old and new ones
        self.assertEqual(new_urn,
                         self.__get_parent_urn(self.path("test-monitored/file1.txt")))
        self.assertEqual(self.__get_parent_urn(document),
                         self.__get_parent_urn(self.path("test-monitored/file1.txt")))

if __name__ == "__main__":
    fixtures.tracker_test_main()
