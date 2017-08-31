import Ice
import json
import threading
import time

import askap.iceutils.monitoringprovider

from . import get_service
from . import logger

from askap.slice import SchedulingBlockService

# noinspection PyUnresolvedReferences
from askap.interfaces.schedblock import ObsState, ISchedulingBlockServicePrx
# noinspection PyUnresolvedReferences
from askap.interfaces.monitoring import MonitoringProviderPrx

from askap.slice import TypedValues

# noinspection PyUnresolvedReferences
from askap.interfaces import TypedValue, TypedValueType


class IngestMonitor(threading.Thread):
    def __init__(self, comm, param, cpObsServer):
        super(IngestMonitor, self).__init__()
        self._monitor = askap.iceutils.monitoringprovider.MONITOR
        self.daemon = True
        self.ice_monitoring_client = IceMonitoringServiceClient(comm, param)
        self.cpObsServer = cpObsServer

    def run(self):
        while True:
            try:
                # check if ingest is running
                sbid = self.cpObsServer.get_current_sbid()
                self._monitor.add('ingest.running', (sbid>-1));
                if (sbid >= 0):
                    # poll ingest services for monitoring point values
                    self.ice_monitoring_client.process_ingest_monitoring(sbid)
                else:
                    # make sure all ingest monitoring points are removed
                    self.ice_monitoring_client.remove_monitoring_points()

                time.sleep(1)
            except Exception as ex:
                # logging
                logger.error('Error while trying to run ingest ', str(ex))


class IceMonitoringServiceClient(object):
    def __init__(self, comm, param):
        self._monitor = askap.iceutils.monitoringprovider.MONITOR
        self._comm = comm
        self.pointnames = param.get("cp.ingest.monitoring.points")
        self.data_service = DataServiceClient(comm)
        self.rank_count = param.get("cp.ingest.monitoring.rank.count")
        self.ingest_service_map = {}
        for i in range(0, self.rank_count):
            self.ingest_service_map[i] = None

    def remove_monitoring_points(self, ingest=''):
        if not ingest: # if ingest is empty, delete all ingest data
            for i in range(0, self.rank_count):
                for name in self.pointnames:
                    pointname = 'ingest' + str(i) + "." + name
                    self._monitor.remove(pointname)
        else:
            for name in self.pointnames:
                pointname = ingest + "." + name
                self._monitor.remove(pointname)

    def process_ingest_monitoring(self, sbid):
        for i, provider in self.ingest_service_map.iteritems():
            if provider is None:
                try:
                    adaptor_name = "MonitoringService@IngestPipelineMonitoringAdapter" + str(i)
                    provider = get_service(adaptor_name,
                                                      MonitoringProviderPrx, self._comm)
                    self.ingest_service_map[i] = provider
                except:
                    # if ingest is not running move on
                    continue

            try:
                points = []
                pointvalues = provider.get(self.pointnames)
                self.data_service.update(sbid, pointvalues)
            except:
                # if ingest is not running need to remove the monitoring points
                self.remove_monitoring_points(ingest='ingest' + str(i))
                continue

            for point in pointvalues:
                # convert points to map
                p_map = {
                    'name': 'ingest' + str(i) + "." + point.name,
                    'timestamp': point.timestamp,
                    'value': point.value.value,
                    'status': point.status.name,
                    'unit': point.unit
                }
                points.append(p_map)
            self._monitor.add_full_points(points)


class DataServiceClient(object):
    def __init__(self, comm):
        self.last_sbid = -1
        self.data_service = get_service("SchedulingBlockService@DataServiceAdapter",
                          ISchedulingBlockServicePrx, comm)

    def setObsVariables(self, sbid, obsvars):
        self.data_service.setObsVariables(sbid, obsvars)

    def update(self, sbid, pointvalues):
        if self.last_sbid==sbid:
            return

        for point in pointvalues:
            if point.name == "cp.ingest.obs.StartFreq":
                startFreqPoint = point
            elif point.name == "cp.ingest.obs.nChan":
                nChanPoint = point
            elif point.name == "cp.ingest.obs.ChanWidth":
                chanWidthPoint = point


        if startFreqPoint and nChanPoint and chanWidthPoint:
            startFreq = startFreqPoint.value.value
            chanWidth = chanWidthPoint.value.value
            nChan = nChanPoint.value.value

            bandWidth = 0
            if chanWidthPoint.unit.lower == "khz":
                bandWidth = chanWidth * nChan / 1000
            else: # assume mhz
                bandWidth = chanWidth * nChan

            centreFreq = startFreq + bandWidth / 2

            obs_info = {}
            obs_info["nChan"] = {"value":nChan}
            obs_info["ChanWidth"] = {"value":chanWidth, "unit":chanWidthPoint.unit}
            obs_info["StartFreq"] = {"value":startFreq, "unit":startFreqPoint.unit}
            obs_info["CentreFreq"] = {"value":centreFreq, "unit":startFreqPoint.unit}
            obs_info["BandWidth"] = {"value":bandWidth, "unit":startFreqPoint.unit}

            obsVar= {"obs.info": json.dumps(obs_info)}
            self.setObsVariables(sbid, obsVar)

            self.last_sbid = sbid
