# Copyright (C) 2020, Sam Thursfield <sam@afuera.me.uk>
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

"""
Test `tracker` commandline tool
"""

import pathlib

import configuration
import fixtures


class TestCli(fixtures.TrackerCommandLineTestCase):
    # The `tracker index` tests overlap somewhat with the miner-on-demand
    # test. These tests only need to cover the CLI-specific features.

    def create_test_file(self, path):
        testfile = pathlib.Path(self.workdir).joinpath(path)
        testfile.parent.mkdir(parents=True, exist_ok=True)
        testfile.write_text("Hello, I'm a test file.")
        return testfile

    def test_index_file(self):
        """Test that `tracker index` triggers indexing of a file."""

        testfile = self.create_test_file('test-data/content.txt')

        self.run_cli(['index', str(testfile)])

        self.assertFileIndexed(testfile.as_uri())

    def test_index_corrupt_file(self):
        """Test that indexing a corrupt file raises an error."""

        # This will generate an invalid .mp3 file.
        testfile = self.create_test_file('test-data/content.mp3')

        with self.assertRaises(fixtures.CliError) as e:
            self.run_cli(['index', str(testfile)])
        #print(e.exception)
        #assert 'foo' in e.exception.args[0]


if __name__ == '__main__':
    fixtures.tracker_test_main()
