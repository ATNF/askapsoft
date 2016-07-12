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

from askap.iceutils import IceSession, get_service_object

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
from askap.interfaces import (
    schedblock,
)


class TestSbStateChange(object):

    def test_stub(self):
        print(dir(schedblock.ObsState))
        pass
