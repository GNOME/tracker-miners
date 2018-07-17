#!/usr/bin/python
#
# Copyright (C) 2010, Nokia <ivan.frade@nokia.com>
# Copyright (C) 2018, Sam Thursfield <sam@afuera.me.uk>
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
For a collection of files, call the extractor and check that the expected
metadata is extracted. Load dynamically the test information from a data
directory (containing xxx.expected files)
"""

from common.utils import configuration as cfg
from common.utils.extractor import get_tracker_extract_jsonld_output
import unittest2 as ut
import json
import os
import sys



class ExtractionTestCase (ut.TestCase):
    """
    Test checks if the tracker extractor is able to retrieve metadata
    """
    def __init__ (self, methodName='runTest', descfile=None):
        """
        Descfile is the description file in a relative path
        """
        ut.TestCase.__init__ (self, methodName)

        self.descfile = descfile
        try:
            with open(descfile) as f:
                self.spec = json.load(f)
        except ValueError as e:
            self.fail("Error loading %s: %s" % (descfile, e))

        # Add a method to the class called after the description file
        methodName = descfile.lower()[:-len(".expected")].replace (" ", "_")[-60:]

        if (self.spec['test'].get('ExpectedFailure', False)):
            setattr (self,
                    methodName,
                    self.expected_failure_test_extraction)
        else:
            setattr (self, methodName, self.generic_test_extraction)

        # unittest framework will run the test called "self._testMethodName"
        # So we set that variable to our new name
        self._testMethodName = methodName

    def runTest (self):
        """
        Empty function pointer, that should NEVER be called. It is required to exist by unittest.
        """
        assert False

    def __get_bugnumber (self):
        return self.spec['test'].get('Bugzilla')

    def generic_test_extraction (self):
        abs_description = os.path.abspath (self.descfile)

        # Filename contains the file to extract, in a relative path to the description file
        desc_root, desc_file = os.path.split (abs_description)

        filename_to_extract = self.spec['test']['Filename']
        self.file_to_extract = os.path.join (desc_root, filename_to_extract)

        result = get_tracker_extract_jsonld_output(self.file_to_extract)
        self.__assert_extraction_ok (result)

    def expected_failure_test_extraction (self):
        try:
            self.generic_test_extraction ()
        except Exception:
            raise ut.case._ExpectedFailure(sys.exc_info())

        if self.__get_bugnumber ():
            raise Exception ("Unexpected success. Maybe bug: " + self.__get_bugnumber () + " has been fixed?")
        else:
            raise Exception ("Unexpected success. Check " + self.rel_description)

    def assertDictHasKey (self, d, key, msg=None):
        if not isinstance(d, dict):
            self.fail ("Expected dict, got %s" % d)
        if not d.has_key (key):
            standardMsg = "Missing: %s\n" % (key)
            self.fail (self._formatMessage (msg, standardMsg))
        else:
            return

    def assertIsURN (self, supposed_uuid, msg=None):
        import uuid

        try:
            if (supposed_uuid.startswith ("<") and supposed_uuid.endswith (">")):
                supposed_uuid = supposed_uuid[1:-1]

            uuid.UUID (supposed_uuid)
        except ValueError:
            standardMsg = "'%s' is not a valid UUID" % (supposed_uuid)
            self.fail (self._formatMessage (msg, standardMsg))

    def __assert_extraction_ok (self, result):
        try:
            self.__check (self.spec['metadata'], result)
        except AssertionError as e:
            print("\ntracker-extract returned: %s" % json.dumps(result, indent=4))
            raise

    def __check (self, spec, result):
        error_missing_prop = "Property '%s' hasn't been extracted from file \n'%s'\n (requested on '%s')"
        error_wrong_value = "on property '%s' from file %s\n (requested on: '%s')"
        error_wrong_length = "Length mismatch on property '%s' from file %s\n (requested on: '%s')"
        error_extra_prop = "Property '%s' was explicitely banned for file \n'%s'\n (requested on '%s')"
        error_extra_prop_v = "Property '%s' with value '%s' was explicitely banned for file \n'%s'\n (requested on %s')"

        expected_pairs = [] # List of expected (key, value)
        unexpected_pairs = []  # List of unexpected (key, value)
        expected_keys = []  # List of expected keys (the key must be there, value doesnt matter)

        for k, v in spec.items():
            if k.startswith ("!"):
                unexpected_pairs.append ( (k[1:], v) )
            elif k == '@type':
                expected_keys.append ( '@type' )
            else:
                expected_pairs.append ( (k, v) )


        for prop, expected_value in expected_pairs:
            self.assertDictHasKey (result, prop,
                                   error_missing_prop % (prop,
                                                         self.file_to_extract,
                                                         self.descfile))
            if expected_value == "@URNUUID@":
                self.assertIsURN (result [prop][0]['@id'],
                                  error_wrong_value % (prop,
                                                       self.file_to_extract,
                                                       self.descfile))
            else:
                if isinstance(expected_value, list):
                    if not isinstance(result[prop], list):
                        raise AssertionError("Expected a list property for %s, but got a %s: %s" % (
                            prop, type(result[prop]).__name__, result[prop]))

                    self.assertEqual (len(expected_value), len(result[prop]),
                                      error_wrong_length % (prop,
                                                            self.file_to_extract,
                                                            self.descfile))

                    for i in range(0, len(expected_value)):
                        self.__check(spec[prop][i], result[prop][i])
                elif isinstance(expected_value, dict):
                    self.__check(expected_value, result[prop])
                else:
                    self.assertEqual (str(spec[prop]), str(result [prop]),
                                      error_wrong_value % (prop,
                                                           self.file_to_extract,
                                                           self.descfile))

        for (prop, value) in unexpected_pairs:
            # There is no prop, or it is but not with that value
            if (value == ""):
                self.assertFalse (result.has_key (prop), error_extra_prop % (prop,
                                                                             self.file_to_extract,
                                                                             self.descfile))
            else:
                if (value == "@URNUUID@"):
                    self.assertIsURN (result [prop][0], error_extra_prop % (prop,
                                                                            self.file_to_extract,
                                                                            self.descfile))
                else:
                    self.assertNotIn (value, result [prop], error_extra_prop_v % (prop,
                                                                                  value,
                                                                                  self.file_to_extract,
                                                                                  self.descfile))

        for prop in expected_keys:
             self.assertDictHasKey (result, prop,
                                    error_missing_prop % (prop,
                                                          self.file_to_extract,
                                                          self.descfile))


def run_all ():
    ##
    # Traverse the TEST_DATA_PATH directory looking for .description files
    # Add a new TestCase to the suite per .description file and run the suite.
    #
    # Is we do this inside a single TestCase an error in one test would stop the whole
    # testing.
    ##
    if (os.path.exists (os.getcwd() + "/test-extraction-data")):
        # Use local directory if available
        TEST_DATA_PATH = os.getcwd() + "/test-extraction-data"
    else:
        TEST_DATA_PATH = os.path.join (cfg.DATADIR, "tracker-tests",
                                       "test-extraction-data")
    print "Loading test descriptions from", TEST_DATA_PATH
    extractionTestSuite = ut.TestSuite ()
    for root, dirs, files in os.walk (TEST_DATA_PATH):
         descriptions = [os.path.join (root, f) for f in files if f.endswith ("expected")]
         for descfile in descriptions:
             tc = ExtractionTestCase(descfile=descfile)
             extractionTestSuite.addTest(tc)
    result = ut.TextTestRunner (verbosity=1).run (extractionTestSuite)
    sys.exit(not result.wasSuccessful())

def run_one (filename):
    ##
    # Run just one .description file
    ##
    description = os.path.join (os.getcwd (), filename) 

    extractionTestSuite = ut.TestSuite ()
    tc = ExtractionTestCase(descfile=description)
    extractionTestSuite.addTest(tc)

    result = ut.TextTestRunner (verbosity=2).run (extractionTestSuite)
    sys.exit(not result.wasSuccessful())


try:
    if len(sys.argv) == 2:
        run_one (sys.argv[1])
    elif len(sys.argv) == 1:
        run_all ()
    else:
        raise RuntimeError("Too many arguments.")
except RuntimeError as e:
    sys.stderr.write("ERROR: %s\n" % e)
    sys.exit(1)
