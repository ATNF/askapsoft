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

from askap.slice import MonitoringProvider
from askap.time import bat_now
from askap.slice import TypedValues

# noinspection PyUnresolvedReferences
from askap.interfaces.monitoring import MonitoringProvider, MonitorPoint, PointStatus

import askap.interfaces as ice_types


class IngestPipelineMonitor(MonitoringProvider):
    def __init__(self):
        self.name = 'Monitoring'

    def get(self, pointsnames, current=None):

        points = []
        timestamp = bat_now()

        point = MonitorPoint()
        point.name = "ingest0.cp.ingest.obs.StartFreq"
        point.timestamp = timestamp
        double_typed_value = ice_types.TypedValueFloat(ice_types.TypedValueType.TypeFloat, 1376.5)
        point.value = double_typed_value
        point.status = PointStatus.OK
        point.unit = 'MHz'
        points.append(point)

        point = MonitorPoint()
        point.name = "ingest0.cp.ingest.obs.nChan"
        point.timestamp = timestamp
        double_typed_value = ice_types.TypedValueInt(ice_types.TypedValueType.TypeInt, 864)
        point.value = double_typed_value
        point.status = PointStatus.OK
        points.append(point)

        point = MonitorPoint()
        point.name = "ingest0.cp.ingest.obs.ChanWidth"
        point.timestamp = timestamp
        double_typed_value = ice_types.TypedValueDouble(ice_types.TypedValueType.TypeDouble, 18.518518)
        point.value = double_typed_value
        point.status = PointStatus.OK
        point.unit = 'MHz'
        points.append(point)

        return points


def main():
    ic = Ice.initialize(sys.argv)
    adapter = ic.createObjectAdapter("IngestPipelineMonitoringAdapter0")
    object = IngestPipelineMonitor()
    adapter.add(object, ic.stringToIdentity("MonitoringService"))
    adapter.activate()
    ic.waitForShutdown()

#    time.sleep(5)

    ic.destroy()

if __name__ == "__main__":
    main()
