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
gi.require_version('Gio', '2.0')
gi.require_version('Gst', '1.0')
gi.require_version('Tracker', '3.0')
from gi.repository import Gio, GLib
from gi.repository import Tracker

import errno
import json
import logging
import os
import pathlib
import shutil
import subprocess
import sys
import tempfile
import time
import unittest as ut

import trackertestutils.helpers
import configuration as cfg
import helpers

log = logging.getLogger(__name__)

AUDIO_GRAPH = "http://tracker.api.gnome.org/ontology/v3/tracker#Audio"
DOCUMENTS_GRAPH = "http://tracker.api.gnome.org/ontology/v3/tracker#Documents"
PICTURES_GRAPH = "http://tracker.api.gnome.org/ontology/v3/tracker#Pictures"
FILESYSTEM_GRAPH = "http://tracker.api.gnome.org/ontology/v3/tracker#FileSystem"

def tracker_test_main():
    """Entry point which must be called by all functional test modules."""
    if cfg.tests_verbose():
        # Output all logs to stderr
        logging.basicConfig(stream=sys.stderr, level=logging.DEBUG)
    else:
        # Output some messages from D-Bus daemon to stderr by default. In practice,
        # only errors and warnings should be output here unless the environment
        # contains G_MESSAGES_DEBUG= and/or TRACKER_VERBOSITY=1 or more.
        handler_stderr = logging.StreamHandler(stream=sys.stderr)
        handler_stderr.addFilter(logging.Filter('trackertestutils.dbusdaemon.stderr'))
        handler_stdout = logging.StreamHandler(stream=sys.stderr)
        handler_stdout.addFilter(logging.Filter('trackertestutils.dbusdaemon.stdout'))
        logging.basicConfig(level=logging.INFO,
                            handlers=[handler_stderr, handler_stdout],
                            format='%(message)s')

    # Avoid the test process sending itself SIGTERM via the GDBusConnection
    # 'exit-on-close' feature.
    #
    # This can happen if something calls a g_bus_get_*() function -- I saw
    # problems when I started using GFile in a test, which causes GIO and GVFS
    # to connect to the session bus in the background. There may be an
    # underlying bug at work, but anyway it's important that our tests don't
    # randomly terminate themselves.
    #
    session_bus = Gio.bus_get_sync(Gio.BusType.SESSION, None)
    session_bus.set_exit_on_close(False)

    ut.main(failfast=True, verbosity=2)


