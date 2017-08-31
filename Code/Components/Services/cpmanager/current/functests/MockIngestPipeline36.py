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
        point.name = "cp.ingest.obs.FieldName"
        point.timestamp = timestamp
        string_typed_value = ice_types.TypedValueString(ice_types.TypedValueType.TypeString, 'Virgo')
        point.value = string_typed_value
        point.status = PointStatus.OK
        points.append(point)

        point = MonitorPoint()
        point.name = "cp.ingest.obs.ScanId"
        point.timestamp = timestamp
        int_typed_value = ice_types.TypedValueInt(ice_types.TypedValueType.TypeInt, 73)
        point.value = int_typed_value
        point.status = PointStatus.OK
        points.append(point)

        return points


def main():
    ic = Ice.initialize(sys.argv)
    adapter = ic.createObjectAdapter("IngestPipelineMonitoringAdapter36")
    object = IngestPipelineMonitor()
    adapter.add(object, ic.stringToIdentity("MonitoringService"))
    adapter.activate()
    ic.waitForShutdown()

#    time.sleep(20)
    ic.destroy()

if __name__ == "__main__":
    main()
