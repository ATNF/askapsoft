"""Test CP Manager schedulingblock state transition handling
"""

import sys
import os
import time
import datetime
import threading
import nose

from nose.tools import *
from unittest import skip

import askap
import askap.interfaces.schedblock
import Ice
import IceStorm

from askap.iceutils import IceSession, get_service_object
from askap.slice import (
    CommonTypes,
    CP,
    QueueService,
    ObsProgramService,
    SchedulingBlockService,
    DataServiceExceptions)
from askap.ds.models.schedulingblock import sb_states as SB_STATES


class TestSbStateChange(object):

    def test_stub(self):
        pass
