#
# Copyright (C) 2010, Nokia <ivan.frade@nokia.com>
# Copyright (C) 2018-2019, Sam Thursfield <sam@afuera.me.uk>
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

import json
import os
import shutil
import sys
import tempfile
import unittest as ut

import configuration as cfg
import fixtures

class GenericExtractionTestCase(fixtures.TrackerExtractTestCase):
    """
    Test checks if the tracker extractor is able to retrieve metadata
    """

    def __init__(self, methodName='runTest', descfile=None):
        """
        Descfile is the description file in a relative path
        """
        super(GenericExtractionTestCase, self).__init__(methodName)

        self.descfile = descfile
        try:
            with open(descfile) as f:
                self.spec = json.load(f)
        except ValueError as e:
            self.fail("Error loading %s: %s" % (descfile, e))

        # Add a method to the class called after the description file
        methodName = descfile.lower()[:-len(".expected")].replace(" ", "_")[-60:]

        if (self.spec['test'].get('ExpectedFailure', False)):
            setattr(self,
                    methodName,
                    self.expected_failure_test_extraction)
        else:
            setattr(self, methodName, self.generic_test_extraction)

        # unittest framework will run the test called "self._testMethodName"
        # So we set that variable to our new name
        self._testMethodName = methodName

    def __get_bugnumber(self):
        return self.spec['test'].get('Bugzilla')

    def generic_test_extraction(self):
        abs_description = os.path.abspath(self.descfile)

        # Filename contains the file to extract, in a relative path to the description file
        desc_root, desc_file = os.path.split(abs_description)

        filename_to_extract = self.spec['test']['Filename']
        self.file_to_extract = os.path.join(desc_root, filename_to_extract)

        tmpdir = tempfile.mkdtemp(prefix='tracker-extract-test-')
        try:
            extra_env = cfg.test_environment(tmpdir)
            result = fixtures.get_tracker_extract_jsonld_output(extra_env, self.file_to_extract)
            self.__assert_extraction_ok(result)
        finally:
            shutil.rmtree(tmpdir, ignore_errors=True)

    @ut.expectedFailure
    def expected_failure_test_extraction(self):
        self.generic_test_extraction()

        if self.__get_bugnumber():
            raise Exception("Unexpected success. Maybe bug: " + self.__get_bugnumber() + " has been fixed?")
        else:
            raise Exception("Unexpected success. Check " + self.rel_description)

    def __assert_extraction_ok(self, result):
        try:
            self.assert_extract_result_matches_spec(self.spec['metadata'], result, self.file_to_extract, self.descfile)
        except AssertionError:
            print("\ntracker-extract returned: %s" % json.dumps(result, indent=4))
            raise


def run_all():
    ##
    # Traverse the TEST_DATA_PATH directory looking for .description files
    # Add a new TestCase to the suite per .description file and run the suite.
    #
    # Is we do this inside a single TestCase an error in one test would stop the whole
    # testing.
    ##
    if (os.path.exists(os.getcwd() + "/test-extraction-data")):
        # Use local directory if available
        TEST_DATA_PATH = os.getcwd() + "/test-extraction-data"
    else:
        TEST_DATA_PATH = os.path.join(cfg.DATADIR, "tracker-tests",
                                      "test-extraction-data")
    print("Loading test descriptions from", TEST_DATA_PATH)
    extractionTestSuite = ut.TestSuite()
    for root, dirs, files in os.walk(TEST_DATA_PATH):
        descriptions = [os.path.join(root, f) for f in files if f.endswith("expected")]
        for descfile in descriptions:
            tc = GenericExtractionTestCase(descfile=descfile)
            extractionTestSuite.addTest(tc)
    result = ut.TextTestRunner(verbosity=1).run(extractionTestSuite)
    sys.exit(not result.wasSuccessful())


def run_one(filename):
    ##
    # Run just one .description file
    ##
    description = os.path.join(os.getcwd(), filename)

    extractionTestSuite = ut.TestSuite()
    tc = GenericExtractionTestCase(descfile=description)
    extractionTestSuite.addTest(tc)

    result = ut.TextTestRunner(verbosity=2).run(extractionTestSuite)
    sys.exit(not result.wasSuccessful())


try:
    if len(sys.argv) == 2:
        run_one(sys.argv[1])
    elif len(sys.argv) == 1:
        run_all()
    else:
        raise RuntimeError("Too many arguments.")
except RuntimeError as e:
    sys.stderr.write("ERROR: %s\n" % e)
    sys.exit(1)
