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

import pathlib
import unittest

import fixtures


class MinerOnDemandTest(fixtures.TrackerMinerTest):
    """
    Tests on-demand indexing.

    This covers the IndexFile and IndexFileForProcess D-Bus methods, and the
    FileProcessed signal.
    """

    def setUp(self):
        fixtures.TrackerMinerTest.setUp(self)

        self.testfile = pathlib.Path(self.workdir).joinpath("test-not-monitored/on-demand.txt")
        self.testfile.parent.mkdir(parents=True, exist_ok=True)
        self.testfile.write_text("Hello, I'm a test file.")

    def test_index_file_basic(self):
        """
        Test on-demand indexing of a file.
        """

        self.assertFileNotIndexed(self.testfile.as_uri())

        self.miner_fs.index_file(self.testfile.as_uri())

        self.assertFileIndexed(self.testfile.as_uri())


if __name__ == "__main__":
    unittest.main(failfast=True, verbosity=2)
