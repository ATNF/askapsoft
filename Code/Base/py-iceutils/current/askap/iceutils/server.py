# Copyright (c) 2012-2017 CSIRO
# Australia Telescope National Facility (ATNF)
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# PO Box 76, Epping NSW 1710, Australia
# atnf-enquiries@csiro.au
#
# This file is part of the ASKAP software distribution.
#
# The ASKAP software distribution is free software: you can redistribute it
# and/or modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of the License
# or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA.
#
"""
ZeroC Ice application wrapper
-----------------------------
"""
import sys
import os
import time
import Ice

# noinspection PyUnresolvedReferences
from askap.slice import FCMService
import askap.interfaces.fcm

from askap.parset import parset_to_dict

# noinspection PyUnresolvedReferences
from askap import logging

from .monitoringprovider import MonitoringProviderImpl

__all__ = ["Server"]

logger = logging.getLogger(__name__)


class FileFCMService(object):
    def __init__(self, pars):
        self._parameters = pars

    # noinspection PyUnusedLocal,PyMethodMayBeStatic
    def get(self, update=True):
        return self._parameters


class FCMWrapper(object):
    """Wrap up FCM service parameter gets to present a simpler interface
    to get only paramters of interest"""
    def __init__(self, parent):
        self._parent = parent
        self._fcm = None
        self._parameters = None
        self._init_fcm()

    def _init_fcm(self):
        prxy = self._parent._comm.stringToProxy("FCMService@FCMAdapter")
        if not prxy:
            raise RuntimeError("Invalid Proxy for FCMService")
        self._fcm = self._parent.wait_for_service(
            "FCM", askap.interfaces.fcm.IFCMServicePrx.checkedCast, prxy)

    def get(self, update=True):
        """
        Get FCM parameters for keys "common" and the service namespace, e.g.
        "dataservice".

        :param update: update from FCM (default) or uses cached version

        :return: dict

        """
        if self._parameters is None or update:
            parameters = self._fcm.get(-1, self._parent.service_key)
            common = self._fcm.get(-1, "common")
            parameters.update(common)
            self._parameters = parameters
        return self._parameters


class TimeoutError(Exception):
    """An exception raised when call timed out"""
    pass


# noinspection PyUnresolvedReferences
class Server(object):
    """A class to abstract an ice application (server). It also sets up
    connection to and initialisation from the FCM for the fiven `fcmkey`.
    The configuration can also be given as a command-line arg
    `--config=<cfg_file>` for e.g. functional test set ups.

    :param comm:
    :param fcmkey:
    :param retries:

    """
    def __init__(self, comm, fcmkey='', retries=-1):
        self.parameters = None
        self._comm = comm
        self._adapter = None
        self._mon_adapter = None
        self._adapter_config = {}
        self._services = []
        self._retries = retries
        self.service_key = fcmkey
        self.logger = logger
        self.monitoring = False
        """Enable provision of monitoring (default `False`. This automatically
        creates a :class:`MonitoringProvider` and adapter using the class
        name for adpater naming. e.g. a class named TestServer results
        in  MonitoringService@TestServerAdapter
        """
        self.configurable = True
        """Control configuration via FCM/command-line (default `True`)"""
        self.fcm = None
        """the FCM service via :obj:`FCMWrapper`"""

    def set_retries(self, retries):
        """
        *deprecated*
        """
        self._retries = retries

    def get_config(self):
        """
        Set up FCM configuration from either given file in command-line arg or
        the FCM
        :return:
        """
        if not self.configurable:
            return
        key = "--config="
        for arg in sys.argv:
            if arg.startswith(key):
                k, v = arg.split("=")
                p = os.path.expanduser(os.path.expandvars(v))
                self.parameters = parset_to_dict(open(p).read())
                self.logger.info("Initialized from local config '%s'" % p)
                self.fcm = FileFCMService(self.parameters)
                self.logger.debug("Created dummy FCM service")
                return
        self._config_from_fcm()

    def _config_from_fcm(self):
        self.fcm = FCMWrapper(self)
        self.parameters = self.fcm.get()
        self.logger.info("Initialized from fcm")

    def get_parameter(self, key, default=None):
        """
        Get an FCM parameter for the given `key` in the server's namespace

        :param key: the key
        :param default: the default value to return if key now found
        :return: value for the specified key or default

        """
        return self.parameters.get(".".join((self.service_key, key)), default)

    def wait_for_service(self, servicename, callback, *args):
        """
        Try to connect to the registry and establish connection to the given
        Ice service
        :param servicename:
        :param callback:
        :param args:
        :return:
        """
        retval = None
        delay = 5.0
        count = 0
        registry = False
        while not registry:
            try:
                retval = callback(*args)
                registry = True
            except (Ice.ConnectionRefusedException,
                    Ice.NoEndpointException,
                    Ice.NotRegisteredException,
                    Ice.ConnectFailedException,
                    Ice.DNSException) as ex:
                if self._retries > -1 and self._retries == count:
                    msg = "Couldn't connect to {0}: ".format(servicename)
                    self.logger.error(msg+str(ex))
                    raise TimeoutError(msg)
                if count < 10:
                    print >> sys.stderr, "Waiting for", servicename
                if count == 10:
                    print >> sys.stderr, "Repeated 10+ times"
                    self.logger.warn("Waiting for {0}".format(servicename))
                registry = False
                count += 1
                time.sleep(delay)
        if registry:
            self.logger.info("Connected to {0}".format(servicename))
            print >> sys.stderr, servicename, "found"
        return retval

    def _create_adapter(self, name, endpoints=None):
        if endpoints is None:
            adapter = self._comm.createObjectAdapter(name)
        else:
            adapter = self._comm.createObjectAdapterWithEndpoints(name,
                                                                  endpoints)
        logger.info("Created adapter {}".format(adapter.getName()))
        return adapter

    def setup_services(self):
        name = self.__class__.__name__ + "Adapter"
        self._adapter = self._create_adapter(
            self._adapter_config.get("name", name),
            self._adapter_config.get("endpoints", None)
        )
        self.get_config()

        if self.monitoring:
            name = name.replace("Adapter", "MonitoringAdapter")
            self._mon_adapter = self._create_adapter(
                self._adapter_config.get("name", name),
                self._adapter_config.get("endpoints", None)
            )
            self._mon_adapter.add(MonitoringProviderImpl(),
                                  self._comm.stringToIdentity(
                                      "MonitoringService"))
            self.wait_for_service("registry", self._mon_adapter.activate)

        # implement this method in derived class
        self.initialize_services()

        for service in self._services:
            self._adapter.add(service['value'],
                              self._comm.stringToIdentity(service['name']))

        self.wait_for_service("registry", self._adapter.activate)

    def run(self):
        """
        Set up the services and :meth:`wait`.
        """
        self.setup_services()
        self.wait()

    def wait(self):
        """Alias for `communicator.waitForShutdown`"""
        self._comm.waitForShutdown()

    def add_service(self, name, value):
        """
        Add the service implementation `value` under the given `name` (this will
        be the name of the identity.

        :param name: name of the identity to be known under (well known name)
        :param value: the implemnation of the interface to provide

        """
        self._services.append({'name': name, 'value': value})

    # noinspection PyMethodMayBeStatic
    def initialize_services(self):
        """Implement in sub-class with code the set up the server application.
        At a minimum :meth:`add_service` should be called."""
        pass
