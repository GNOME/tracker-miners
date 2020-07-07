#
# Copyright (C) 2010, Nokia <ivan.frade@nokia.com>
# Copyright (C) 2018,2020 Sam Thursfield <sam@afuera.me.uk>
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


import gi
gi.require_version('Tracker', '3.0')
from gi.repository import Gio, GLib, GObject
from gi.repository import Tracker

import dataclasses
import logging
import os
import pathlib
import subprocess

import trackertestutils.mainloop

import configuration


log = logging.getLogger(__name__)


class WakeupCycleTimeoutException(RuntimeError):
    pass


@dataclasses.dataclass
class FileProcessedResult():
    path : pathlib.Path
    status : bool


class FileProcessedError(Exception):
    pass


class await_files_processed():
    """Context manager to await file processing by Tracker miners & extractors.

    """
    def __init__(self, miner, expected, timeout=None):
        self.miner = miner
        self.expected = expected
        self.timeout = timeout or configuration.AWAIT_TIMEOUT

        self.loop = trackertestutils.mainloop.MainLoop()

        self.gfiles = []
        self.status = {}
        for result in self.expected:
            gfile = Gio.File.new_for_path(str(result.path))
            self.gfiles.append(gfile)
            self.status[gfile.get_path()] = result.status

    def __enter__(self):
        if len(self.expected) == 1:
            log.info("Awaiting files-processed signal from %s for file %s",
                     self.miner, self.expected[0].path)
        else:
            log.info("Awaiting %i files-processed signals from %s",
                     len(self.expected), self.miner)

        def signal_cb(proxy, sender_name, signal_name, parameters):
            if signal_name == 'FilesProcessed':
                array = parameters.unpack()[0]
                for uri, success, message in array:
                    log.debug("Processing file-processed event for %s", uri)

                    matched = False

                    signal_gfile = Gio.File.new_for_uri(uri)

                    expected_gfile = None
                    expected_status = None
                    for expected_gfile in self.gfiles:
                        if expected_gfile.get_path() == signal_gfile.get_path():
                            expected_status = self.status[expected_gfile.get_path()]
                            if success == expected_status:
                                log.debug("Matched %s", uri)
                                matched = True
                                break
                            else:
                                raise FileProcessedError(f"{uri}: Expected status {expected_status}, got {success}")

                    if matched:
                        self.gfiles.remove(expected_gfile)

                    if len(self.gfiles) == 0:
                        log.info("All files were processed, exiting loop.")
                        self.loop.quit()

        def timeout_cb():
            log.info("Timeout fired after %s seconds", self.timeout)
            raise trackertestutils.helpers.AwaitTimeoutException(
                f"Timeout awaiting files-processed signal on {self.miner}")

        self.signal_id = self.miner.connect('g-signal', signal_cb)
        self.timeout_id = GLib.timeout_add_seconds(self.timeout, timeout_cb)

        return

    def __exit__(self, etype, evalue, etraceback):
        if etype is not None:
            return False

        self.loop.run_checked()
        log.debug("Main loop finished")

        GLib.source_remove(self.timeout_id)
        GObject.signal_handler_disconnect(self.miner, self.signal_id)

        return True


def await_bus_name(connection, name):
    loop = trackertestutils.mainloop.MainLoop()

    def appeared_cb(connection, name, name_owner):
        log.debug("%s appeared (owner: %s)", name, name_owner)
        loop.quit()

    def vanished_cb(connection, name):
        log.debug("%s vanished", name)

    Gio.bus_watch_name_on_connection(
        connection, name, Gio.BusNameWatcherFlags.NONE, appeared_cb,
        vanished_cb);
    loop.run_checked()


