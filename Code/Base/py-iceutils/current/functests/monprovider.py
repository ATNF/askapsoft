#!/usr/bin/env python
# Copyright (c) 2014,2016 CSIRO
# Australia Telescope National Facility (ATNF)
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# PO Box 76, Epping NSW 1710, Australia
# atnf-enquiries@csiro.au
#
# This file is part of the ASKAP software distribution.
#
# The ASKAP software distribution is free software: you can redistribute it
# and/or modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of the License
# or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA.
#
import sys
import Ice
import threading

import askap.iceutils.monitoringprovider
from askap.iceutils import Server
from askap import logging


class Publisher(threading.Thread):
    def __init__(self, key='a', interval=0.1):
        threading.Thread.__init__(self)
        self.stop = threading.Event()
        self._key = key
        self._interval = interval

    def run(self):
        count = 0
        while not self.stop.is_set():
            askap.iceutils.monitoringprovider.MONITOR.add_points(
                {self._key: count})
            self.stop.wait(self._interval)
            count += 1


class MonProvider(Server):
    def __init__(self, comm):
        Server.__init__(self, comm, fcmkey='dummy')

    def initialize_services(self):
        self.add_service("MonProviderService",
                         askap.iceutils.monitoringprovider.MonitoringProviderImpl())


def main():
    communicator = Ice.initialize(sys.argv)
    logging.init_logging(sys.argv)
    logger = logging.getLogger(__file__)
    pub1 = Publisher('a', 0.1)
    pub2 = Publisher('b', 0.11)
    try:
        fcm = MonProvider(communicator)
        pub1.start()
        pub2.start()
        fcm.run()
    except Exception as ex:
        logger.error(str(ex))
        raise
    finally:
        pub1.stop.set()
        pub2.stop.set()
        communicator.destroy()


if __name__ == "__main__":
    main()
