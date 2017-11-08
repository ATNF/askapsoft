#!/usr/bin/env python

from nose.tools import assert_equals, assert_almost_equals
import sys, traceback, os, time, json
from askap.iceutils import IceSession

from askap.ingest import get_service

# noinspection PyUnresolvedReferences
from askap.interfaces.schedblock import ISchedulingBlockServicePrx

# noinspection PyUnresolvedReferences
from askap.interfaces.cp import ICPObsServicePrx

# noinspection PyUnresolvedReferences
from askap.interfaces.monitoring import MonitoringProviderPrx


class TestCPIngest(object):
    def __init__(self):
        self.logger = None
        self.igsession = None
        self.cpservice = None

    def setup(self):
        os.chdir('functests')
        self.igsession = None
        self.infile = "test/cpingest.in"
        if os.path.exists(self.infile):
            os.remove(self.infile)
        os.environ["ICE_CONFIG"] = "ice.cfg"
        os.environ["PYTHON_LOG_FILE"] = "cpmanager.log"
        self.igsession = IceSession('applications.txt', cleanup=True)
        try:
            self.igsession.start()
            time.sleep(5)

            self.dataservice = get_service("SchedulingBlockService@DataServiceAdapter",
                          ISchedulingBlockServicePrx,
                          self.igsession.communicator)

            self.cpservice = get_service(
                "CentralProcessorService@IngestManagerAdapter",
                ICPObsServicePrx,
                self.igsession.communicator)

            self.monitoringservice = get_service("MonitoringService@IngestManagerMonitoringAdapter",
                          MonitoringProviderPrx,
                          self.igsession.communicator)


        except Exception as ex:
            os.chdir('..')
            self.igsession.terminate()
            raise

    def teardown(self):
        self.igsession.terminate()
        os.chdir('..')
        self.igsession = None
        self.cpservice = None


    def test_obs(self):
        self.cpservice.startObs(5)
        is_complete = self.cpservice.waitObs(2000)
        assert_equals(is_complete, False)
        self.cpservice.abortObs()
        is_complete = self.cpservice.waitObs(2000)
        assert_equals(is_complete, True)
        is_complete = self.cpservice.waitObs(200)
        assert_equals(is_complete, True)

    def test_dataservice(self):
        self.cpservice.startObs(5)
        time.sleep(5)
        # sch block should have obs var updated
        obs_vars = self.dataservice.getObsVariables(5, '')
        obs_info = obs_vars.get('obs.info')

        # obs_info should be a json string, convert it back to
        # json object and check values
        obs_info_obj = json.loads(obs_info)

        startFreq = 1376.5
        chanWidth = 18.518518
        nChan = 2592
#        bandWidth = chanWidth * nChan
#        centreFreq = startFreq + bandWidth / 2

        temp = obs_info_obj["nChan"]["value"]

        assert_almost_equals(obs_info_obj["nChan"]["value"], nChan, places=1)
        assert_equals(obs_info_obj["ChanWidth"]["value"], chanWidth)
        assert_equals(obs_info_obj["StartFreq"]["value"], startFreq)
 #       assert_equals(obs_info_obj["CentreFreq"]["value"], centreFreq)
 #       assert_equals(obs_info_obj["BandWidth"]["value"], bandWidth)

        self.cpservice.abortObs()
        is_complete = self.cpservice.waitObs(2000)
        assert_equals(is_complete, True)


    def test_monitoring(self):
        # monitoring points should not be there
        point_names = ['ingest.running', 'ingest0.cp.ingest.obs.StartFreq', 'ingest0.cp.ingest.obs.nChan',
                       'ingest0.cp.ingest.obs.ChanWidth', 'ingest36.cp.ingest.obs.FieldName', 'ingest36.cp.ingest.obs.ScanId']

        points = self.monitoringservice.get(point_names)
        assert_equals(len(points), 1)
        assert_equals(points[0].value.value, False)

        self.cpservice.startObs(5)
        time.sleep(5)
        # monitoring points should be there
        points = self.monitoringservice.get(point_names)
        assert_equals(len(points), 6)
        assert_equals(points[0].value.value, True)
        assert_equals(points[1].value.value, 1376.5)
        assert_equals(points[2].value.value, 864)

        chanWidth = 18.518518
        value = points[3].value.value
        assert_almost_equals(value, chanWidth, places=6)

        assert_equals(points[4].value.value, 'Virgo')
        assert_equals(points[5].value.value, 73)

        self.cpservice.abortObs()
        time.sleep(5)
        # monitoring points should be gone
        points = self.monitoringservice.get(point_names)
        assert_equals(len(points), 1)
        assert_equals(points[0].value.value, False)
