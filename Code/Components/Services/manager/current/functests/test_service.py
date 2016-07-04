"""Test CP Manager service interface
"""
from common import *
from nose.tools import assert_equals
import time

class TestCPManagerService(object):
    def __init__(self):
        self.igsession = None
        self.service = None
        self.pointname1 = "site.test.point1"
        self.pmap = {}

    def setUp(self):
        os.environ["ICE_CONFIG"] = "ice.cfg"
        self.igsession = IceSession("applications.txt", cleanup=True)
        try:
            self.igsession.start()
            self.service = get_service_object(
                self.igsession.communicator,
                "CentralProcessorService@CentralProcessorAdapter",
                askap.interfaces.cp.ICPObsServicePrx
            )
        except Exception, ex:
            self.igsession.terminate()
            raise

    def tearDown(self):
        self.igsession.terminate()
        self.igsession = None

    def test_get_service_version(self):
        version = self.service.getServiceVersion()
        print version
        assert len(version) > 0
