# Copyright (C) 2020, Sam Thursfield <sam@afuera.me.uk>
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

"""
Mock device helpers for testing.
"""


import logging
import os

try:
    import gi
    gi.require_version('UMockdev', '1.0')
    from gi.repository import UMockdev
    HAVE_UMOCKDEV = True
except ImportError as e:
    HAVE_UMOCKDEV = False
    print("Did not find UMockdev library: %s." % e)

log = logging.getLogger(__name__)


class UMockdevNotFound(Exception):
    pass


class MockBattery():
    def __init__(self, testbed):
        self.testbed = testbed
        # Mostly copied from
        # https://github.com/martinpitt/umockdev/blob/master/docs/examples/battery.py
        self.device = testbed.add_device('power_supply', 'fakeBAT0', None,
                                         ['type', 'Battery',
                                          'present', '1',
                                          'status', 'Discharging',
                                          'energy_full', '60000000',
                                          'energy_full_design', '80000000',
                                          'energy_now', '48000000',
                                          'voltage_now', '12000000'],
                                          ['POWER_SUPPLY_ONLINE', '1'])

    def set_battery_power_normal_charge(self):
        self.testbed.set_attribute(self.device, 'status', 'Discharging')
        self.testbed.set_attribute(self.device, 'energy_now', '48000000')
        self.testbed.uevent(self.device, 'change')

    def set_battery_power_low_charge(self):
        self.testbed.set_attribute(self.device, 'status', 'Discharging')
        self.testbed.set_attribute(self.device, 'energy_now', '1500000')
        self.testbed.uevent(self.device, 'change')

    def set_ac_power(self):
        self.testbed.set_attribute(self.device, 'status', 'Charging')
        self.testbed.uevent(self.device, 'change')


def libumockdev_loaded():
    """Returns True if the process was run inside `umockdev-wrapper`."""
    return 'libumockdev-preload' in os.environ['LD_PRELOAD']


def create_testbed():
    if not HAVE_UMOCKDEV:
        raise UMockdevNotFound()
    return UMockdev.Testbed.new()


def upowerd_path():
    with open('/usr/share/dbus-1/system-services/org.freedesktop.UPower.service') as f:
        for line in f:
            if line.startswith('Exec='):
                upowerd_path = line.split('=', 1)[1].strip()
                break
        else:
            sys.stderr.write('Cannot determine upowerd path\n')
            sys.exit(1)
    return upowerd_path
