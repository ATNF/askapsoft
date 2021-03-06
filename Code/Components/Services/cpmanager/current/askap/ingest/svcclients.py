import Ice
import json
import threading
import time, math
import subprocess

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
        self.params = param
        self.sbid = -1

    def run(self):
        while True:
            try:
                # check if ingest is running
                sbid = self.cpObsServer.get_current_sbid()
                self._monitor.add('ingest.running', (sbid>-1))
                if (sbid >= 0):
                    # poll ingest services for monitoring point values
                    self.sbid = sbid
                    self.ice_monitoring_client.process_ingest_monitoring(sbid)
                else:
                    # start post ingest script if necessary
                    if (self.sbid > 0):
                        self.start_postingest(self.sbid)
                        self.sbid = -1

                    # make sure all ingest monitoring points are removed
                    self.ice_monitoring_client.remove_monitoring_points()

                time.sleep(1)
            except Exception as ex:
                # logging
                logger.error('Error while trying to poll ingest ', str(ex))

    def start_postingest(self, sbid):
        logger.debug("start postingest for " + str(sbid))
        return
#        command = self.params.get("cp.ingest.postingest.script")
#        cmd = [command, sbid]
#        logger.info("start post ingest script: " + str(cmd))
#        subprocess.Popen(cmd, shell=False)

class IceMonitoringServiceClient(object):
    def __init__(self, comm, param):
        self._monitor = askap.iceutils.monitoringprovider.MONITOR
        self._comm = comm
        self.pointnames = param.get("cp.ingest.monitoring.points")
        if not self.pointnames:
            self.pointnames=['cp.ingest.PacketsLostCount', 'cp.ingest.PacketsLostPercent', \
    'cp.ingest.VisFlagCount', 'cp.ingest.VisFlagPercent', 'cp.ingest.MisalignedStreamsCount', \
    'cp.ingest.MisalignedStreamsPercent', 'cp.ingest.MSWritingDuration', 'cp.ingest.VisCornerTurnDuration', \
    'cp.ingest.ProcessingDuration', 'cp.ingest.SourceTaskDuration', 'cp.ingest.obs.ScanId', \
    'cp.ingest.obs.FieldName', 'cp.ingest.obs.dir1', 'cp.ingest.obs.dir2', 'cp.ingest.obs.CoordSys', \
    'cp.ingest.obs.Interval', 'cp.ingest.obs.StartFreq', 'cp.ingest.obs.nChan', 'cp.ingest.obs.ChanWidth', \
    'cp.ingest.obs.SourceStartFreq', 'cp.ingest.obs.SourceNChan', 'cp.ingest.obs.SourceChanWidth', \
    'cp.ingest.dUTC', 'cp.ingest.dUT1', 'cp.ingest.MeasuresTableMJD', 'cp.ingest.MeasuresTableVersion', \
    'cp.ingest.SoftwareVersion', 'cp.ingest.obs.DataRate', 'cp.ingest.PacketsBuffered', 'cp.ingest.BufferUsagePercent']

        self.data_service = DataServiceClient(comm)

        self.host_map = param.get("cp.ingest.hosts_map")
        self.rank_count = 0

        for host in self.host_map.split(','):
            counts = host.split(':')
            self.rank_count += int(counts[1])

        logger.debug("total rank_count " + str(self.rank_count))

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
                    logger.debug("Connected to MonitoringService@IngestPipelineMonitoringAdapter" + str(i))

                except Exception as e:
                    # if ingest is not running move on
                    continue

            points = []
            pointnames = []
            for name in self.pointnames:
                pointnames.append("ingest" + str(i) + "." + name)

            try:
                pointvalues = provider.get(pointnames)
            except Exception as e:
                # if ingest is not running need to remove the monitoring points
                self.remove_monitoring_points(ingest='ingest' + str(i))
                continue

            for point in pointvalues:
                # convert points to map
                p_map = {
                    'name': point.name,
                    'timestamp': point.timestamp,
                    'value': point.value.value,
                    'status': point.status.name,
                    'unit': point.unit
                }
                points.append(p_map)
            self._monitor.add_full_points(points)

            try:
                self.data_service.update(sbid, pointvalues)
            except Exception as e:
                # if ingest is not running need to remove the monitoring points
                logger.error('Error while trying to update data service ', str(e))
                continue

