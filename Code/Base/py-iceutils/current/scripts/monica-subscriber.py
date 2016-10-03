#!/usr/bin/env python
#
# Copyright (c) 2016 CSIRO
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
import sys
import os
import Ice
import IceStorm
# pylint: disable-msg=W0611
from askap.slice import TypedValues

# ice doesn't agree with pylint
# pylint: disable-msg=E0611
import askap.interfaces as iceint
from askap.slice import MoniCA

import atnf.atoms.mon.comms as moncomms


# noinspection PyUnusedLocal,PyMethodMayBeStatic
class PubSubClientImpl(moncomms.PubSubClient):
    def updateData(self, newdata, current=None):
        print >>sys.stderr, newdata

        
class MonicaSubscriber(object):
    """IceStorm Subscriber to monica points"""
    def __init__(self, topic):
        self.ice = self._setup_communicator()
        self.prxy = None
        self.manager = None
        self.topic_name = topic
        # Hard-wired in MoniCA server config to set up topics
        self.controltopicname = "MoniCA.PubSubControl"
        self._setup_icestorm()

    @staticmethod
    def _setup_communicator():
        if "ICE_CONFIG" in os.environ:
            return Ice.initialize(sys.argv)
        host = 'aktos10.atnf.csiro.au'
        port = 4061
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
                self.manager = IceStorm.TopicManagerPrx.checkedCast(prxstr)
            except (Ice.LocalException, Exception) as ex:
                self.manager = None
                raise ex
        self.controltopic = self.manager.retrieve(self.controltopicname)
        self.control = self.controltopic.getPublisher().ice_twoway()
        self.control = moncomms.PubSubControlPrx.uncheckedCast(self.control)

        try:
            self.topic = self.manager.retrieve(self.topic_name)
        except IceStorm.NoSuchTopic:
            try:
                self.topic = self.manager.create(self.topic_name)
            except IceStorm.TopicExists:
                self.topic = self.manager.retrieve(self.topic_name)

        self.adapter = \
            self.ice.createObjectAdapterWithEndpoints("MonicaSubscriberAdapter",
                                                      "tcp")
        self.subscriber = \
            self.adapter.addWithUUID(PubSubClientImpl()).ice_twoway()
        qos = {}
        try:
            self.topic.subscribeAndGetPublisher(qos, self.subscriber)
        except IceStorm.AlreadySubscribed:
            self.topic.unsubscribe(self.subscriber)
            self.topic.subscribeAndGetPublisher(qos, self.subscriber)
        self.adapter.activate()

    def __enter__(self):
        return self

    # noinspection PyUnusedLocal,PyBroadException
    def __exit__(self, *exc):
        try:
            self.topic.unsubscribe(self.subscriber)
            self.control.unsubscribe(self.topic_name)
        except:
            pass
        return False

    def add_points(self, points):
        subrequest = moncomms.PubSubRequest()
        subrequest.topicname = self.topic_name
        subrequest.pointnames = points
        self.control.subscribe(subrequest)
        
        
if __name__ == "__main__":
    points = []
    if len(sys.argv) > 1:
        points = sys.argv[1].split(',')
    if len(points) == 0:
        print >>sys.stderr, \
            "No points defined. Please specify comma-separated point names"
        sys.exit(1)
    with MonicaSubscriber("mypoints") as msub:
        msub.add_points(points)
        msub.ice.waitForShutdown()            
