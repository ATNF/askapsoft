"""  {{cookiecutter.service_long_name}} lifecycle test
"""

import os
import sys
from datetime import datetime
from time import sleep

from unittest import skip

# import IceStorm
from askap.iceutils import CPFuncTestBase, get_service_object
from askap.slice import I{{cookiecutter.ice_service_name}}
from askap.interfaces.{{cookiecutter.ice_interface_namespace}} import I{{cookiecutter.ice_service_name}}Prx


# @skip
class Test(CPFuncTestBase):
    def __init__(self):
        super(Test, self).__init__()
        self.ice_client = None

    def setUp(self):
        # Note that the working directory is 'functests', thus paths are
        # relative to that location.
        os.environ["ICE_CONFIG"] = "config-files/ice.cfg"
        os.environ['TEST_DIR'] = 'test_lifecycle'
        super(Test, self).setUp()

        try:
            self.ice_client = get_service_object(
                self.ice_session.communicator,
                "{{cookiecutter.ice_service_name}}@{{cookiecutter.ice_service_name}}Adapter",
                I{{cookiecutter.ice_service_name}}Prx)
        except Exception as ex:
            self.shutdown()
            raise

    def test_get_service_version(self):
        # fbs = self.feedback_service
        # fbs.clear_history()
        assert self.ice_client.getServiceVersion() == "1.0"
