"""Test CP Manager service interface
"""

import os
from unittest import skip

from askap.iceutils import CPFuncTestBase, get_service_object

# always import from askap.slice before trying to import any interfaces
from askap.slice import CP
from askap.interfaces.cp import ICPObsServicePrx

# @skip
class Test(CPFuncTestBase):
    def __init__(self):
        super(Test, self).__init__()
        self.cpclient = None

    def setUp(self):
        # Note that the working directory is 'functests', thus paths are
        # relative to that location.
        os.environ["ICE_CONFIG"] = "config-files/ice.cfg"
        os.environ['TEST_DIR'] = 'test_real_ingest'
        super(Test, self).setUp()

        try:
            self.cpclient = get_service_object(
                self.ice_session.communicator,
                "CentralProcessorService@CentralProcessorAdapter",
                ICPObsServicePrx)
        except Exception as ex:
            self.shutdown()
            raise

    def test_get_service_version(self):
        #Don't test the full string, as the version changes with SVN revision or tag.
        print "Calling getServiceVersion...",
        process_name = self.cpclient.getServiceVersion().split(';')[0]
        print "DONE"
        assert 'manager' == process_name

    def test_start_abort_wait_observation_sequence(self):
        print "Starting observation...",
        self.cpclient.startObs(0)
        print "DONE"

        print "Waiting 5s for observation to complete (it shouldn't)...",
        self.cpclient.waitObs(5000)
        print "DONE"

        print "Aborting observation...",
        self.cpclient.abortObs()
        print "DONE"

        print "Waiting for observation to abort...",
        self.cpclient.waitObs(-1)
        print "DONE"
