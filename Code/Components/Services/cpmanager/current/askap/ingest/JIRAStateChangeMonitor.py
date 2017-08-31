from askap.slice import SchedulingBlockService
# noinspection PyUnresolvedReferences
from askap.interfaces.schedblock import ISBStateMonitor, ObsState

import subprocess
import time
import os
from askap.jira import Jira


class JIRAStateChangeMonitor(ISBStateMonitor):
    def __init__(self, params):
        self.params = params
        self.jira = Jira()

    def changed(self, sbid, new_state, update_time):
        if new_state.name == ObsState.OBSERVERD:
            self.jira.create_issue(sbid, "Ready for data processing")
