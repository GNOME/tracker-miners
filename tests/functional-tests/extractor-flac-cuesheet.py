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

"""
Tests the FLAC+cuesheet extraction feature.
"""


import pathlib
import shutil
import tempfile
import unittest as ut

import configuration as cfg
import datagenerator
import fixtures


class FlacCuesheetTest(fixtures.TrackerExtractTestCase):
    def spec(self, audio_path):
        audio_uri = audio_path.as_uri()
        return {
            '@type': ['nfo:Audio'],
            'nie:url': audio_uri,
            'nfo:duration': 360,
            'nfo:sampleRate': 44100,
            'nie:hasLogicalPart': [
                {
                    '@type': ['nmm:MusicPiece', 'nfo:Audio'],
                    'nfo:audioOffset': 0.0,
                    'nfo:duration': 257,
                    'nie:isLogicalPartOf': audio_uri,
                    'nie:isStoredAs': audio_uri,
                    'nie:title': 'Only Shallow',
                    'nmm:trackNumber': 1,
                    'nmm:musicAlbum': {
                        '@id': 'urn:album:Loveless:My%20Bloody%20Valentine',
                        '@type': 'nmm:MusicAlbum',
                        'nmm:albumTrackCount': 2,
                        'nmm:albumArtist': ['urn:artist:My%20Bloody%20Valentine'],
                        'nie:title': 'Loveless',
                    },
                    'nmm:musicAlbumDisc': {
                        '@id': 'urn:album-disc:Loveless:My%20Bloody%20Valentine:Disc1',
                        '@type': 'nmm:MusicAlbumDisc',
                        'nmm:setNumber': 1,
                        'nmm:albumDiscAlbum': 'urn:album:Loveless:My%20Bloody%20Valentine',
                    },
                    'nmm:performer': {
                        '@id': 'urn:artist:My%20Bloody%20Valentine',
                        '@type': 'nmm:Artist',
                        'nmm:artistName': 'My Bloody Valentine',
                    },
                },
                {
                    '@type': ['nmm:MusicPiece', 'nfo:Audio'],
                    'nfo:audioOffset': 257.6933333333333,
                    'nfo:duration': 102,
                    'nie:isLogicalPartOf': audio_uri,
                    'nie:isStoredAs': audio_uri,
                    'nmm:musicAlbum': 'urn:album:Loveless:My%20Bloody%20Valentine',
                    'nmm:musicAlbumDisc': 'urn:album-disc:Loveless:My%20Bloody%20Valentine:Disc1',
                    'nmm:performer': 'urn:artist:My%20Bloody%20Valentine',
                    'nie:title': 'Loomer',
                    'nmm:trackNumber': 2,
                }
            ],
        }

    def test_external_cue_sheet(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            datadir = pathlib.Path(__file__).parent.joinpath('test-extraction-data')
            shutil.copy(datadir.joinpath('audio', 'cuesheet-test.cue'), tmpdir)

            audio_path = pathlib.Path(tmpdir).joinpath('cuesheet-test.flac')
            datagenerator.create_test_flac(audio_path, duration=6*60)

            result = fixtures.get_tracker_extract_jsonld_output(
                cfg.test_environment(tmpdir), audio_path)

        self.assert_extract_result_matches_spec(
            self.spec(audio_path), result, audio_path, __file__)


if __name__ == '__main__':
    ut.main(verbosity=2)
