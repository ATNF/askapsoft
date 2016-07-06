"""Test CP Manager service interface
"""

from common import *

import nose
from unittest import skip

class TestRealIngest(object):
    def __init__(self):
        self.igsession = None
        self.service = None

    def setUp(self):
        # Note that the working directory is 'functests', thus paths are
        # relative to that location.
        os.environ["ICE_CONFIG"] = "config-files/ice.cfg"
        os.environ['TEST_DIR'] = 'test_real_ingest'
        self.igsession = IceSession(
            os.path.expandvars("$TEST_DIR/applications.txt"),
            cleanup=True)
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
        #Don't test the full string, as the version changes with SVN revision or tag.
        print "Calling getServiceVersion...",
        process_name = self.service.getServiceVersion().split(';')[0]
        print "DONE"
        assert 'manager' == process_name

    def test_start_abort_wait_observation_sequence(self):
        print "Starting observation...",
        self.service.startObs(0)
        print "DONE"

        print "Waiting 5s for observation to complete (it shouldn't)...",
        self.service.waitObs(5000)
        print "DONE"

        print "Aborting observation...",
        self.service.abortObs()
        print "DONE"

        print "Waiting for observation to abort...",
        self.service.waitObs(-1)
        print "DONE"
