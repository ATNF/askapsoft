#!/usr/bin/env python
import sys
import os
import subprocess
import time
import socket
import threading
# pylint: disable-msg=E0611
from nose.tools import assert_equals, assert_not_equals, assert_true
import askap.loghandlers
import IceStorm
import askap.logging
from askap.iceutils import IceSession, get_service_object
# pylint: disable-msg=W0611
from askap.slice import LoggingService
# ice doesn't agree with pylint
# pylint: disable-msg=E0611
from askap.interfaces.logging import ILogger


last_event = []
EVENT = threading.Event()

# pylint: disable-msg=W0232
class LoggerImpl(ILogger):
    # pylint: disable-msg=W0613,W0603,R0201
    def send(self, event, current=None):        
        global last_event
        EVENT.set()
        last_event = [event.origin, event.level, event.created,
                      event.message, event.tag, event.hostname]

class LogSubscriber(object):
    def __init__(self, comm):
        self.ice = comm
        self.manager = get_service_object(
            self.ice,
            'IceStorm/TopicManager@IceStorm.TopicManager',
            IceStorm.TopicManagerPrx)
        topicname = "logger"
        try:
            self.topic = self.manager.retrieve(topicname)
        except IceStorm.NoSuchTopic:
            try:
                self.topic = self.manager.create(topicname)
            except IceStorm.TopicExists:
                self.topic = self.manager.retrieve(topicname)
        # defined in config.icegrid
        self.adapter = \
            self.ice.createObjectAdapterWithEndpoints("LoggingServiceAdapter",
                                                      "tcp")

        subscriber = self.adapter.addWithUUID(LoggerImpl()).ice_oneway()
        qos = {}
        try:
            self.topic.subscribeAndGetPublisher(qos, subscriber)
        except IceStorm.AlreadySubscribed:
            self.topic.unsubscribe(self.subscriber)
            self.topic.subscribeAndGetPublisher(qos, self.subscriber)
        self.adapter.activate()


def check_received():
    for i in range(10):
        if len(last_event) > 0:
            return True
        time.sleep(0.5)
    return False


class TestIceLogger(object):
    def __init__(self):
        self.logger = None
        self.subscriber = None
        self.isession = None
        self.topic = None
        self.adapter = None

    def setup(self):
        os.environ["ICE_CONFIG"] = 'ice.cfg'
        os.chdir('functests')
        self.isession = IceSession(cleanup=True)
        try:
            self.isession.add_app("icebox")
            self.isession.start()
            self.subscriber = LogSubscriber(self.isession.communicator)
            askap.logging.config.fileConfig("test.log_cfg")
            self.logger = askap.logging.getLogger(__name__)
        except Exception as ex:
            os.chdir("..")
            self.isession.terminate()
            raise

    def test_info(self):
        self.logger.setLevel(askap.logging.INFO)
        msg = "Log Test"
        tag = "gotcha"
        self.logger.info(msg, extra={"tag": tag})
        EVENT.wait(5)
        # make sure it gets delivered
        assert_true(len(last_event) > 0, "No log event received")
#        assert_true(check_received()
        assert_equals(last_event[0], __name__)
        assert_equals(last_event[-3], msg)
        assert_equals(last_event[-2], tag)
        assert_equals(last_event[-1], socket.gethostname())

    def teardown(self):
        self.isession.terminate()
        os.chdir("..")
