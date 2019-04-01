#!/usr/bin/env python3
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

from common.utils import configuration as cfg
from common.utils.helpers import log
import errno
import json
import math
import os
import re
import subprocess
import unittest as ut

import gi
gi.require_version('Gst', '1.0')
from gi.repository import GLib, Gst


def get_tracker_extract_jsonld_output(filename, mime_type=None):
    """
    Runs `tracker-extract --file` to extract metadata from a file.
    """

    tracker_extract = os.path.join(cfg.TRACKER_EXTRACT_PATH)
    command = [tracker_extract, '--verbosity=0', '--output-format=json-ld', '--file', filename]
    if mime_type is not None:
        command.extend(['--mime', mime_type])

    # We depend on parsing the output, so verbosity MUST be 0.
    env = os.environ.copy()
    env['TRACKER_VERBOSITY'] = '0'
    # Tell GStreamer not to fork to create the registry
    env['GST_REGISTRY_FORK'] = 'no'

    log('Running: %s' % ' '.join(command))
    try:
        p = subprocess.Popen(command, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    except OSError as e:
        if e.errno == errno.ENOENT:
            raise RuntimeError("Did not find tracker-extract binary. Is the 'extract' option disabled?")
        else:
            raise RuntimeError("Error running tracker-extract: %s" % (e))
    stdout, stderr = p.communicate()

    if p.returncode != 0:
        raise RuntimeError(
            "tracker-extract returned non-zero exit code: %s\n"
            "Error output:\n%s\n" % (p.returncode, stderr.decode('unicode-escape').strip()))

    if len(stderr) > 0:
        log("Error output from tracker-extract:\n%s" % stderr.decode('unicode-escape').strip())

    try:
        output = stdout.decode('utf-8')
        data = json.loads(output)
    except ValueError as e:
        raise RuntimeError("tracker-extract did not return valid JSON data: %s\n"
                           "Output was: %s" % (e, output))

    return data


class TrackerExtractTestCase(ut.TestCase):
    def assertDictHasKey(self, d, key, msg=None):
        if not isinstance(d, dict):
            self.fail("Expected dict, got %s" % d)
        if key not in d:
            standardMsg = "Missing: %s\n" % (key)
            self.fail(self._formatMessage(msg, standardMsg))
        else:
            return

    def assertIsURN(self, supposed_uuid, msg=None):
        import uuid

        try:
            if (supposed_uuid.startswith("<") and supposed_uuid.endswith(">")):
                supposed_uuid = supposed_uuid[1:-1]

            uuid.UUID(supposed_uuid)
        except ValueError:
            standardMsg = "'%s' is not a valid UUID" % (supposed_uuid)
            self.fail(self._formatMessage(msg, standardMsg))

    def assert_extract_result_matches_spec(self, spec, result, filename, spec_filename):
        """
        Checks tracker-extract json-ld output against the expected result.

        Use get_tracker_extract_jsonld_output() to get the extractor output.

        Look in test-extraction-data/*/*.expected.json for examples of the spec
        format.
        """

        error_missing_prop = "Property '%s' hasn't been extracted from file \n'%s'\n (requested on '%s')"
        error_wrong_value = "on property '%s' from file %s\n (requested on: '%s')"
        error_wrong_length = "Length mismatch on property '%s' from file %s\n (requested on: '%s')"
        error_extra_prop = "Property '%s' was explicitely banned for file \n'%s'\n (requested on '%s')"
        error_extra_prop_v = "Property '%s' with value '%s' was explicitely banned for file \n'%s'\n (requested on %s')"

        expected_pairs = []  # List of expected (key, value)
        unexpected_pairs = []  # List of unexpected (key, value)
        expected_keys = []  # List of expected keys (the key must be there, value doesnt matter)

        for k, v in list(spec.items()):
            if k.startswith("!"):
                unexpected_pairs.append((k[1:], v))
            elif k == '@type':
                expected_keys.append('@type')
            else:
                expected_pairs.append((k, v))

        for prop, expected_value in expected_pairs:
            self.assertDictHasKey(result, prop,
                                    error_missing_prop % (prop, filename, spec_filename))
            if expected_value == "@URNUUID@":
                self.assertIsURN(result[prop][0]['@id'],
                                    error_wrong_value % (prop, filename, spec_filename))
            else:
                if isinstance(expected_value, list):
                    if not isinstance(result[prop], list):
                        raise AssertionError("Expected a list property for %s, but got a %s: %s" % (
                            prop, type(result[prop]).__name__, result[prop]))

                    self.assertEqual(len(expected_value), len(result[prop]),
                                        error_wrong_length % (prop, filename, spec_filename))

                    for i in range(0, len(expected_value)):
                        if isinstance(expected_value[i], dict):
                            self.assert_extract_result_matches_spec(expected_value[i], result[prop][i], filename, spec_filename)
                        else:
                            self.assertEqual(str(expected_value[i]), str(result[prop][i]),
                                             error_wrong_value % (prop, filename, spec_filename))
                elif isinstance(expected_value, dict):
                    self.assert_extract_result_matches_spec(expected_value, result[prop], filename, spec_filename)
                else:
                    self.assertEqual(str(spec[prop]), str(result[prop]),
                                        error_wrong_value % (prop, filename, spec_filename))

        for (prop, value) in unexpected_pairs:
            # There is no prop, or it is but not with that value
            if (value == ""):
                self.assertFalse(prop in result,
                                 error_extra_prop % (prop, filename, spec_filename))
            else:
                if (value == "@URNUUID@"):
                    self.assertIsURN(result[prop][0],
                                     error_extra_prop % (prop, filename, spec_filename))
                else:
                    self.assertNotIn(value, result[prop],
                                     error_extra_prop_v % (prop, value, filename, spec_filename))

        for prop in expected_keys:
            self.assertDictHasKey(result, prop,
                                  error_missing_prop % (prop, filename, spec_filename))


def create_test_flac(path, duration, timeout=10):
    """
    Create a .flac audio file for testing purposes.

    FLAC audio doesn't compress test data particularly efficiently, so
    committing an audio file more than a few seconds long to Git is not
    practical. This function creates a .flac file containing a test tone.
    The 'duration' parameter sets the length in seconds of the time.

    The function is guaranteed to return or raise an exception within the
    number of seconds given in the 'timeout' parameter.
    """

    Gst.init([])

    num_buffers = math.ceil(duration * 44100 / 1024.0)

    pipeline_src = ' ! '.join([
        'audiotestsrc num-buffers=%s samplesperbuffer=1024' % num_buffers,
        'capsfilter caps="audio/x-raw,rate=44100"',
        'flacenc',
        'filesink location=%s' % path,
    ])

    log("Running pipeline: %s" % pipeline_src)
    pipeline = Gst.parse_launch(pipeline_src)
    ret = pipeline.set_state(Gst.State.PLAYING)

    msg = pipeline.get_bus().poll(Gst.MessageType.ERROR | Gst.MessageType.EOS,
                                timeout * Gst.SECOND)
    if msg and msg.type == Gst.MessageType.EOS:
        pass
    elif msg and msg.type == Gst.MessageType.ERROR:
        raise RuntimeError(msg.parse_error())
    elif msg:
        raise RuntimeError("Got unexpected GStreamer message %s" % msg.type)
    else:
        raise RuntimeError("Timeout generating test audio file after %i seconds" % timeout)

    pipeline.set_state(Gst.State.NULL)
