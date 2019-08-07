#!/usr/bin/env python3
#
# Copyright (C) 2010, Nokia <jean-luc.lamadon@nokia.com>
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

"Constants describing Tracker D-Bus services"

import errno
import json
import logging
import os
import tempfile
import sys

from . import options


if 'TRACKER_FUNCTIONAL_TEST_CONFIG' not in os.environ:
    raise RuntimeError("The TRACKER_FUNCTIONAL_TEST_CONFIG environment "
                       "variable must be set to point to the location of "
                       "the generated configuration.json file.")

with open(os.environ['TRACKER_FUNCTIONAL_TEST_CONFIG']) as f:
    config = json.load(f)

TRACKER_BUSNAME = 'org.freedesktop.Tracker1'
TRACKER_OBJ_PATH = '/org/freedesktop/Tracker1/Resources'
RESOURCES_IFACE = "org.freedesktop.Tracker1.Resources"

MINERFS_BUSNAME = "org.freedesktop.Tracker1.Miner.Files"
MINERFS_OBJ_PATH = "/org/freedesktop/Tracker1/Miner/Files"
MINER_IFACE = "org.freedesktop.Tracker1.Miner"
MINERFS_INDEX_OBJ_PATH = "/org/freedesktop/Tracker1/Miner/Files/Index"
MINER_INDEX_IFACE = "org.freedesktop.Tracker1.Miner.Files.Index"

TRACKER_BACKUP_OBJ_PATH = "/org/freedesktop/Tracker1/Backup"
BACKUP_IFACE = "org.freedesktop.Tracker1.Backup"

TRACKER_STATS_OBJ_PATH = "/org/freedesktop/Tracker1/Statistics"
STATS_IFACE = "org.freedesktop.Tracker1.Statistics"

TRACKER_STATUS_OBJ_PATH = "/org/freedesktop/Tracker1/Status"
STATUS_IFACE = "org.freedesktop.Tracker1.Status"

TRACKER_EXTRACT_BUSNAME = "org.freedesktop.Tracker1.Miner.Extract"
TRACKER_EXTRACT_OBJ_PATH = "/org/freedesktop/Tracker1/Miner/Extract"

WRITEBACK_BUSNAME = "org.freedesktop.Tracker1.Writeback"


DCONF_MINER_SCHEMA = "org.freedesktop.Tracker.Miner.Files"

# Autoconf substitutes paths in the configuration.json file without
# expanding variables, so we need to manually insert these.


def expandvars(variable):
    # Note: the order matters!
    result = variable
    for var, value in [("${datarootdir}", RAW_DATAROOT_DIR),
                       ("${exec_prefix}", RAW_EXEC_PREFIX),
                       ("${prefix}", PREFIX),
                       ("@top_builddir@", TOP_BUILDDIR)]:
        result = result.replace(var, value)

    return result


PREFIX = config['PREFIX']
RAW_EXEC_PREFIX = config['RAW_EXEC_PREFIX']
RAW_DATAROOT_DIR = config['RAW_DATAROOT_DIR']
TOP_BUILDDIR = os.environ['TRACKER_FUNCTIONAL_TEST_BUILD_DIR']

TRACKER_EXTRACT_PATH = os.path.normpath(expandvars(config['TRACKER_EXTRACT_PATH']))
TRACKER_MINER_FS_PATH = os.path.normpath(expandvars(config['TRACKER_MINER_FS_PATH']))
TRACKER_STORE_PATH = os.path.normpath(expandvars(config['TRACKER_STORE_PATH']))
TRACKER_WRITEBACK_PATH = os.path.normpath(expandvars(config['TRACKER_WRITEBACK_PATH']))

DATADIR = os.path.normpath(expandvars(config['RAW_DATAROOT_DIR']))


def generated_ttl_dir():
    return os.path.join(TOP_BUILD_DIR, 'tests', 'functional-tests', 'ttl')


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
    try:
        os.makedirs(_TEST_MONITORED_TMP_DIR)
    except OSError as e:
        if e.errno == errno.EEXIST:
            pass
        else:
            raise
    return tempfile.mkdtemp(dir=_TEST_MONITORED_TMP_DIR)


def remove_monitored_test_dir(path):
    # This will fail if the directory is not empty.
    os.rmdir(path)

    # We delete the parent directory if possible, to avoid cluttering the user's
    # home dir, but there may be other tests running in parallel so we ignore
    # an error if there are still files present in it.
    try:
        os.rmdir(_TEST_MONITORED_TMP_DIR)
    except OSError as e:
        if e.errno == errno.ENOTEMPTY:
            pass


if options.get_environment_boolean('TRACKER_TESTS_VERBOSE'):
    logging.basicConfig(stream=sys.stderr, level=logging.DEBUG)
