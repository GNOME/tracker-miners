# Copyright (C) 2016, Sam Thursfield (sam@afuera.me.uk)
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

"""
Tests failure cases of tracker-extract.
"""

import os
import shutil
import unittest as ut

import configuration as cfg
import fixtures


VALID_FILE = os.path.join(
    os.path.dirname(__file__), 'test-extraction-data', 'audio',
    'mp3-id3v2.4-1.mp3')
VALID_FILE_TITLE = 'Simply Juvenile'

TRACKER_EXTRACT_FAILURE_DATA_SOURCE = 'tracker:extractor-failure-data-source'


class ExtractorDecoratorTest(fixtures.TrackerMinerTest):
    def test_reextraction(self):
        """Tests whether known files are still re-extracted on user request."""
        miner_fs = self.miner_fs
        store = self.tracker

        # Insert a valid file and wait extraction of its metadata.
        file_path = os.path.join(self.indexed_dir, os.path.basename(VALID_FILE))
        expected = f'a nmm:MusicPiece ; nie:title "{VALID_FILE_TITLE}"'
        with self.tracker.await_insert(fixtures.AUDIO_GRAPH, expected,
                                       timeout=cfg.AWAIT_TIMEOUT) as resource:
            shutil.copy(VALID_FILE, file_path)
        file_urn = resource.urn

        try:
            # Remove a key piece of metadata.
            #   (Writeback must be disabled in the config so that the file
            #   itself is not changed).
            store.update(
                'DELETE { GRAPH ?g { <%s> nie:title ?title } }'
                ' WHERE { GRAPH ?g { <%s> nie:title ?title } }' % (file_urn, file_urn))
            assert not store.ask('ASK { <%s> nie:title ?title }' % file_urn)

            file_uri = 'file://' + os.path.join(self.indexed_dir, file_path)

            # Request re-indexing (same as `tracker index --file ...`)
            # The extractor should reindex the file and re-add the metadata that we
            # deleted, so we should see the nie:title property change.
            with self.tracker.await_insert(fixtures.AUDIO_GRAPH, f'nie:title "{VALID_FILE_TITLE}"',
                                           timeout=cfg.AWAIT_TIMEOUT):
                miner_fs.index_file(file_uri)

            title_result = store.query('SELECT ?title { <%s> nie:interpretedAs/nie:title ?title }' % file_uri)
            assert len(title_result) == 1
            self.assertEqual(title_result[0][0], VALID_FILE_TITLE)
        finally:
            os.remove(file_path)


if __name__ == '__main__':
    fixtures.tracker_test_main()
