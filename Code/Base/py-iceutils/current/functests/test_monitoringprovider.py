#!/usr/bin/env python
import os
import threading

from nose.tools import assert_equals, assert_greater
from askap.iceutils import IceSession, get_service_object

# pylint: disable-msg=E0611
import askap.interfaces.monitoring

class TestMonProvider(object):
    def __init__(self):
        self.logger = None
        self.igsession = None
        self.service = None

    def setup(self):
        os.environ["ICE_CONFIG"] = 'ice.cfg'
        os.chdir('functests')
        self.igsession = IceSession('applications2.txt', cleanup=True)
        try:
            self.igsession.start()
            self.service = get_service_object(
                self.igsession.communicator,
                "MonProviderService@MonProviderAdapter",
                askap.interfaces.monitoring.MonitoringProviderPrx
                )
        except Exception as ex:
            self.teardown()
            raise

    def teardown(self):
        os.chdir('..')
        self.igsession.terminate()

    def test_get(self):
        block = threading.Event()
        first = self.service.get(['a'])
        assert_equals(first[0].name, 'a')
        block.wait(0.1)
        second = self.service.get(['a'])
        assert_equals(second[0].name, 'a')
        assert_greater(second[0].value.value, first[0].value.value)
        both = self.service.get(['a', 'b'])
        assert_equals(len(both), 2)
        assert_equals(len(self.service.get(['c'])), 0)
