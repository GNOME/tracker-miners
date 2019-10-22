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


import unittest

from extractor import get_tracker_extract_jsonld_output
from writebacktest import CommonTrackerWritebackTest


class WritebackAudioTest(CommonTrackerWritebackTest):
    def _writeback_test_title(self, path):
        """Test updating the 'title' tag of a music file."""

        path = self.prepare_test_audio(path)
        initial_mtime = path.stat().st_mtime

        TEST_VALUE = "test_title"
        SPARQL_TMPL = """
           DELETE { ?u nie:title ?v } WHERE { ?u nie:url '%s' ; nie:title ?v }
           INSERT { ?u nie:title '%s' }
           WHERE  { ?u nie:url '%s' }
        """
        self.tracker.update(SPARQL_TMPL % (path.as_uri(), TEST_VALUE, path.as_uri()))

        self.wait_for_file_change(path, initial_mtime)

        results = get_tracker_extract_jsonld_output(self.extra_env, path)
        self.assertIn(TEST_VALUE, results['nie:title'])

    def _writeback_test_album_title(self, path):
        """Test updating the 'album title' tag of a music file."""

        path = self.prepare_test_audio(path)
        url = path.as_uri()
        initial_mtime = path.stat().st_mtime

        TEST_VALUE = "test_album_title"

        # First we delete link between the musicpiece and the old album.
        update = 'DELETE { ?musicpiece nmm:musicAlbum ?album } WHERE { ?musicpiece nie:url "%s" }' % url
        self.tracker.update(update)

        # Then we check if a matching album resource already exists. (In the
        # case of this test, this is unnecessary, but real applications will
        # need to do so).
        query = 'SELECT ?album { ?album a nmm:MusicAlbum ; nie:title "New Title" }'
        results = self.tracker.query(query)
        assert len(results) == 0

        # Create a new music album and music album disk.
        new_album_title = TEST_VALUE
        new_album_urn = "urn:album:test_album_title"
        new_album_disc_urn = "urn:album-disc:test_album_title:Disc1"

        update = """
            INSERT DATA {
                <%s> a nmm:MusicAlbum ;
                    nie:title "%s" .
                <%s> a nmm:MusicAlbumDisc ;
                    nmm:setNumber 1 ;
                    nmm:albumDiscAlbum <%s> .
            }
        """ % (new_album_urn, new_album_title, new_album_disc_urn, new_album_urn)
        self.tracker.update(update)

        # Finally, update the existing musicpiece to refer to the new album.
        update = 'INSERT { ?musicpiece nmm:musicAlbum <%s> } WHERE { ?musicpiece nie:url "%s" . }' % (new_album_urn, url)
        self.tracker.update(update)

        self.wait_for_file_change(path, initial_mtime)

        results = get_tracker_extract_jsonld_output(path)
        self.assertIn(TEST_VALUE, results['nmm:musicAlbum']['nie:title'])

    def test_writeback_mp3_title(self):
        self._writeback_test_title(self.datadir_path('writeback-test-5.mp3'))
    def test_writeback_mp3_album_title(self):
        self._writeback_test_album_title(self.datadir_path('writeback-test-5.mp3'))

    def test_writeback_ogg_title(self):
        self._writeback_test_title(self.datadir_path('writeback-test-6.ogg'))
    def test_writeback_ogg_album_title(self):
        self._writeback_test_album_title(self.datadir_path('writeback-test-6.ogg'))

    def test_writeback_flac_title(self):
        self._writeback_test_title(self.datadir_path('writeback-test-7.flac'))
    def test_writeback_flac_album_title(self):
        self._writeback_test_album_title(self.datadir_path('writeback-test-7.flac'))

    def test_writeback_aac_title(self):
        self._writeback_test_title(self.datadir_path('writeback-test-8.mp4'))
    def test_writeback_aac_album_title(self):
        self._writeback_test_album_title(self.datadir_path('writeback-test-8.mp4'))

if __name__ == "__main__":
    unittest.main(failfast=True, verbosity=2)
