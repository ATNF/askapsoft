import sys, traceback, Ice, IceStorm

from askap.parset import ParameterSet

from askap.iceutils import Server
from askap.parset import ParameterSet

from . import logger

from ObsService import CPObsServerImp
from svcclients import IngestMonitor

class IngestManager(Server):
    def __init__(self, comm):
        Server.__init__(self, comm, fcmkey='cpmanager')
        self._monitor = None
        self.logger = logger

    def initialize_services(self):
        topicname = "sbstatechange"
        prx = self._comm.stringToProxy(
            'IceStorm/TopicManager@IceStorm.TopicManager')
        manager = self.wait_for_service("IceStorm",
                                        IceStorm.TopicManagerPrx.checkedCast,
                                        prx)
        try:
            topic = manager.retrieve(topicname)
        except IceStorm.NoSuchTopic:
            try:
                topic = manager.create(topicname)
            except IceStorm.TopicExists:
                topic = manager.retrieve(topicname)

        try:
            adapter = self._comm.createObjectAdapterWithEndpoints("SBStateMonitorAdapter", "tcp")
            subscriber = adapter.addWithUUID(JIRAStateChangeMonitor(self.parameters)).ice_oneway();
            adapter.activate()
            topic.subscribeAndGetPublisher({}, subscriber)
        except:
            raise RuntimeError("ICE adapter initialisation failed")

        cpObsServer = CPObsServerImp(self.parameters)
        self.add_service("CentralProcessorService", cpObsServer)

        ingest_monitor = IngestMonitor(self._comm, self.parameters, cpObsServer)
        ingest_monitor.start()
