#!/usr/bin/python
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
#

from common.utils import configuration as cfg
from common.utils.helpers import log
import json
import os
import re
import subprocess


def get_tracker_extract_jsonld_output(filename, mime_type=None):
    """
    Runs `tracker-extract --file` to extract metadata from a file.
    """

    tracker_extract = os.path.join (cfg.TRACKER_EXTRACT_PATH)
    command = [tracker_extract, '--verbosity=0', '--output-format=json-ld', '--file', filename]
    if mime_type is not None:
        command.extend(['--mime', mime_type])

    # We depend on parsing the output, so verbosity MUST be 0.
    env = os.environ.copy()
    env['TRACKER_VERBOSITY'] = '0'

    try:
        log ('Running: %s' % ' '.join(command))
        output = subprocess.check_output (command, env=env)
    except subprocess.CalledProcessError as e:
        raise Exception("Error %i from %s, output, see stderr for details" %
                        (e.returncode, tracker_extract))
    try:
        data = json.loads(output)
    except ValueError as e:
        raise RuntimeError("Invalid JSON returned by tracker-extract: "
                        "%s.\nOutput was: %s" % (e, output))

    return data