class MinerFsHelper ():

    MINERFS_BUSNAME = "org.freedesktop.Tracker3.Miner.Files"
    MINERFS_OBJ_PATH = "/org/freedesktop/Tracker3/Miner/Files"
    MINER_IFACE = "org.freedesktop.Tracker3.Miner"
    MINERFS_INDEX_OBJ_PATH = "/org/freedesktop/Tracker3/Miner/Files/Index"
    MINER_INDEX_IFACE = "org.freedesktop.Tracker3.Miner.Files.Index"

    def __init__(self, dbus_connection):
        self.log = logging.getLogger(__name__)

        self.bus = dbus_connection

        self.loop = trackertestutils.mainloop.MainLoop()

        self.miner_iface = Gio.DBusProxy.new_sync(
            self.bus, Gio.DBusProxyFlags.DO_NOT_AUTO_START_AT_CONSTRUCTION, None,
            self.MINERFS_BUSNAME, self.MINERFS_OBJ_PATH, self.MINER_IFACE)

        self.index = Gio.DBusProxy.new_sync(
            self.bus, Gio.DBusProxyFlags.DO_NOT_AUTO_START_AT_CONSTRUCTION, None,
            self.MINERFS_BUSNAME, self.MINERFS_INDEX_OBJ_PATH, self.MINER_INDEX_IFACE)

    def start(self):
        if 'TRACKER_TESTS_MINER_FS_COMMAND' in os.environ:
            # This can be used to manually run the miner-fs instead of using
            # D-Bus autoactivation. Useful if you want to use a debugging tool.
            # The process should exit when sandbox D-Bus daemon exits.
            command = os.environ['TRACKER_TESTS_MINER_FS_COMMAND']
            logging.info("Manually starting tracker-miner-fs using TRACKER_TESTS_MINER_FS_COMMAND %s", command)
            p = subprocess.Popen(command, shell=True)
            if p.poll():
                raise RuntimeError("Error manually starting miner-fs")
            # Wait for the process to connect to D-Bus. Autoactivation has a
            # hard timeout of 25 seconds, which can be too short if running
            # under Valgrind.
            await_bus_name(self.bus, self.MINERFS_BUSNAME)

        self.miner_iface.Start()

    def stop(self):
        self.miner_iface.Stop()

    def get_sparql_connection(self):
        return Tracker.SparqlConnection.bus_new(
            'org.freedesktop.Tracker3.Miner.Files', None, self.bus)

    def index_file(self, uri):
        log.debug("IndexFile(%s)", uri)
        return self.index.IndexFile('(s)', uri)

    def index_file_for_process(self, uri):
        log.debug("IndexFileForProcess(%s)", uri)
        return self.index.IndexFileForProcess('(s)', uri)

    def await_file_processed(self, path, status=True):
        expected = [FileProcessedResult(path, status)]
        return await_files_processed(self.miner_iface, expected)

    def await_files_processed(self, expected):
        return await_files_processed(self.miner_iface, expected)


class ExtractorHelper ():

    EXTRACTOR_BUSNAME = "org.freedesktop.Tracker3.Miner.Extract"
    EXTRACTOR_OBJ_PATH = "/org/freedesktop/Tracker3/Miner/Extract"
    MINER_IFACE = "org.freedesktop.Tracker3.Miner"

    def __init__(self, dbus_connection):
        self.log = logging.getLogger(__name__)

        self.bus = dbus_connection

        self.loop = trackertestutils.mainloop.MainLoop()

        self.miner_iface = Gio.DBusProxy.new_sync(
            self.bus, Gio.DBusProxyFlags.DO_NOT_AUTO_START_AT_CONSTRUCTION, None,
            self.EXTRACTOR_BUSNAME, self.EXTRACTOR_OBJ_PATH, self.MINER_IFACE)

    def await_file_processed(self, path, status=True, timeout=None):
        expected = [FileProcessedResult(path, status)]
        return await_files_processed(self.miner_iface, expected, timeout=timeout)

    def await_files_processed(self, expected, timeout=None):
        return await_files_processed(self.miner_iface, expected, timeout=timeout)
