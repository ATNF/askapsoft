""" Sky Model Service lifecycle test
"""

import os
import sys
from datetime import datetime
from time import sleep

from unittest import skip

# import IceStorm
from askap.iceutils import CPFuncTestBase, get_service_object
from askap.slice import SkyModelService
from askap.interfaces.skymodelservice import ISkyModelServicePrx


# @skip
class Test(CPFuncTestBase):
    def __init__(self):
        super(Test, self).__init__()
        self.sms_client = None

    def setUp(self):
        # Note that the working directory is 'functests', thus paths are
        # relative to that location.
        os.environ["ICE_CONFIG"] = "config-files/ice.cfg"
        os.environ['TEST_DIR'] = 'test_lifecycle'
        super(Test, self).setUp()

        try:
            self.sms_client = get_service_object(
                self.ice_session.communicator,
                "SkyModelService@SkyModelServiceAdapter",
                ISkyModelServicePrx)
        except Exception as ex:
            self.shutdown()
            raise

    def test_get_service_version(self):
        # fbs = self.feedback_service
        # fbs.clear_history()
        assert self.sms_client.getServiceVersion() == "1.0"
