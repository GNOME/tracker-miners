# -*- coding: utf-8 -*-

# Copyright (C) 2010, Nokia (ivan.frade@nokia.com)
# Copyright (C) 2019-2020, Sam Thursfield (sam@afuera.me.uk)
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

import locale
import os
import unittest as ut

import configuration as cfg
import fixtures


class MinerFTSStopwordsTest(fixtures.TrackerMinerFTSTest):
    """
    Search for stopwords in a file 
    """

    def __get_some_stopwords(self):
        langcode, encoding = locale.getdefaultlocale()
        if "_" in langcode:
            langcode = langcode.split("_")[0]

        stopwordsdir = os.environ['TRACKER_LANGUAGE_STOP_WORDS_DIR']
        stopwordsfile = os.path.join(stopwordsdir, "stopwords." + langcode)

        if not os.path.exists(stopwordsfile):
            self.skipTest("No stopwords for the current locale ('%s' doesn't exist)" % (stopwordsfile))
            return []

        stopwords = []
        counter = 0
        with open(stopwordsfile) as f:
            for line in f:
                if len(line) > 4:
                    stopwords.append(line[:-1])
                    counter += 1

                if counter > 5:
                    break

        return stopwords

    @ut.skip("Stopwords are disabled by default since https://gitlab.gnome.org/GNOME/tracker/merge_requests/172")
    def test_01_stopwords(self):
        stopwords = self.__get_some_stopwords()
        TEXT = " ".join(["this a completely normal text automobile"] + stopwords)

        self.set_text(TEXT)
        results = self.search_word("automobile")
        self.assertEqual(len(results), 1)
        for i in range(0, len(stopwords)):
            results = self.search_word(stopwords[i])
            self.assertEqual(len(results), 0)

    # FIXME add all the special character tests!
    # http://git.gnome.org/browse/tracker/commit/?id=81c0d3bd754a6b20ac72323481767dc5b4a6217b


if __name__ == "__main__":
    fixtures.tracker_test_main()