class ObsVariable(object):
    def __init__(self):
        self.startFreq = 0
        self.endFreq = 0
        self.centreFreq = 0
        self.bandWidth = 0
        self.nChan = 0


class DataServiceClient(object):
    def __init__(self, comm):
        self.last_sbid = -1
        self.obs_info = {}
        self.data_service = get_service("SchedulingBlockService@DataServiceAdapter",
                          ISchedulingBlockServicePrx, comm)

    def setObsVariables(self, sbid, obsvars):
        logger.debug("set obs var for " + str(sbid) + ": " + str(obsvars))
        self.data_service.setObsVariables(sbid, obsvars)

    def update(self, sbid, pointvalues):
        if self.last_sbid!=sbid:
            self.obs_info = {}

        startFreqPoint = nChanPoint = chanWidthPoint = {}
        for point in pointvalues:
            if point.name.endswith("cp.ingest.obs.StartFreq"):
                startFreqPoint = point
            elif point.name.endswith("cp.ingest.obs.nChan"):
                nChanPoint = point
            elif point.name.endswith("cp.ingest.obs.ChanWidth"):
                chanWidthPoint = point


        new_obs_info = ObsVariable()
        if startFreqPoint and nChanPoint and chanWidthPoint:
            new_obs_info.startFreq = startFreqPoint.value.value
            new_obs_info.chanWidth = chanWidthPoint.value.value
            new_obs_info.nChan = nChanPoint.value.value

            new_obs_info.bandWidth = 0
            if chanWidthPoint.unit.lower == "khz":
                new_obs_info.bandWidth = new_obs_info.chanWidth * new_obs_info.nChan / 1000
            else: # assume mhz
                new_obs_info.bandWidth = new_obs_info.chanWidth * new_obs_info.nChan

            new_obs_info.centreFreq = new_obs_info.startFreq + new_obs_info.bandWidth / 2
            new_obs_info.endFreq = new_obs_info.startFreq + new_obs_info.bandWidth

            # amend freq is necessary. No need to worry about gaps as they will get filled
            changed = False
            if self.obs_info:
                if new_obs_info.startFreq >= self.obs_info.endFreq:
                    self.obs_info.endFreq = new_obs_info.endFreq
                    self.obs_info.centreFreq = (self.obs_info.startFreq + self.obs_info.endFreq)/2
                    self.obs_info.bandWidth = self.obs_info.endFreq - self.obs_info.startFreq
                    self.obs_info.nChan = math.floor(self.obs_info.bandWidth/new_obs_info.chanWidth)
                    changed = True
                elif new_obs_info.endFreq <= self.obs_info.startFreq:
                    self.obs_info.startFreq = new_obs_info.startFreq
                    self.obs_info.centreFreq = (self.obs_info.startFreq + self.obs_info.endFreq)/2
                    self.obs_info.bandWidth = self.obs_info.endFreq - self.obs_info.startFreq
                    self.obs_info.nChan = math.ceil(self.obs_info.bandWidth/new_obs_info.chanWidth)
                    changed = True
            else:
                self.obs_info = ObsVariable()
                self.obs_info.startFreq = new_obs_info.startFreq
                self.obs_info.chanWidth = new_obs_info.chanWidth
                self.obs_info.nChan = new_obs_info.nChan
                self.obs_info.bandWidth = new_obs_info.bandWidth
                self.obs_info.centreFreq = new_obs_info.centreFreq
                self.obs_info.endFreq = new_obs_info.endFreq
                changed = True

            # if anything changed then update obsvar
            if changed:
                obs_var = {}
                obs_var["nChan"] = {"value":self.obs_info.nChan}
                obs_var["ChanWidth"] = {"value":chanWidthPoint.value.value, "unit":chanWidthPoint.unit}
                obs_var["StartFreq"] = {"value":self.obs_info.startFreq, "unit":startFreqPoint.unit}
                obs_var["CentreFreq"] = {"value":self.obs_info.centreFreq, "unit":startFreqPoint.unit}
                obs_var["BandWidth"] = {"value":self.obs_info.bandWidth, "unit":startFreqPoint.unit}

                obs_var_json= {"obs.info": json.dumps(obs_var)}
                self.setObsVariables(sbid, obs_var_json)

                self.last_sbid = sbid
