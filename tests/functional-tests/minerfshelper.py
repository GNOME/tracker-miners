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


import logging

from gi.repository import Gio
from gi.repository import GLib

import trackertestutils.mainloop


log = logging.getLogger(__name__)

REASONABLE_TIMEOUT = 5

class WakeupCycleTimeoutException(RuntimeError):
    pass


class MinerFsHelper ():

    MINERFS_BUSNAME = "org.freedesktop.Tracker1.Miner.Files"
    MINERFS_OBJ_PATH = "/org/freedesktop/Tracker1/Miner/Files"
    MINER_IFACE = "org.freedesktop.Tracker1.Miner"
    MINERFS_INDEX_OBJ_PATH = "/org/freedesktop/Tracker1/Miner/Files/Index"
    MINER_INDEX_IFACE = "org.freedesktop.Tracker1.Miner.Files.Index"

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

    def stop(self):
        self.miner_fs.Stop()

    def start_watching_progress(self):
        self._previous_status = None
        self._target_wakeup_count = None
        self._wakeup_count = 0

        def signal_handler(proxy, sender_name, signal_name, parameters):
            if signal_name == 'Progress':
                self._progress_cb(*parameters.unpack())

        self._progress_handler_id = self.miner_fs.connect('g-signal', signal_handler)

    def stop_watching_progress(self):
        if self._progress_handler_id != 0:
            self.miner_fs.disconnect(self._progress_handler_id)

    def _progress_cb(self, status, progress, remaining_time):
        if self._previous_status is None:
            self._previous_status = status
        if self._previous_status != 'Idle' and status == 'Idle':
            self._wakeup_count += 1

        if self._target_wakeup_count is not None and self._wakeup_count >= self._target_wakeup_count:
            self.loop.quit()

    def wakeup_count(self):
        """Return the number of wakeup-to-idle cycles the miner-fs completed."""
        return self._wakeup_count

    def await_wakeup_count(self, target_wakeup_count, timeout=REASONABLE_TIMEOUT):
        """Block until the miner has completed N wakeup-and-idle cycles.

        This function is for use by miner-fs tests that should trigger an
        operation in the miner, but which do not cause a new resource to be
        inserted. These tests can instead wait for the status to change from
        Idle to Processing... and then back to Idle.

        The miner may change its status any number of times, but you can use
        this function reliably as follows:

            wakeup_count = miner_fs.wakeup_count()
            # Trigger a miner-fs operation somehow ...
            miner_fs.await_wakeup_count(wakeup_count + 1)
            # The miner has probably finished processing the operation now.

        If the timeout is reached before enough wakeup cycles complete, an
        exception will be raised.

        """

        assert self._target_wakeup_count is None

        if self._wakeup_count >= target_wakeup_count:
            log.debug("miner-fs wakeup count is at %s (target is %s). No need to wait", self._wakeup_count, target_wakeup_count)
        else:
            def _timeout_cb():
                raise WakeupCycleTimeoutException()
            timeout_id = GLib.timeout_add_seconds(timeout, _timeout_cb)

            log.debug("Waiting for miner-fs wakeup count of %s (currently %s)", target_wakeup_count, self._wakeup_count)
            self._target_wakeup_count = target_wakeup_count
            self.loop.run_checked()

            self._target_wakeup_count = None
            GLib.source_remove(timeout_id)

    def index_file(self, uri):
        return self.index.IndexFile('(s)', uri)
