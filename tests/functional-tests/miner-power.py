# Copyright (C) 2020, Sam Thursfield (sam@afuera.me.uk)
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the
# Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA  02110-1301, USA.

"""
Test that the miner responds to changes in power / battery status.
"""

import os
import sys

import configuration
import devices
import fixtures
import minerhelper

import trackertestutils


testbed = None

class MinerPowerTest(fixtures.TrackerMinerTest):
    def setUp(self):
        # We don't use setUp() from the base class because we need to start
        # upowerd before the miner-fs.

        extra_env = configuration.test_environment(self.workdir)
        extra_env['LANG'] = 'en_GB.utf8'

        # This sets the UMOCKDEV_DIR variable in the process environment.
        #testbed = self.sandbox.get_umockdev_testbed()
        self.battery = devices.MockBattery(testbed)
        self.battery.set_battery_power_normal_charge()

        self.sandbox = trackertestutils.helpers.TrackerDBusSandbox(
            session_bus_config_file=configuration.TEST_SESSION_BUS_CONFIG_FILE,
            system_bus_config_file=configuration.TEST_SYSTEM_BUS_CONFIG_FILE,
            extra_env=extra_env)

        try:
            self.sandbox.start()
            self.sandbox.system_bus.activate_service('org.freedesktop.UPower', '/org/freedesktop/UPower')
            self.sandbox.set_config(self.config())

            self.miner_fs = minerhelper.MinerFsHelper(self.sandbox.get_session_bus_connection())
            self.miner_fs.start()
        except Exception:
            self.sandbox.stop()
            raise

    def test_miner_fs_pause_on_low_battery(self):
        """The miner-fs should stop indexing if there's a low battery warning."""
        minerhelper.await_status(self.miner_fs.miner_fs, "Idle")

        with minerhelper.await_signal(self.miner_fs.miner_fs, "Paused"):
            self.battery.set_battery_power_low_charge()
        self.assertEqual(self.miner_fs.get_status(), "Paused")

        with minerhelper.await_signal(self.miner_fs.miner_fs, "Resumed"):
            self.battery.set_ac_power()
        self.assertEqual(self.miner_fs.get_status(), "Idle")


if __name__ == "__main__":
    if not devices.HAVE_UMOCKDEV:
        # Return 'skipped' error code so `meson test` reports the test
        # correctly.
        sys.exit(77)

    if not devices.libumockdev_loaded():
        raise RuntimeError("This test must be run inside umockdev-wrapper.")

    # This sets a process-wide environment variable UMOCKDEV_DIR.
    testbed = devices.create_testbed()

    fixtures.tracker_test_main()