class TrackerMinerTest(ut.TestCase):
    def __init__(self, *args, **kwargs):
        super(TrackerMinerTest, self).__init__(*args, **kwargs)

        self.workdir = cfg.create_monitored_test_dir()
        self.indexed_dir = os.path.join(self.workdir, 'test-monitored')

    def config(self):
        settings = {
            'org.freedesktop.Tracker3.Miner.Files': {
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
        extra_env = cfg.test_environment(self.workdir)
        extra_env['LANG'] = 'en_GB.utf8'

        self.sandbox = trackertestutils.helpers.TrackerDBusSandbox(
            session_bus_config_file=cfg.TEST_DBUS_DAEMON_CONFIG_FILE, extra_env=extra_env)

        self.sandbox.start()

        try:
            # It's important that this directory exists BEFORE we start Tracker:
            # it won't monitor an indexing root for changes if it doesn't exist,
            # it'll silently ignore it instead. See the tracker_crawler_start()
            # function.
            os.makedirs(self.indexed_dir, exist_ok=True)

            self.sandbox.set_config(self.config())

            self.miner_fs = helpers.MinerFsHelper(self.sandbox.get_session_bus_connection())
            self.miner_fs.start()

            self.extractor = helpers.ExtractorHelper(self.sandbox.get_session_bus_connection())

            self.tracker = trackertestutils.helpers.StoreHelper(
                self.miner_fs.get_sparql_connection())
        except Exception:
            self.sandbox.stop()
            raise

    def tearDown(self):
        self.sandbox.stop()
        cfg.remove_monitored_test_dir(self.workdir)

    def path(self, filename):
        return os.path.join(self.workdir, filename)

    def uri(self, filename):
        return "file://" + os.path.join(self.workdir, filename)

    def assertResourceExists(self, urn):
        if self.tracker.ask("ASK { <%s> a rdfs:Resource }" % urn) == False:
            self.fail("Resource <%s> does not exist" % urn)

    def assertResourceMissing(self, urn):
        if self.tracker.ask("ASK { <%s> a rdfs:Resource }" % urn) == True:
            self.fail("Resource <%s> should not exist" % urn)

    def assertFileNotIndexed(self, url_or_path):
        if isinstance(url_or_path, pathlib.Path):
            url = url_or_path.as_uri()
        else:
            assert url_or_path.startswith('file://')
            url = url_or_path

        if self.tracker.ask("ASK { ?r a rdfs:Resource ; nie:url <%s> }" % url) == True:
            self.fail("File <%s> should not be indexed" % url)

    def assertFileIndexed(self, url_or_path):
        if isinstance(url_or_path, pathlib.Path):
            url = url_or_path.as_uri()
        else:
            assert url_or_path.startswith('file://')
            url = url_or_path

        if self.tracker.ask("ASK { ?r a rdfs:Resource ; nie:url <%s> }" % url) == False:
            self.fail("File <%s> should be indexed, but is not." % url)

    def await_document_inserted(self, path, content=None, timeout=cfg.AWAIT_TIMEOUT):
        """Wraps await_insert() context manager."""

        # Safety check, you can get confused if you pass a URI because
        # pathlib.Path.as_uri() will convert a valid URI into an invalid one.
        assert isinstance(path, pathlib.Path) or not path.startswith('file://')
        url = self.uri(path)

        expected = [
            'a nfo:Document',
            f'nie:isStoredAs <{url}>',
        ]

        if content:
            content_escaped = Tracker.sparql_escape_string(content)
            expected += [f'nie:plainTextContent "{content_escaped}"']

        return self.tracker.await_insert(
            DOCUMENTS_GRAPH, '; '.join(expected), timeout=timeout)

    def await_document_uri_change(self, resource_id, from_path, to_path):
        """Wraps await_update() context manager."""
        from_url = self.uri(from_path)
        to_url = self.uri(to_path)
        return self.tracker.await_property_update(DOCUMENTS_GRAPH,
                                                  resource_id,
                                                  f'nie:isStoredAs <{from_url}>',
                                                  f'nie:isStoredAs <{to_url}>',
                                                  timeout=cfg.AWAIT_TIMEOUT)

    def await_photo_inserted(self, path):
        url = self.uri(path)

        expected = [
            'a nmm:Photo',
            f'nie:isStoredAs <{url}>',
        ]

        return self.tracker.await_insert(PICTURES_GRAPH, '; '.join(expected), timeout=cfg.AWAIT_TIMEOUT)


class TrackerMinerFTSTest (TrackerMinerTest):
    """
    Superclass to share methods. Shouldn't be run by itself.
    """

    def setUp(self):
        # It's very important to make this directory BEFORE the miner starts.
        # If a configured root doesn't exist when the miner starts up, it will
        # be ignored even after it's created.
        os.makedirs(self.indexed_dir, exist_ok=True)

        super(TrackerMinerFTSTest, self).setUp()

        self.testfile = "test-monitored/miner-fts-test.txt"

    def set_text(self, text):
        text_escaped = Tracker.sparql_escape_string(text)
        path = pathlib.Path(self.path(self.testfile))

        if path.exists():
            old_text_escaped = Tracker.sparql_escape_string(path.read_text())
            resource_id = self.tracker.get_content_resource_id(self.uri(self.testfile))
            with self.tracker.await_content_update(DOCUMENTS_GRAPH,
                                                   resource_id,
                                                   f'nie:plainTextContent "{old_text_escaped}"',
                                                   f'nie:plainTextContent "{text_escaped}"',
                                                   timeout=cfg.AWAIT_TIMEOUT):
                path.write_text(text)
        else:
            url = self.uri(self.testfile)
            expected = f'a nfo:Document; nie:isStoredAs <{url}>; nie:plainTextContent "{text_escaped}"'
            with self.tracker.await_insert(DOCUMENTS_GRAPH, expected, timeout=cfg.AWAIT_TIMEOUT):
                path.write_text(text)

    def search_word(self, word):
        """
        Return list of URIs with the word in them
        """
        log.info("Search for: %s", word)
        results = self.tracker.query("""
                SELECT ?url WHERE {
                  ?u a nfo:TextDocument ;
                      nie:isStoredAs ?url ;
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
        query = "SELECT tracker:id(?urn) WHERE { ?urn nie:isStoredAs/nie:url \"%s\". }" % uri
        result = self.tracker.query(query)
        assert len(result) == 1
        return int(result[0][0])


def get_tracker_extract_jsonld_output(extra_env, filename, mime_type=None):
    """
    Runs `tracker-extract --file` to extract metadata from a file.
    """

    tracker_extract = os.path.join(cfg.TRACKER_EXTRACT_PATH)
    command = [tracker_extract, '--output-format=json-ld', '--file', str(filename)]
    if mime_type is not None:
        command.extend(['--mime', mime_type])

    # We depend on parsing the output, so we must avoid the GLib log handler
    # writing stuff to stdout.
    extra_env['G_MESSAGES_DEBUG'] = ''

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
    WRITEBACK_BUSNAME = "org.freedesktop.Tracker3.Writeback"
    WRITEBACK_PATH = "/org/freedesktop/Tracker3/Writeback"
    WRITEBACK_IFACE = "org.freedesktop.Tracker3.Writeback"

    def setUp(self):
        super(TrackerWritebackTest, self).setUp()
        self.writeback_proxy = Gio.DBusProxy.new_sync(
            self.sandbox.get_session_bus_connection(),
            Gio.DBusProxyFlags.DO_NOT_AUTO_START_AT_CONSTRUCTION, None,
            self.WRITEBACK_BUSNAME, self.WRITEBACK_PATH, self.WRITEBACK_IFACE)

    def datadir_path(self, filename):
        """Returns the full path to a writeback test file."""
        datadir = os.path.join(os.path.dirname(__file__), 'test-writeback-data')
        return pathlib.Path(os.path.join(datadir, filename))

    def writeback_data(self, variant):
        self.writeback_proxy.Writeback('(a{sv})', variant)

    def prepare_test_audio(self, filename):
        path = pathlib.Path(os.path.join(self.indexed_dir, os.path.basename(filename)))
        url = path.as_uri()

        # Copy and wait.
        expected = f'nie:url <{url}>'
        with self.tracker.await_insert(FILESYSTEM_GRAPH, expected, timeout=cfg.AWAIT_TIMEOUT):
            shutil.copy(filename, self.indexed_dir)
        return path

    def prepare_test_image(self, source_path):
        dest_path = pathlib.Path(os.path.join(self.indexed_dir, os.path.basename(source_path)))
        url = dest_path.as_uri()

        # Copy and wait.
        expected = f'nie:url <{url}>'
        with self.tracker.await_insert(FILESYSTEM_GRAPH, expected, timeout=cfg.AWAIT_TIMEOUT):
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


class CliError(Exception):
    pass


class TrackerCommandLineTestCase(TrackerMinerTest):
    def setUp(self):
        super(TrackerCommandLineTestCase, self).setUp()

        self.env = os.environ.copy()
        self.env.update(cfg.test_environment(self.workdir))

        path = self.env.get('PATH', []).split(':')
        self.env['PATH'] = ':'.join([cfg.cli_dir()] + path)

        self.env['DBUS_SESSION_BUS_ADDRESS'] = self.sandbox.daemon.address

        self.tracker_cli = shutil.which('tracker3', path=self.env['PATH'])

    def run_cli(self, command):
        command = [self.tracker_cli] + [str(c) for c in command]

        log.info("Running: %s", ' '.join(command))
        result = subprocess.run(command, stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE, env=self.env)

        if len(result.stdout) > 0:
            log.debug("stdout: %s", result.stdout.decode('utf-8'))
        if len(result.stderr) > 0:
            log.debug("stderr: %s", result.stderr.decode('utf-8'))

        if result.returncode != 0:
            error = result.stderr.decode('utf-8')
            if len(error) == 0:
                error = result.stdout.decode('utf-8')
            raise CliError('\n'.join([
                "CLI command failed.",
                "Command: %s" % ' '.join(command),
                "Error: %s" % error]))

        return result.stdout.decode('utf-8')
