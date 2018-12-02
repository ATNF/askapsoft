#!/usr/bin/env python

import sys
from askap.loghandlers.impl import LoggerImpl
import Ice
import IceStorm

class Server(Ice.Application):
    def run(self, args):
        manager = IceStorm.TopicManagerPrx.\
            checkedCast(self.communicator().stringToProxy('IceStorm/TopicManager@IceStorm.TopicManager'))

        topicname = "logger"
        try:
            topic = manager.retrieve(topicname)
        except IceStorm.NoSuchTopic, e:
            try:
                topic = manager.create(topicname)
            except IceStorm.TopicExists, ex:
                print self.appName() + ": temporary error. try again"
                raise
        logadapter = self.communicator().\
            createObjectAdapterWithEndpoints("LoggerService", "tcp")
        
        subscriber = logadapter.addWithUUID(LoggerImpl())
        subscriber = subscriber.ice_twoway()
        qos = {}
        qos["reliability"] = "ordered"
        try:
            topic.subscribeAndGetPublisher(qos, subscriber)
        except IceStorm.AlreadySubscribed, ex:
            raise
        logadapter.activate()
        self.shutdownOnInterrupt()
        self.communicator().waitForShutdown()
        #
        # Unsubscribe all subscribed objects.
        #
        topic.unsubscribe(subscriber)
        return 0

sys.stdout.flush()
app = Server()
sys.exit(app.main(sys.argv))
