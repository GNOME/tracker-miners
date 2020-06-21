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

"""
Test `tracker` commandline tool
"""

import pathlib

import configuration
import fixtures

class TestCli(fixtures.TrackerCommandLineTestCase):
    def test_search(self):
        datadir = pathlib.Path(__file__).parent.joinpath('test-cli-data')

        # FIXME: synchronous `tracker index` isn't ready yet; 
        # see https://gitlab.gnome.org/GNOME/tracker/-/issues/188
        # in the meantime we manually wait for it to finish.

        file1 = datadir.joinpath('text/Document 1.txt')
        file2 = datadir.joinpath('text/Document 2.txt')

        with self.await_document_inserted(file1):
            with self.await_document_inserted(file2):
                output = self.run_cli(
                    ['tracker3', 'index', '--file', str(datadir)])

        # FIXME: the --all should NOT be needed.
        # See: https://gitlab.gnome.org/GNOME/tracker-miners/-/issues/116
        output = self.run_cli(
            ['tracker3', 'search', '--all', 'banana'])
        self.assertIn(file1.as_uri(), output)
        self.assertNotIn(file2.as_uri(), output)


if __name__ == '__main__':
    fixtures.tracker_test_main()
