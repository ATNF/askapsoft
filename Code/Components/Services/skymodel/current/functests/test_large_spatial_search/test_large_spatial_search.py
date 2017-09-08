""" Sky Model Service spatial search tests against a large database

These tests are configured to use the same SQLite database as some of the
C++ GlobalSkyModel class unit tests.

By replicating the same tests against the same database, it helps ensure
consistency between the backend C++ implementation and the Ice interface to the
SMS.
"""

import os
import sys
from datetime import datetime
from time import sleep, time

import nose.tools as nt
from unittest import skip

from askap.iceutils import CPFuncTestBase, get_service_object
from askap.slice import SkyModelService
from askap.interfaces.skymodelservice import (
    ISkyModelServicePrx,
    Coordinate,
    Rect,
    RectExtents,
    ContinuumComponent,
    ContinuumComponentPolarisation,
    SearchCriteria,
)


@skip
class Test(CPFuncTestBase):
    def __init__(self):
        super(Test, self).__init__()
        self.sms_client = None

    def setUp(self):
        # Note that the working directory is 'functests', thus paths are
        # relative to that location.
        os.environ["ICE_CONFIG"] = "config-files/ice.cfg"
        os.environ['TEST_DIR'] = 'test_large_spatial_search'
        super(Test, self).setUp()

        try:
            self.sms_client = get_service_object(
                self.ice_session.communicator,
                "SkyModelService@SkyModelServiceAdapter",
                ISkyModelServicePrx)
        except Exception as ex:
            self.shutdown()
            raise

    def test_radius_20(self):
        start = time()
        components = self.sms_client.coneSearch(
            centre=Coordinate(255.0, 0.0),
            radius=20.0,
            criteria=SearchCriteria())
        elapsed = time() - start
        sys.stderr.write('Search completed in {0} seconds ... '.format(elapsed))
        assert len(components) == 19569

    def test_radius_40(self):
        start = time()
        components = self.sms_client.coneSearch(
            centre=Coordinate(255.0, 0.0),
            radius=40.0,
            criteria=SearchCriteria())
        elapsed = time() - start
        sys.stderr.write('Search completed in {0} seconds ... '.format(elapsed))
        assert len(components) == 79100
