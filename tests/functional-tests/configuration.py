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


import errno
import json
import logging
import os
import shutil
import tempfile
import sys


DEFAULT_TIMEOUT = 10


if 'TRACKER_FUNCTIONAL_TEST_CONFIG' not in os.environ:
    raise RuntimeError("The TRACKER_FUNCTIONAL_TEST_CONFIG environment "
                       "variable must be set to point to the location of "
                       "the generated configuration.json file.")

with open(os.environ['TRACKER_FUNCTIONAL_TEST_CONFIG']) as f:
    config = json.load(f)


TEST_DBUS_DAEMON_CONFIG_FILE = config['TEST_DBUS_DAEMON_CONFIG_FILE']
TRACKER_EXTRACT_PATH = config['TRACKER_EXTRACT_PATH']


def cli_dir():
    return config['TEST_CLI_DIR']


def test_environment(tmpdir):
    return {
        'DCONF_PROFILE': config['TEST_DCONF_PROFILE'],
        'TRACKER_CLI_SUBCOMMANDS_DIR': config['TEST_CLI_SUBCOMMANDS_DIR'],
        'TRACKER_TEST_DOMAIN_ONTOLOGY_RULE': config['TEST_DOMAIN_ONTOLOGY_RULE'],
        'TRACKER_EXTRACTOR_RULES_DIR': config['TEST_EXTRACTOR_RULES_DIR'],
        'TRACKER_EXTRACTORS_DIR': config['TEST_EXTRACTORS_DIR'],
        'GSETTINGS_SCHEMA_DIR': config['TEST_GSETTINGS_SCHEMA_DIR'],
        'TRACKER_LANGUAGE_STOP_WORDS_DIR': config['TEST_LANGUAGE_STOP_WORDS_DIR'],
        'TRACKER_MINER_SERVICES_DIR': config['TEST_MINER_SERVICES_DIR'],
        'TRACKER_WRITEBACK_MODULES_DIR': config['TEST_WRITEBACK_MODULES_DIR'],
        'XDG_CACHE_HOME': os.path.join(tmpdir, 'cache'),
        'XDG_CONFIG_HOME': os.path.join(tmpdir, 'config'),
        'XDG_DATA_HOME': os.path.join(tmpdir, 'data'),
        'XDG_RUNTIME_DIR': os.path.join(tmpdir, 'run'),
    }


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
    shutil.rmtree(path)

    # We delete the parent directory if possible, to avoid cluttering the user's
    # home dir, but there may be other tests running in parallel so we ignore
    # an error if there are still files present in it.
    try:
        os.rmdir(_TEST_MONITORED_TMP_DIR)
    except OSError as e:
        if e.errno == errno.ENOTEMPTY:
            pass


def get_environment_boolean(variable):
    '''Parse a yes/no boolean passed through the environment.'''

    value = os.environ.get(variable, 'no').lower()
    if value in ['no', '0', 'false']:
        return False
    elif value in ['yes', '1', 'true']:
        return True
    else:
        raise RuntimeError('Unexpected value for %s: %s' %
                           (variable, value))


def get_environment_int(variable, default=0):
    try:
        return int(os.environ.get(variable))
    except (TypeError, ValueError):
        return default


def tests_verbose():
    return get_environment_boolean('TRACKER_TESTS_VERBOSE')
