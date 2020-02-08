# Copyright (C) 2018-2019, Sam Thursfield <sam@afuera.me.uk>
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
import math
import shlex

import gi
gi.require_version('Gst', '1.0')
from gi.repository import Gst


log = logging.getLogger(__name__)


def create_test_flac(path, duration, timeout=10, tags=None):
    """
    Create a .flac audio file for testing purposes.

    FLAC audio doesn't compress test data particularly efficiently, so
    committing an audio file more than a few seconds long to Git is not
    practical. This function creates a .flac file containing a test tone.
    The 'duration' parameter sets the length in seconds of the time.

    The function is guaranteed to return or raise an exception within the
    number of seconds given in the 'timeout' parameter.
    """

    Gst.init([])

    num_buffers = math.ceil(duration * 44100 / 1024.0)

    pipeline_src = ' ! '.join([
        'audiotestsrc num-buffers=%s samplesperbuffer=1024' % num_buffers,
        'capsfilter caps="audio/x-raw,rate=44100"',
        'flacenc name=flacenc',
        'filesink location="%s"' % str(path),
    ])

    log.debug("Running pipeline: %s", pipeline_src)
    pipeline = Gst.parse_launch(pipeline_src)

    if tags:
        flacenc = pipeline.get_child_by_name('flacenc')
        flacenc.merge_tags(tags, Gst.TagMergeMode.REPLACE_ALL)

    pipeline.set_state(Gst.State.PLAYING)

    msg = pipeline.get_bus().poll(Gst.MessageType.ERROR | Gst.MessageType.EOS,
                                timeout * Gst.SECOND)
    if msg and msg.type == Gst.MessageType.EOS:
        pass
    elif msg and msg.type == Gst.MessageType.ERROR:
        raise RuntimeError(msg.parse_error())
    elif msg:
        raise RuntimeError("Got unexpected GStreamer message %s" % msg.type)
    else:
        raise RuntimeError("Timeout generating test audio file after %i seconds" % timeout)

    pipeline.set_state(Gst.State.NULL)
