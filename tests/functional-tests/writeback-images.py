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

"""Tests for Tracker writeback daemon."""


import logging
import unittest as ut

import configuration
import fixtures

import gi
from gi.repository import GLib

log = logging.getLogger(__name__)


class WritebackImagesTest(fixtures.TrackerWritebackTest):
    """
    Write in tracker store the properties witih writeback support and check
    that the new values are actually in the file
    """

    def __writeback_test(self, filename, mimetype, prop, expectedKey=None):
        """
        Set a value in @prop for the @filename. Then ask tracker-extractor
        for metadata and check in the results dictionary if the property is there.

        Note: given the special translation of some property-names in the dictionary
        with extracted metadata, there is an optional parameter @expectedKey
        to specify what property to check in the dictionary. If None, then
        the @prop is used.
        """

        path = self.prepare_test_image(self.datadir_path(filename))
        initial_mtime = path.stat().st_mtime

        TEST_VALUE = prop.replace(":", "") + "test"
        self.writeback_data({
            'http://www.w3.org/1999/02/22-rdf-syntax-ns#type': GLib.Variant('s', 'http://tracker.api.gnome.org/ontology/v3/nfo#Image'),
            'http://tracker.api.gnome.org/ontology/v3/nie#isStoredAs': GLib.Variant('s', path.as_uri()),
            prop: GLib.Variant('s', TEST_VALUE),
        })

        log.debug("Waiting for change on %s", path)
        self.wait_for_file_change(path, initial_mtime)
        log.debug("Got the change")

        results = fixtures.get_tracker_extract_jsonld_output({}, path, mimetype)
        keyDict = expectedKey or prop
        self.assertIn(TEST_VALUE, results[keyDict])

    def __writeback_hasTag_test(self, filename, mimetype):

        SPARQL_TMPL = """
            INSERT {
              <test://writeback-hasTag-test/1> a nao:Tag ;
                        nao:prefLabel "testTag" .

              ?u a rdfs:Resource; nao:hasTag <test://writeback-hasTag-test/1> .
            } WHERE {
              ?u nie:url '%s' .
            }
        """

        path = self.prepare_test_image(self.datadir_path(filename))
        initial_mtime = path.stat().st_mtime

        self.tracker.update(SPARQL_TMPL % (filename))

        self.wait_for_file_change(path, initial_mtime)

        results = fixtures.get_tracker_extract_jsonld_output(self.extra_env, filename, mimetype)
        self.assertIn("testTag", results["nao:hasTag"])

    # JPEG test

    def test_001_jpeg_title(self):
        self.__writeback_test(
            "writeback-test-1.jpeg", "image/jpeg",
            "http://tracker.api.gnome.org/ontology/v3/nie#title",
            "nie:title")

    def test_002_jpeg_description(self):
        self.__writeback_test(
            "writeback-test-1.jpeg", "image/jpeg",
            "http://tracker.api.gnome.org/ontology/v3/nie#description",
            "nie:description")

    # def test_003_jpeg_keyword (self):
    #    #FILENAME = "test-writeback-monitored/writeback-test-1.jpeg"
    #    self.__writeback_test (self.get_test_filename_jpeg (), "image/jpeg",
    #                           "nie:keyword", "nao:hasTag")

    # def test_004_jpeg_hasTag (self):
    #    #FILENAME = "test-writeback-monitored/writeback-test-1.jpeg"
    #    self.__writeback_hasTag_test (self.get_test_filename_jpeg (), "image/jpeg")

    # TIFF tests

    def test_011_tiff_title(self):
        self.__writeback_test(
            "writeback-test-2.tif", "image/tiff",
            "http://tracker.api.gnome.org/ontology/v3/nie#title",
            "nie:title")

    def test_012_tiff_description(self):
        self.__writeback_test(
            "writeback-test-2.tif", "image/tiff",
            "http://tracker.api.gnome.org/ontology/v3/nie#description",
            "nie:description")

    # def test_013_tiff_keyword (self):
    #    FILENAME = "test-writeback-monitored/writeback-test-2.tif"
    #    self.__writeback_test (self.get_test_filename_tiff (), "image/tiff",
    #                           "nie:keyword", "nao:hasTag")

    # def test_014_tiff_hasTag (self):
    #    FILENAME = "test-writeback-monitored/writeback-test-2.tif"
    #    self.__writeback_hasTag_test (self.get_test_filename_tiff (), "image/tiff")

    # PNG tests

    def test_021_png_title(self):
        self.__writeback_test(
            "writeback-test-4.png", "image/png",
            "http://tracker.api.gnome.org/ontology/v3/nie#title",
            "nie:title")

    def test_022_png_description(self):
        self.__writeback_test(
            "writeback-test-4.png", "image/png",
            "http://tracker.api.gnome.org/ontology/v3/nie#description",
            "nie:description")

    # def test_023_png_keyword (self):
    #    FILENAME = "test-writeback-monitored/writeback-test-4.png"
    #    self.__writeback_test (self.get_test_filename_png (), "image/png", "nie:keyword", "nao:hasTag:prefLabel")

    # def test_024_png_hasTag (self):
    #    FILENAME = "test-writeback-monitored/writeback-test-4.png"
    #    self.__writeback_hasTag_test (self.get_test_filename_png (), "image/png")


if __name__ == "__main__":
    ut.main(failfast=True, verbosity=2)
