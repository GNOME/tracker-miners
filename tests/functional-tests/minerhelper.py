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


import gi
gi.require_version('Tracker', '3.0')
from gi.repository import Gio, GLib, GObject
from gi.repository import Tracker

import contextlib
import logging

import trackertestutils.dbusdaemon
import trackertestutils.mainloop

import configuration


log = logging.getLogger(__name__)


class AwaitTimeoutException(RuntimeError):
    pass


def await_status(miner_iface, target_status, timeout=configuration.DEFAULT_TIMEOUT):
    log.info("Blocking until miner reports status of %s", target_status)
    loop = trackertestutils.mainloop.MainLoop()

    if miner_iface.GetStatus() == target_status:
        log.info("Status is %s now", target_status)
        return

    def signal_cb(proxy, sender_name, signal_name, parameters):
        if signal_name == 'Progress':
            status, progress, remaining_time = parameters.unpack()
            log.debug("Got status: %s", status)
            if status == target_status:
                loop.quit()

    def timeout_cb():
        log.info("Timeout fired after %s seconds", timeout)
        raise AwaitTimeoutException(
            f"Timeout awaiting miner status of '{target_status}'")

    signal_id = miner_iface.connect('g-signal', signal_cb)
    timeout_id = GLib.timeout_add_seconds(timeout, timeout_cb)

    loop.run_checked()

    GObject.signal_handler_disconnect(miner_iface, signal_id)
    GLib.source_remove(timeout_id)


class await_signal():
    """Context manager to await a specific D-Bus signal.

    Useful to wait for org.freedesktop.Tracker3.Miner signals like
    Paused and Resumed.

    """
    def __init__(self, miner_iface, signal_name,
                 timeout=configuration.DEFAULT_TIMEOUT):
        self.miner_iface = miner_iface
        self.signal_name = signal_name
        self.timeout = timeout

        self.loop = trackertestutils.mainloop.MainLoop()

    def __enter__(self):
        log.info("Awaiting signal %s", self.signal_name)

        def signal_cb(proxy, sender_name, signal_name, parameters):
            if signal_name == self.signal_name:
                log.debug("Received signal %s", signal_name)
                self.loop.quit()

        def timeout_cb():
            log.info("Timeout fired after %s seconds", self.timeout)
            raise AwaitTimeoutException(
                f"Timeout awaiting signal '{self.signal_name}'")

        self.signal_id = self.miner_iface.connect('g-signal', signal_cb)
        self.timeout_id = GLib.timeout_add_seconds(self.timeout, timeout_cb)

    def __exit__(self, etype, evalue, etraceback):
        if etype is not None:
            return False

        self.loop.run_checked()

        GLib.source_remove(self.timeout_id)
        GObject.signal_handler_disconnect(self.miner_iface, self.signal_id)

        return True


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

        self.miner_fs = Gio.DBusProxy.new_sync(
            self.bus, Gio.DBusProxyFlags.DO_NOT_AUTO_START_AT_CONSTRUCTION, None,
            self.MINERFS_BUSNAME, self.MINERFS_OBJ_PATH, self.MINER_IFACE)

        self.index = Gio.DBusProxy.new_sync(
            self.bus, Gio.DBusProxyFlags.DO_NOT_AUTO_START_AT_CONSTRUCTION, None,
            self.MINERFS_BUSNAME, self.MINERFS_INDEX_OBJ_PATH, self.MINER_INDEX_IFACE)

    def start(self):
        self.miner_fs.Start()
        trackertestutils.dbusdaemon.await_bus_name(self.bus, self.MINERFS_BUSNAME)

    def stop(self):
        self.miner_fs.Stop()

    def get_status(self):
        return self.miner_fs.GetStatus()

    def get_sparql_connection(self):
        return Tracker.SparqlConnection.bus_new(
            'org.freedesktop.Tracker3.Miner.Files', None, self.bus)

    def index_file(self, uri):
        return self.index.IndexFile('(s)', uri)
