#
# Copyright (C) 2010, Nokia <ivan.frade@nokia.com>
# Copyright (C) 2018-2020 Sam Thursfield <sam@afuera.me.uk>
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
Fixtures used by the tracker-miners functional-tests.
"""

import gi
gi.require_version('Gst', '1.0')
gi.require_version('Tracker', '3.0')
from gi.repository import GLib
from gi.repository import Tracker

import errno
import json
import logging
import os
import pathlib
import shutil
import subprocess
import time
import unittest as ut
from itertools import chain

import trackertestutils.dconf
import trackertestutils.helpers
import configuration as cfg
from minerfshelper import MinerFsHelper

log = logging.getLogger(__name__)


DEFAULT_TEXT = "Some stupid content, to have a test file"


def ensure_dir_exists(dirname):
    if not os.path.exists(dirname):
        os.makedirs(dirname)


class TrackerMinerTest(ut.TestCase):
    def config(self):
        settings = {
            'org.freedesktop.Tracker.Miner.Files': {
                'enable-writeback': GLib.Variant.new_boolean(False),
                'index-recursive-directories': GLib.Variant.new_strv([self.indexed_dir]),
                'index-single-directories': GLib.Variant.new_strv([]),
                'index-optical-discs': GLib.Variant.new_boolean(False),
                'index-removable-devices': GLib.Variant.new_boolean(False),
                'index-applications': GLib.Variant.new_boolean(False),
                'throttle': GLib.Variant.new_int32(5),
            }
        }
        return settings

    def setUp(self):
        self.workdir = cfg.create_monitored_test_dir()

        self.indexed_dir = os.path.join(self.workdir, 'test-monitored')

        # It's important that this directory exists BEFORE we start Tracker:
        # it won't monitor an indexing root for changes if it doesn't exist,
        # it'll silently ignore it instead. See the tracker_crawler_start()
        # function.
        ensure_dir_exists(self.indexed_dir)

        try:
            extra_env = cfg.test_environment(self.workdir)
            extra_env['LANG'] = 'en_GB.utf8'

            self.sandbox = trackertestutils.helpers.TrackerDBusSandbox(
                dbus_daemon_config_file=cfg.TEST_DBUS_DAEMON_CONFIG_FILE, extra_env=extra_env)

            self.sandbox.start()

            try:
                for schema_name, contents in self.config().items():
                    dconf = trackertestutils.dconf.DConfClient(self.sandbox)
                    for key, value in contents.items():
                        dconf.write(schema_name, key, value)

                # We must create the test data before the miner does its
                # initial crawl, or it may miss some files due
                # https://gitlab.gnome.org/GNOME/tracker-miners/issues/79.
                monitored_files = self.create_test_data()

                self.miner_fs = MinerFsHelper(self.sandbox.get_connection())
                self.miner_fs.start()
                self.miner_fs.start_watching_progress()

                self.tracker = trackertestutils.helpers.StoreHelper(
                    self.miner_fs.get_sparql_connection())

                for tf in monitored_files:
                    url = self.uri(tf)
                    self.tracker.ensure_resource(f"a nfo:Document ; nie:url <{url}>")
            except Exception:
                self.sandbox.stop()
                raise
        except Exception:
            self.remove_test_data()
            cfg.remove_monitored_test_dir(self.workdir)
            raise

    def tearDown(self):
        self.sandbox.stop()
        self.remove_test_data()
        cfg.remove_monitored_test_dir(self.workdir)

    def path(self, filename):
        return os.path.join(self.workdir, filename)

    def uri(self, filename):
        return "file://" + os.path.join(self.workdir, filename)

    def create_test_data(self):
        monitored_files = [
            'test-monitored/file1.txt',
            'test-monitored/dir1/file2.txt',
            'test-monitored/dir1/dir2/file3.txt'
        ]

        unmonitored_files = [
            'test-no-monitored/file0.txt'
        ]

        for tf in chain(monitored_files, unmonitored_files):
            testfile = self.path(tf)
            ensure_dir_exists(os.path.dirname(testfile))
            with open(testfile, 'w') as f:
                f.write(DEFAULT_TEXT)

        return monitored_files

    def remove_test_data(self):
        try:
            shutil.rmtree(os.path.join(self.workdir, 'test-monitored'))
            shutil.rmtree(os.path.join(self.workdir, 'test-no-monitored'))
        except Exception as e:
            log.warning("Failed to remove temporary data dir: %s", e)

    def assertResourceExists(self, urn):
        if self.tracker.ask("ASK { <%s> a rdfs:Resource }" % urn) == False:
            self.fail("Resource <%s> does not exist" % urn)

    def assertResourceMissing(self, urn):
        if self.tracker.ask("ASK { <%s> a rdfs:Resource }" % urn) == True:
            self.fail("Resource <%s> should not exist" % urn)

    def await_document_inserted(self, path, content=None):
        """Wraps await_insert() context manager."""
        url = self.uri(path)

        expected = [
            'a nfo:Document',
            f'nie:url <{url}>',
        ]

        if content:
            content_escaped = Tracker.sparql_escape_string(content)
            expected += [f'nie:plainTextContent "{content_escaped}"']

        return self.tracker.await_insert('; '.join(expected))

    def await_document_uri_change(self, resource_id, from_path, to_path):
        """Wraps await_update() context manager."""
        from_url = self.uri(from_path)
        to_url = self.uri(to_path)
        return self.tracker.await_update(resource_id,
                                         f'nie:url <{from_url}>',
                                         f'nie:url <{to_url}>')

    def await_photo_inserted(self, path):
        url = self.uri(path)

        expected = [
            'a nmm:Photo',
            f'nie:url <{url}>',
        ]

        return self.tracker.await_insert('; '.join(expected))


class TrackerMinerFTSTest (TrackerMinerTest):
    """
    Superclass to share methods. Shouldn't be run by itself.
    """

    def prepare_directories(self):
        # Override content from the base class
        pass

    def setUp(self):
        super(TrackerMinerFTSTest, self).setUp()

        self.testfile = "test-monitored/miner-fts-test.txt"

    def set_text(self, text):
        text_escaped = Tracker.sparql_escape_string(text)
        path = pathlib.Path(self.path(self.testfile))

        if path.exists():
            old_text_escaped = Tracker.sparql_escape_string(path.read_text())
            resource_id = self.tracker.get_resource_id(self.uri(self.testfile))
            with self.tracker.await_update(resource_id,
                                           f'nie:plainTextContent "{old_text_escaped}"',
                                           f'nie:plainTextContent "{text_escaped}"'):
                path.write_text(text)
        else:
            url = self.uri(self.testfile)
            expected = f'a nfo:Document; nie:url <{url}>; nie:plainTextContent "{text_escaped}"'
            with self.tracker.await_insert(expected):
                path.write_text(text)

    def search_word(self, word):
        """
        Return list of URIs with the word in them
        """
        log.info("Search for: %s", word)
        results = self.tracker.query("""
                SELECT ?url WHERE {
                  ?u a nfo:TextDocument ;
                      nie:url ?url ;
                      fts:match '%s'.
                 }
                 """ % (word))
        return [r[0] for r in results]

    def basic_test(self, text, word):
        """
        Save the text on the testfile, search the word
        and assert the testfile is only result.

        Be careful with the default contents of the text files
        ( see minertest.py DEFAULT_TEXT )
        """
        self.set_text(text)
        results = self.search_word(word)
        self.assertEqual(len(results), 1)
        self.assertIn(self.uri(self.testfile), results)

    def _query_id(self, uri):
        query = "SELECT tracker:id(?urn) WHERE { ?urn nie:url \"%s\". }" % uri
        result = self.tracker.query(query)
        assert len(result) == 1
        return int(result[0][0])


def get_tracker_extract_jsonld_output(extra_env, filename, mime_type=None):
    """
    Runs `tracker-extract --file` to extract metadata from a file.
    """

    tracker_extract = os.path.join(cfg.TRACKER_EXTRACT_PATH)
    command = [tracker_extract, '--verbosity=0', '--output-format=json-ld', '--file', str(filename)]
    if mime_type is not None:
        command.extend(['--mime', mime_type])

    # We depend on parsing the output, so verbosity MUST be 0.
    extra_env['TRACKER_VERBOSITY'] = '0'
    # Tell GStreamer not to fork to create the registry
    extra_env['GST_REGISTRY_FORK'] = 'no'
    log.debug('Adding to environment: %s', ' '.join('%s=%s' % (k, v) for k, v in extra_env.items()))

    env = os.environ.copy()
    env.update(extra_env)

    log.debug('Running: %s', ' '.join(command))
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
            "Error output:\n%s\n" % (p.returncode, stderr.decode('utf-8').strip()))

    if len(stderr) > 0:
        error_output = stderr.decode('utf-8').strip()
        log.debug("Error output from tracker-extract:\n%s", error_output)

    try:
        output = stdout.decode('utf-8')

        if len(output.strip()) == 0:
            raise RuntimeError("tracker-extract didn't return any data.\n"
                               "Error output was: %s" % error_output)

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
            import pdb
            pdb.set_trace()

            standardMsg = "Missing: %s" % (key)
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


TEST_FILE_JPEG = "writeback-test-1.jpeg"
TEST_FILE_TIFF = "writeback-test-2.tif"
TEST_FILE_PNG = "writeback-test-4.png"


class TrackerWritebackTest (TrackerMinerTest):
    """
    Superclass to share methods. Shouldn't be run by itself.
    Start all processes including writeback, miner pointing to WRITEBACK_TMP_DIR
    """

    def config(self):
        values = super(TrackerWritebackTest, self).config()
        values['org.freedesktop.Tracker.Miner.Files']['enable-writeback'] = GLib.Variant.new_boolean(True)
        return values

    def create_test_data(self):
        return []

    def remove_test_data(self):
        for test_file in pathlib.Path(self.indexed_dir).iterdir():
            test_file.unlink()

    def datadir_path(self, filename):
        """Returns the full path to a writeback test file."""
        datadir = os.path.join(os.path.dirname(__file__), 'test-writeback-data')
        return pathlib.Path(os.path.join(datadir, filename))

    def prepare_test_audio(self, filename):
        path = pathlib.Path(os.path.join(self.indexed_dir, os.path.basename(filename)))
        url = path.as_uri()

        # Copy and wait. The extractor adds the nfo:duration property.
        expected = f'a nfo:Audio ; nie:url <{url}> ; nfo:duration ?duration'
        with self.tracker.await_insert(expected):
            shutil.copy(path, self.indexed_dir)
        return path

    def prepare_test_image(self, source_path):
        dest_path = pathlib.Path(os.path.join(self.indexed_dir, os.path.basename(source_path)))
        url = dest_path.as_uri()

        # Copy and wait. The extractor adds the nfo:width property.
        expected = f'a nfo:Image ; nie:url <{url}> ; nfo:width ?width'
        with self.tracker.await_insert(expected):
            shutil.copy(source_path, self.indexed_dir)
        return dest_path

    def uri(self, filename):
        return pathlib.Path(filename).as_uri()

    def get_mtime(self, filename):
        return os.stat(filename).st_mtime

    def wait_for_file_change(self, filename, initial_mtime):
        start = time.time()
        while time.time() < start + 5:
            mtime = os.stat(filename).st_mtime
            if mtime > initial_mtime:
                return
            time.sleep(0.2)

        raise Exception(
            "Timeout waiting for %s to be updated (mtime has not changed)" %
            filename)


# Copy rate, 10KBps (1024b/100ms)
SLOWCOPY_RATE = 1024


class TrackerApplicationTest (TrackerWritebackTest):
    def get_urn_count_by_url(self, url):
        select = """
        SELECT ?u WHERE { ?u nie:url \"%s\" }
        """ % (url)
        return len(self.tracker.query(select))

    def get_test_image(self):
        TEST_IMAGE = "test-image-1.jpg"
        return TEST_IMAGE

    def get_test_video(self):
        TEST_VIDEO = "test-video-1.mp4"
        return TEST_VIDEO

    def get_test_music(self):
        TEST_AUDIO = "test-music-1.mp3"
        return TEST_AUDIO

    def get_data_dir(self):
        return self.datadir

    def get_dest_dir(self):
        return self.indexed_dir

    def slowcopy_file_fd(self, src, fdest, rate=SLOWCOPY_RATE):
        """
        @rate: bytes per 100ms
        """
        log.debug("Copying slowly\n '%s' to\n '%s'", src, fdest.name)
        fsrc = open(src, 'rb')
        buffer_ = fsrc.read(rate)
        while (buffer_ != b""):
            fdest.write(buffer_)
            time.sleep(0.1)
            buffer_ = fsrc.read(rate)
        fsrc.close()

    def slowcopy_file(self, src, dst, rate=SLOWCOPY_RATE):
        """
        @rate: bytes per 100ms
        """
        fdest = open(dst, 'wb')
        self.slowcopy_file_fd(src, fdest, rate)
        fdest.close()

    def setUp(self):
        # Use local directory if available. Installation otherwise.
        if os.path.exists(os.path.join(os.getcwd(),
                                       "test-apps-data")):
            self.datadir = os.path.join(os.getcwd(),
                                        "test-apps-data")
        else:
            self.datadir = os.path.join(cfg.DATADIR,
                                        "tracker-tests",
                                        "test-apps-data")

        super(TrackerApplicationTest, self).setUp()
