"""Test CP Manager schedulingblock state transition handling
"""

import datetime
import nose
import os
import sys
import time
import threading

from nose.tools import *
from unittest import skip

# always import from askap.slice before trying to import any interfaces
from askap.slice import (
    CommonTypes,
    CP,
    QueueService,
    ObsProgramService,
    SchedulingBlockService,
    DataServiceExceptions,
)

import askap
import Ice
import IceStorm

from askap import logging
from askap.iceutils import IceSession, get_service_object
from askap.interfaces import (
    schedblock,
)


class TestSbStateChange(object):

    def null_references(self):
        self.igsession = None
        self.cpservice = None
        self.sbservice = None

    def setup(self):
        # Note that the working directory is 'functests', thus paths are
        # relative to that location.
        self.null_references()

        print os.getcwd()

        # Create the Ice session
        os.environ["ICE_CONFIG"] = "config-files/ice.cfg"
        os.environ['TEST_DIR'] = "test_sb_state_change"
        self.igsession = IceSession(
            os.path.expandvars("$TEST_DIR/applications.txt"),
            cleanup=True)

        # Supporting data
        self.program_id = "TestProgram"
        self.templ_name = "testing"

        # Create the services
        try:
            self.igsession.start()

            self.cpservice = get_service_object(
                self.igsession.communicator,
                "CentralProcessorService@CentralProcessorAdapter",
                askap.interfaces.cp.ICPObsServicePrx)

            self.sbservice = get_service_object(
                self.igsession.communicator,
                "SchedulingBlockService@DataServiceAdapter",
                askap.interfaces.schedblock.ISchedulingBlockServicePrx)

        except Exception, ex:
            self.igsession.terminate()
            raise

    def teardown(self):
        self.igsession.terminate()
        os.chdir('..')
        self.null_references()

    def test_fake(self):
        pass

    @skip
    def test_transition_executing(self):
        """CP Manager response to EXECUTING state transition"""
        print os.getcwd()
        id0 = self.sbservice.create(self.program_id, self.templ_name, alias="")
        assert id0
        self.sbservice.transition(id0, schedblock.ObsState.EXECUTING)
        assert self.sbservice.getState(id0) == schedblock.ObsState.EXECUTING
        # TODO: somehow test for manager response

    @skip
    def test_transition_processing(self):
        """CP Manager response to PROCESSING state transition"""
        assert False
