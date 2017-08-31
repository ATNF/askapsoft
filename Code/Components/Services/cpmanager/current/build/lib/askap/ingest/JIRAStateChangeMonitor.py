from askap.slice import SchedulingBlockService
from askap.interfaces.schedblock import ISBStateMonitor, ObsState

import subprocess
import time
import os
from askap.jira import Jira

class JIRAStateChangeMonitor(ISBStateMonitor):
    def __init__(self, params):
        self.params = params
        self.jira = Jira()

    def changed(self, sbid, newState, updateTime):
        if newState.name == ObsState.OBSERVERD:
            self.jira.create_issue(sbid, "Ready for data processing")
