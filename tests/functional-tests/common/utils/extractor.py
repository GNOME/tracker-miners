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

    log ('Running: %s' % ' '.join(command))
    p = subprocess.Popen (command, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = p.communicate()

    if p.returncode != 0:
        raise RuntimeError(
            "tracker-extract returned non-zero exit code: %s\n"
            "Error output:\n%s\n" % (p.returncode, stderr.decode('unicode-escape').strip()))

    if len(stderr) > 0:
        log ("Error output from tracker-extract:\n%s" % stderr.decode('unicode-escape').strip())

    try:
        output = stdout.decode('utf-8')
        data = json.loads(output)
    except ValueError as e:
        raise RuntimeError("tracker-extract did not return valid JSON data: %s\n"
                           "Output was: %s" % (e, output))

    return data
