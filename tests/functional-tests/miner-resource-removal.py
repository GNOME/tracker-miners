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

"""
Test that resource removal does not leave debris or clobber too much,
especially in the case where nie:InformationElement != nie:DataObject
"""

import os
import pathlib
import unittest as ut

# We must import this to set up logging.
import configuration
from fixtures import TrackerMinerTest

TRACKER_OWN_GRAPH_URN = "urn:uuid:472ed0cc-40ff-4e37-9c0c-062d78656540"

class MinerResourceRemovalTest(TrackerMinerTest):

    def prepare_directories(self):
        # Override content from the base class
        pass

    def create_test_content(self, file_urn, title):
        sparql = "WITH %s INSERT { \
                    _:ie a nmm:MusicPiece ; \
                         nie:title \"%s\" ; \
                         nie:isStoredAs <%s> \
                  } " % (TRACKER_OWN_GRAPH_URN, title, file_urn)

        with self.tracker.await_insert(f'a nmm:MusicPiece; nie:title "{title}"') as resource:
            self.tracker.update(sparql)
        return resource

    def create_test_file(self, file_name):
        path = pathlib.Path(self.path(file_name))
        text = "Test"

        with self.await_document_inserted(file_name, content=text) as resource:
            path.write_text(text)
        return resource

    def test_01_file_deletion(self):
        """
        Ensure every logical resource (nie:InformationElement) contained with
        in a file is deleted when the file is deleted.
        """

        file_1 = self.create_test_file("test-monitored/test_1.txt")
        file_2 = self.create_test_file("test-monitored/test_2.txt")
        ie_1 = self.create_test_content(file_1.urn, "Test resource 1")
        ie_2 = self.create_test_content(file_2.urn, "Test resource 2")

        with self.tracker.await_delete(file_1.id):
            os.unlink(self.path("test-monitored/test_1.txt"))

        self.assertResourceMissing(file_1.urn)
        # Ensure the logical resource is deleted when the relevant file is
        # removed.
        self.assertResourceMissing(ie_1.urn)

        self.assertResourceExists(file_2.urn)
        self.assertResourceExists(ie_2.urn)

    # def test_02_removable_device_data (self):
    #    """
    #    Tracker does periodic cleanups of data on removable volumes that haven't
    #    been seen since 'removable-days-threshold', and will also remove all data
    #    from removable volumes if 'index-removable-devices' is disabled.
    #
    #    FIXME: not yet possible to test this - we need some way of mounting
    #    a fake removable volume: https://bugzilla.gnome.org/show_bug.cgi?id=659739
    #    """

        #dconf = DConfClient ()
        #dconf.write (cfg.DCONF_MINER_SCHEMA, 'index-removable-devices', 'true')

        #self.mount_test_removable_volume ()

        #self.add_test_resource ("urn:test:1", test_volume_urn)
        #self.add_test_resource ("urn:test:2", None)

        # Trigger removal of all resources from removable devices
        #dconf.write (cfg.DCONF_MINER_SCHEMA, 'index-removable-devices', 'false')

        # Check that only the data on the removable volume was deleted
        #self.await_updates (2)


if __name__ == "__main__":
    ut.main(failfast=True, verbosity=2)
