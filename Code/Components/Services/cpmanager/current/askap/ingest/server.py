import Ice, IceStorm

from askap.iceutils import Server

from . import logger

from ObsService import CPObsServiceImp
from svcclients import IngestMonitor
from JIRAStateChangeMonitor import JIRAStateChangeMonitor


class IngestManager(Server):
    def __init__(self, comm):
        logger.debug('constructor')
        Server.__init__(self, comm, fcmkey='cp.ingest', monitoring=True)
        self._monitor = None
        self.logger = logger

    def initialize_services(self):
        logger.debug('initialize_services')

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
            subscriber = adapter.addWithUUID(JIRAStateChangeMonitor(self.parameters)).ice_oneway()
            adapter.activate()
            topic.subscribeAndGetPublisher({}, subscriber)
        except:
            raise RuntimeError("ICE adapter initialisation failed")

        cp_obsServer = CPObsServiceImp(self.parameters)
        self.add_service("CentralProcessorService", cp_obsServer)

        ingest_monitor = IngestMonitor(self._comm, self.parameters, cp_obsServer)
        ingest_monitor.start()

        logger.debug('initialize_services finished')
