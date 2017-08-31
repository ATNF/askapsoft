import Ice
import json
import threading

from . import get_service
from askap.interfaces.schedblock import ObsState, ISchedulingBlockServicePrx
from askap.interfaces.monitoring import MonitoringProviderPrx

import askap.interfaces as ICE_Typed_Value

class IngestMonitor(threading.Thread):
    def __init__(self, comm, param, cpObsServer):
        super(IngestMonitor, self).__init__()
        self.ice_monitoring_client = IceMonitoringServiceClient(comm, param)
        self.cpObsServer = cpObsServer

    def run():
        while True:
            # check if ingest is running
            sbid = self.cpObsServer.get_current_sbid()
            bool_value = ICE_Typed_Value.TypedValueType.TypedValueBool(value=(sbid>=0))

            self._monitor.update("ingest.running", bool_value, MonitorPointStatus.OK);
            if (sbid >= 0):
                # poll ingest services for monitoring point values
                self.ice_monitoring_client.process_ingest_monitoring()

            time.sleep(1)

class IceMonitoringServiceClient(object):
    def __init__(self, comm, param):
        self._monitor = askap.iceutils.monitoringprovider.MONITOR
        self.pointnames = param.get_parameter("ingest.monitoring.points")
        self.data_service = DataService(comm)
        for i in range(0, param.get_parameter("ingest.monitoring.rank.count")):
            adaptor_name = "SchedulingBlockService@DataServiceAdapter" + i
            ingest_name = "ingest" + i
            monitoring_provider = get_service(adaptor_name,
                                          MonitoringProviderPrx, comm)
            self.ingest_service_map[ingest_name] = monitoring_provider

    def process_ingest_monitoring(self):
        for ingestname, provider in self.ingest_service_map:
            pointvalues = provider.get(self.pointnames)
            self.data_service.update(sbid, pointvalues)

            for point in pointvalues:
                point.name = ingestname + "." + point.name;
                self._monitor.update(point)


class DataServiceClient(object):
    def __init__(self, comm):
        self.data_service = get_service("SchedulingBlockService@DataServiceAdapter",
                          ISchedulingBlockServicePrx, comm)

    def getObsParameters(self, sbid):
        return self.data_service.getObsParameters(sbid)

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

            obs_info["nChan"] = {"value":nChan}
            obs_info["ChanWidth"] = {"value":chanWidth, "unit":chanWidthPoint.unit}
            obs_info["StartFreq"] = {"value":startFreq, "unit":startFreqPoint.unit}
            obs_info["CentreFreq"] = {"value":centreFreq, "unit":startFreqPoint.unit}
            obs_info["BandWidth"] = {"value":bandWidth, "unit":startFreqPoint.unit}

            obsVar= {"obs.info": json.dumps(obs_info)}
            self.setObsVariables(sbid, obsVar)

            self.last_sbid = sbid
