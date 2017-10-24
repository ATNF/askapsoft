import sys, traceback, Ice, os
import time
import subprocess

from . import logger

from askap.parset import ParameterSet, slice_parset
from askap.slice import CP

# noinspection PyUnresolvedReferences
from askap.interfaces.cp import ICPObsService

import askap.iceutils.monitoringprovider


class CPObsServiceImp(ICPObsService):
    def __init__(self, params):
        self._monitor = askap.iceutils.monitoringprovider.MONITOR
        self.params = params
        self.current_sbid = -1
        self.proc = None

    def startObs(self, sbid, current=None):
        if self.current_sbid >= 0:
            logger.error("Ingest Pipeline already running")
            raise RuntimeError("Ingest Pipeline already running")

        if self.params.get("cp.ingest.workdir"):
            workdir = self.params.get("cp.ingest.workdir") + "/" + str(sbid)
            configFile = "cpingest.in"
        else:
            logger.error("cp.ingest.workdir not configured in fcm")
            raise RuntimeError("cp.ingest.workdir not configured in fcm")


        if not os.path.exists(workdir):
            try:
                os.makedirs(workdir)
            except:
                logger.error("Could not create directory: " + workdir)
                raise RuntimeError("Could not create directory: " + workdir)

        try:
            with open(os.path.join(workdir, configFile), 'w+') as config_file:
                self.write_config(config_file, sbid)

            self.start_ingest(workdir, sbid)
        except Exception as ex:
            logger.error("Could not start ingest for " + str(sbid) + ": " + str(ex))
            raise RuntimeError("Could not start ingest for: " + str(sbid) + str(ex))

    def abortObs(self, current=None):
        logger.debug("Abort " + str(self.current_sbid))
        if self.proc:
            if self.proc.poll() is None:
                self.proc.terminate()

        logger.debug("Aborted " + str(self.current_sbid))
        self.current_sbid = -1

    # will return the sbid the ingest is processing if it's running
    # otherwise return -1
    def get_current_sbid(self):
        if self.proc:
            if self.proc.poll() is not None:
                self.current_sbid = -1

        return self.current_sbid

    def waitObs(self, timeout, current=None):
        if self.proc is None: # no process is currently running
            return True

        if self.proc.poll() is not None:
            return True     # process finished

        time.sleep(timeout/1000)

        if self.proc.poll() is None:
            return False    # proess still not finished
        else:
            return True     # process finished

    def write_config(self, file, sbid):
        file.write("sbid = " + str(sbid) + "\n")
        for k,v in self.params.items():
                file.write(k + " = " + str(v) + "\n")

    def start_ingest(self, work_dir, sbid):
        command = os.path.expandvars(self.params.get("cp.ingest.command"))
        args = self.params.get("cp.ingest.args").split(' ')
        logfile = os.path.expandvars(self.params.get("cp.ingest.logfile", "cpingest.log"))

        os.environ["sbid"] = str(sbid)

        cmd = [command]
        cmd = cmd + args

        logger.info("start ingest for " + str(sbid) + ": " + str(cmd))
        with open(os.path.join(work_dir, logfile), "w") as log:
            self.proc = subprocess.Popen(cmd, shell=False,
                     stderr=log, stdout=log, cwd=work_dir)

        self.current_sbid = sbid
