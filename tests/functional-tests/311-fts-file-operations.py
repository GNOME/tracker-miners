# -*- coding: utf-8 -*-

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
# TODO:
#     These tests are for files... we need to write them for folders!
#
"""
Monitor a directory, copy/move/remove/update text files and check that
the text contents are updated accordingly in the indexes.
"""
import os
import shutil
import locale
import time

import unittest as ut
from minertest import CommonTrackerMinerFTSTest, DEFAULT_TEXT
import configuration as cfg


NFO_DOCUMENT = 'http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#Document'


class MinerFTSFileOperationsTest (CommonTrackerMinerFTSTest):
    """
    Move, update, delete the files and check the text indexes are updated accordingly.
    """

    def test_01_removal_of_file(self):
        """
        When removing the file, its text contents disappear from the index
        """
        TEXT = "automobile is red and big and whatnot"
        self.basic_test(TEXT, "automobile")

        id = self._query_id(self.uri(self.testfile))
        os.remove(self.path(self.testfile))
        self.tracker.await_resource_deleted(NFO_DOCUMENT, id)

        results = self.search_word("automobile")
        self.assertEqual(len(results), 0)

    def test_02_empty_the_file(self):
        """
        Emptying the file, the indexed words are also removed
        """
        TEXT = "automobile is red and big and whatnot"
        self.basic_test(TEXT, "automobile")

        self.set_text("")
        results = self.search_word("automobile")
        self.assertEqual(len(results), 0)

    @ut.skip("FIXME: this test fails!")
    def test_03_update_the_file(self):
        """
        Changing the contents of the file, updates the index
        """
        TEXT = "automobile is red and big and whatnot"
        self.basic_test(TEXT, "automobile")

        self.set_text("airplane is blue and small and wonderful")

        results = self.search_word("automobile")
        self.assertEqual(len(results), 0)

        results = self.search_word("airplane")
        self.assertEqual(len(results), 1)

    # Skip the test_text_13... feel, feet, fee in three diff files and search feet

    def __recreate_file(self, filename, content):
        if os.path.exists(filename):
            os.remove(filename)

        f = open(filename, "w")
        f.write(content)
        f.close()

    def test_04_on_unmonitored_file(self):
        """
        Set text in an unmonitored file. There should be no results.
        """
        TEXT = "automobile is red"

        TEST_15_FILE = "test-no-monitored/fts-indexing-test-15.txt"
        self.__recreate_file(self.path(TEST_15_FILE), TEXT)

        results = self.search_word("automobile")
        self.assertEqual(len(results), 0)

        os.remove(self.path(TEST_15_FILE))

    def test_05_move_file_unmonitored_monitored(self):
        """
        Move file from unmonitored location to monitored location and index should be updated
        """

        TEXT = "airplane is beautiful"
        TEST_16_SOURCE = "test-no-monitored/fts-indexing-text-16.txt"
        TEST_16_DEST = "test-monitored/fts-indexing-text-16.txt"

        self.__recreate_file(self.path(TEST_16_SOURCE), TEXT)
        # the file is supposed to be ignored by tracker, so there is no notification..
        time.sleep(2)

        results = self.search_word("airplane")
        self.assertEqual(len(results), 0)

        shutil.copyfile(self.path(TEST_16_SOURCE), self.path(TEST_16_DEST))
        self.tracker.await_resource_inserted(rdf_class=NFO_DOCUMENT,
                                             url=self.uri(TEST_16_DEST),
                                             required_property='nie:plainTextContent')

        results = self.search_word("airplane")
        self.assertEqual(len(results), 1)

        os.remove(self.path(TEST_16_SOURCE))
        os.remove(self.path(TEST_16_DEST))

    # skip test for a file in a hidden directory


if __name__ == "__main__":
    ut.main(failfast=True)
