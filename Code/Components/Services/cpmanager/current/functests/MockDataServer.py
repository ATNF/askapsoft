#!/usr/bin/env python

# good simple example for Python ICE https://forums.zeroc.com/discussion/6295/simple-ice-registry-example

"""

        // Server configuration
        MyAdapter.Endpoints=tcp
        MyAdapter.AdapterId=Adapter1
        Ice.Default.Locator=IceGrid/Locator:tcp -h hostname -p 4061

        // Server code
        adapter = ic.createObjectAdapter("MyAdapter")
        adapter.add(obj1, ic.stringToIdentity("obj1"))
        adapter.activate()


        // Client configuration
        Ice.Default.Locator=IceGrid/Locator:tcp -h hostname -p 4061

        // Client code
        proxy = ic.stringToProxy("obj1 @ Adapter1")
"""

import sys, traceback, Ice, time

from askap.slice import SchedulingBlockService

# noinspection PyUnresolvedReferences
from askap.interfaces.schedblock import ISchedulingBlockService

import askap.interfaces as ice_types


class MockDataService(ISchedulingBlockService):

    def __init__(self):
        self._obsvars = {}
        self._sbid = -1

    def getObsVariables(self, sbid, key, current=None):
        return self._obsvars

    def setObsVariables(self, sbid, obsvars, current=None):
        self._obsvars = obsvars
        self._sbid = sbid

def main():
    ic = Ice.initialize(sys.argv)
    adapter = ic.createObjectAdapter("DataServiceAdapter")
    object = MockDataService()
    adapter.add(object, ic.stringToIdentity("SchedulingBlockService"))
    adapter.activate()
    ic.waitForShutdown()
#    time.sleep(20)
    ic.destroy()

if __name__ == "__main__":
    main()
