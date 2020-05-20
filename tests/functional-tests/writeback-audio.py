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


import unittest

import fixtures

import gi
from gi.repository import GLib

class WritebackAudioTest(fixtures.TrackerWritebackTest):
    def _writeback_test(self, path):
        prop = 'nie:title'

        path = self.prepare_test_audio(self.datadir_path(path))
        initial_mtime = path.stat().st_mtime

        TEST_VALUE = prop.replace(":", "") + "test"

        self.writeback_data({
            'http://www.w3.org/1999/02/22-rdf-syntax-ns#type': GLib.Variant('s', 'http://tracker.api.gnome.org/ontology/v3/nfo#Audio'),
            'http://tracker.api.gnome.org/ontology/v3/nie#isStoredAs': GLib.Variant('s', path.as_uri()),
            'http://tracker.api.gnome.org/ontology/v3/nie#title': GLib.Variant('s', TEST_VALUE),
        })

        self.wait_for_file_change(path, initial_mtime)

        results = fixtures.get_tracker_extract_jsonld_output({}, path)
        self.assertIn(TEST_VALUE, results[prop])

    def test_writeback_mp3(self):
        self._writeback_test(self.datadir_path('writeback-test-5.mp3'))

    def test_writeback_ogg(self):
        self._writeback_test(self.datadir_path('writeback-test-6.ogg'))

    def test_writeback_flac(self):
        self._writeback_test(self.datadir_path('writeback-test-7.flac'))

    def test_writeback_aac(self):
        self._writeback_test(self.datadir_path('writeback-test-8.mp4'))

if __name__ == "__main__":
    unittest.main(failfast=True, verbosity=2)
