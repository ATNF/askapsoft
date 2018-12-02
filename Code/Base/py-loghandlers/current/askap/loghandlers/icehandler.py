"""
==========================================================================
Module :mod:`icehandler` -- python logging over IceStorm
==========================================================================

To set up an IceHandler, an IceStorm node has to be started in IceGrid.
The topic is _logger_. The :class:`IceHandler` instance will register as a
publisher under this topic. Anybody interested in reciving log messages can
then subscribe to this topic.
"""
import sys
import os
import socket
from askap import logging
from askap.logging import Handler

import Ice
import IceStorm
# noinspection PyUnresolvedReferences
from askap.slice import LoggingService
# noinspection PyUnresolvedReferences
from askap.interfaces.logging import (ILoggerPrx, ILogEvent, LogLevel)

__all__ = ['IceHandler']


_loglevel_map = {logging.NOTSET: LogLevel.TRACE,
                 logging.DEBUG: LogLevel.DEBUG,
                 logging.INFO: LogLevel.INFO,
                 logging.WARN: LogLevel.WARN,
                 logging.WARNING: LogLevel.WARN,
                 logging.ERROR: LogLevel.ERROR,
                 logging.CRITICAL: LogLevel.FATAL,
                 logging.FATAL: LogLevel.FATAL}


class IceHandler(Handler):
    """
    A handler class which writes logging records, without formatting
    to a ZeroC Ice publisher.
    """
    def __init__(self, topic='logger', host='localhost',
                 port=4061, communicator=None):
        """
        Initialize the handler.
        """
        Handler.__init__(self)
        self._internal_communicator = False
        if communicator:
            self.ice = communicator
        else:
            self._internal_communicator = True
            self.ice = self._setup_communicator(host, port)
        self.prxy = None
        self.manager = None
        self._topic = topic
        self._setup_icestorm()
        self.formatter = None
        self._hostname = socket.gethostname()

    @staticmethod
    def _setup_communicator(host, port):
        if not host:
            if not os.environ.get("ICE_CONFIG", None):
                raise OSError("'ICE_CONFIG' not set")
            return Ice.initialize(sys.argv)
        init = Ice.InitializationData()
        init.properties = Ice.createProperties()
        loc = "IceGrid/Locator:tcp -h " + host + " -p " + str(port)
        init.properties.setProperty('Ice.Default.Locator', loc)
        init.properties.setProperty('Ice.IPv6', '0')
        return Ice.initialize(init)

    def _setup_icestorm(self):
        """Create the IceStorm connection and subscribe to the logger topic.
        """
        if not self.manager:
            prxstr = self.ice.stringToProxy(
                'IceStorm/TopicManager@IceStorm.TopicManager')
            try:
                # noinspection PyUnresolvedReferences
                self.manager = IceStorm.TopicManagerPrx.checkedCast(prxstr)
            except (Ice.LocalException, Exception):
                # print >>sys.stderr, "Ice logging not working", ex
                self.manager = None
                return

        # noinspection PyUnresolvedReferences
        try:
            topic = self.manager.retrieve(self._topic)
        except IceStorm.NoSuchTopic:
            try:
                topic = self.manager.create(self._topic)
            except IceStorm.TopicExists:
                print >>sys.stderr, \
                    "Ice logging not working. Temporary error. try again"
                return

        publisher = topic.getPublisher()
        publisher = publisher.ice_twoway()

        self.prxy = ILoggerPrx.uncheckedCast(publisher)

    def emit(self, record):
        """Send the logging Record as an :class:`ILogEvent`
        """
        if not self.manager or not self.prxy:
            self._setup_icestorm()
            if not self.manager or not self.prxy:
                return
        tag = getattr(record, "tag", "")
        hostname = getattr(record, "hostname", self._hostname)

        lvl = _loglevel_map[record.levelno]
        event = ILogEvent(record.name, record.created,
                          lvl, str(record.msg), tag, hostname)
        try:
            self.prxy.send(event)
#        except Ice.CommunicatorDestroyedException:
#            pass
        except Exception as ex:
            print >>sys.stderr, "emit failed:", str(ex)

    def close(self):
        """Destroy the Ice connection"""
        if self._internal_communicator and self.ice:
            self.ice.destroy()
        Handler.close(self)
