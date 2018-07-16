#!/usr/bin/python
#-*- coding: utf-8 -*-

# Copyright (C) 2010, Nokia (ivan.frade@nokia.com)
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

#
# TODO:
#     These tests are for files... we need to write them for folders!
#
"""
Monitor a directory, copy/move/remove/update text files and check that
the text contents are updated accordingly in the indexes.
"""
import os
import shutil
import locale
import time

import unittest2 as ut
from common.utils.helpers import log
from common.utils.minertest import CommonTrackerMinerFTSTest, MINER_TMP_DIR, uri, path, DEFAULT_TEXT
from common.utils import configuration as cfg

class MinerFTSStopwordsTest (CommonTrackerMinerFTSTest):
    """
    Search for stopwords in a file 
    """

    def __get_some_stopwords (self):

        langcode, encoding = locale.getdefaultlocale ()
        if "_" in langcode:
            langcode = langcode.split ("_")[0]

        stopwordsfile = os.path.join (cfg.DATADIR, "tracker", "stop-words", "stopwords." + langcode)

        if not os.path.exists (stopwordsfile):
            self.skipTest ("No stopwords for the current locale ('%s' doesn't exist)" % (stopwordsfile))
            return []
        
        stopwords = []
        counter = 0
        for line in open (stopwordsfile, "r"):
            if len (line) > 4:
                stopwords.append (line[:-1])
                counter += 1

            if counter > 5:
                break
            
        return stopwords
    
    def test_01_stopwords (self):
        stopwords = self.__get_some_stopwords ()
        TEXT = " ".join (["this a completely normal text automobile"] + stopwords)
        
        self.set_text (TEXT)
        results = self.search_word ("automobile")
        self.assertEquals (len (results), 1)
        log ("Stopwords: %s" % stopwords)
        for i in range (0, len (stopwords)):
            results = self.search_word (stopwords[i])
            self.assertEquals (len (results), 0)

    ## FIXME add all the special character tests!
    ##  http://git.gnome.org/browse/tracker/commit/?id=81c0d3bd754a6b20ac72323481767dc5b4a6217b
    

if __name__ == "__main__":
    ut.main ()
