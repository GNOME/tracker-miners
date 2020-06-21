#
# Copyright (C) 2010, Nokia <jean-luc.lamadon@nokia.com>
# Copyright (C) 2019, Sam Thursfield (sam@afuera.me.uk)
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


from gi.repository import GLib

import errno
import json
import logging
import os
import shutil
import tempfile
import sys


if 'TRACKER_FUNCTIONAL_TEST_CONFIG' not in os.environ:
    raise RuntimeError("The TRACKER_FUNCTIONAL_TEST_CONFIG environment "
                       "variable must be set to point to the location of "
                       "the generated configuration.json file.")

with open(os.environ['TRACKER_FUNCTIONAL_TEST_CONFIG']) as f:
    config = json.load(f)


TEST_DBUS_DAEMON_CONFIG_FILE = config['TEST_DBUS_DAEMON_CONFIG_FILE']
TRACKER_EXTRACT_PATH = config['TRACKER_EXTRACT_PATH']


def test_environment(tmpdir):
    return {
        'DCONF_PROFILE': config['TEST_DCONF_PROFILE'],
        'TRACKER_TEST_DOMAIN_ONTOLOGY_RULE': config['TEST_DOMAIN_ONTOLOGY_RULE'],
        'TRACKER_EXTRACTOR_RULES_DIR': config['TEST_EXTRACTOR_RULES_DIR'],
        'TRACKER_EXTRACTORS_DIR': config['TEST_EXTRACTORS_DIR'],
        'GSETTINGS_SCHEMA_DIR': config['TEST_GSETTINGS_SCHEMA_DIR'],
        'TRACKER_LANGUAGE_STOP_WORDS_DIR': config['TEST_LANGUAGE_STOP_WORDS_DIR'],
        'TRACKER_WRITEBACK_MODULES_DIR': config['TEST_WRITEBACK_MODULES_DIR'],
        'XDG_CACHE_HOME': os.path.join(tmpdir, 'cache'),
        'XDG_CONFIG_HOME': os.path.join(tmpdir, 'config'),
        'XDG_DATA_HOME': os.path.join(tmpdir, 'data'),
        'XDG_RUNTIME_DIR': os.path.join(tmpdir, 'run'),
    }


def cli_dir():
    return config['TEST_CLI_DIR']


def cli_subcommands_dir():
    return config['TEST_CLI_SUBCOMMANDS_DIR']


# This path is used for test data for tests which expect filesystem monitoring
# to work. For this reason we must avoid it being on a tmpfs filesystem. Note
# that this MUST NOT be a hidden directory, as Tracker is hardcoded to ignore
# those. The 'ignore-files' configuration option can be changed, but the
# 'filter-hidden' property of TrackerIndexingTree is hardwired to be True at
# present :/
_TEST_MONITORED_TMP_DIR = os.path.join(os.environ["HOME"], "tracker-tests")
if _TEST_MONITORED_TMP_DIR.startswith('/tmp'):
    if 'REAL_HOME' in os.environ:
        _TEST_MONITORED_TMP_DIR = os.path.join(os.environ["REAL_HOME"], "tracker-tests")
    else:
        print ("HOME is in the /tmp prefix - this will cause tests that rely "
                + "on filesystem monitoring to fail as changes in that prefix are "
                + "ignored.")


def create_monitored_test_dir():
    '''Returns a unique tmpdir which supports filesystem monitor events.'''
    os.makedirs(_TEST_MONITORED_TMP_DIR, exist_ok=True)
    return tempfile.mkdtemp(dir=_TEST_MONITORED_TMP_DIR)


def remove_monitored_test_dir(path):
    if tests_no_cleanup():
        print("\nTRACKER_DEBUG=tests-no-cleanup: Test data kept in %s" % path)
    else:
        shutil.rmtree(path)

        # We delete the parent directory if possible, to avoid cluttering the user's
        # home dir, but there may be other tests running in parallel so we ignore
        # an error if there are still files present in it.
        try:
            os.rmdir(_TEST_MONITORED_TMP_DIR)
        except OSError as e:
            if e.errno == errno.ENOTEMPTY:
                pass


def get_environment_int(variable, default=0):
    try:
        return int(os.environ.get(variable))
    except (TypeError, ValueError):
        return default


# Timeout when awaiting resources. For developers, we want a short default
# so they don't spend a long time waiting for tests to fail. For CI we want
# to set a longer timeout so we don't see failures on slow CI runners.
AWAIT_TIMEOUT = get_environment_int('TRACKER_TESTS_AWAIT_TIMEOUT', default=10)


DEBUG_TESTS = 1
DEBUG_TESTS_NO_CLEANUP = 2

_debug_flags = None
def get_debug_flags():
    """Parse the TRACKER_DEBUG environment variable and return flags."""
    global _debug_flags
    if _debug_flags is None:
        flag_tests = GLib.DebugKey()
        flag_tests.key = 'tests'
        flag_tests.value = DEBUG_TESTS

        flag_tests_no_cleanup = GLib.DebugKey()
        flag_tests_no_cleanup.key = 'tests-no-cleanup'
        flag_tests_no_cleanup.value = DEBUG_TESTS_NO_CLEANUP

        flags = [flag_tests, flag_tests_no_cleanup]
        flags_str = os.environ.get('TRACKER_DEBUG', '')

        _debug_flags = GLib.parse_debug_string(flags_str, flags)
    return _debug_flags


def tests_verbose():
    """True if TRACKER_DEBUG=tests"""
    return (get_debug_flags() & DEBUG_TESTS)


def tests_no_cleanup():
    """True if TRACKER_DEBUG=tests-no-cleanup"""
    return (get_debug_flags() & DEBUG_TESTS_NO_CLEANUP)
