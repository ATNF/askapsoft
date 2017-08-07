"""
Module :mod:`askap.loghandlers.impl` -- A simple implementation of the
:class:`ILogger` interface printing to stdout
"""
import datetime
# pylint: disable-msg=W0611
from askap.slice import LoggingService

# ice doesn't agree with pylint
# pylint: disable-msg=E0611
from askap.interfaces.logging import ILogger, LogLevel

# pylint: disable-msg=W0232
class LoggerImpl(ILogger):
    # pylint: disable-msg=W0613,R0201
    def send(self, event, current=None):
        """
        Print the received message to stdout.
        """
        print """Event: %s
  Level:   %s
  Time:    %s
  Host:    %s
  Tag:     %s
  Message: %s
""" % (event.origin, str(event.level),
       datetime.datetime.fromtimestamp(event.created),
       event.hostname, event.tag,
       event.message)
