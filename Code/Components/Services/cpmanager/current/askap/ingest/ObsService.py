import os
import time
import subprocess

from . import logger

from askap.slice import CP  # noqa: F401

# noinspection PyUnresolvedReferences
from askap.interfaces.cp import (ICPObsService,
                                 AlreadyRunningException,
                                 PipelineStartException)

import askap.iceutils.monitoringprovider


class CPObsServiceImp(ICPObsService):
    def __init__(self, params):
        self._monitor = askap.iceutils.monitoringprovider.MONITOR
        self.fcm = params
        self.current_sbid = -1
        self.proc = None
        self.params = None

    def startObs(self, sbid, current=None):
        logger.debug("start observation for " + str(sbid))

        self.current_sbid = self.get_current_sbid()

        if self.current_sbid >= 0:
            msg = "Ingest Pipeline already running for SB{}".format(
                self.current_sbid)
            logger.error(msg)
            raise AlreadyRunningException(msg)

        # everytime an obs is started, load latest from fcm
        self.params = self.fcm.get()
        logger.debug("finished loading fcm for " + str(sbid))

        if self.params.get("cp.ingest.workdir"):
            workdir = self.params.get("cp.ingest.workdir") + "/" + str(sbid)
            configFile = "cpingest.in"
        else:
            logger.error("cp.ingest.workdir not configured in fcm")
            raise PipelineStartException(
                "cp.ingest.workdir not configured in fcm")

        if not os.path.exists(workdir):
            try:
                os.makedirs(workdir)
            except:
                logger.error("Could not create directory: " + workdir)
                raise PipelineStartException(
                    "Could not create directory: " + workdir)

        try:
            with open(os.path.join(workdir, configFile), 'w+') as config_file:
                logger.debug("writing ingest config: " + config_file.name)
                self.write_config(config_file, sbid)
        except Exception as ex:
            logger.error("Could not write ingest config for " +
                         str(sbid) + ": " + str(ex))
            raise PipelineStartException(
                "Could not write ingest config for " + str(sbid) + str(ex))

        try:
            self.start_ingest(workdir, sbid)
        except Exception as ex:
            logger.error("Could not start ingest for " +
                         str(sbid) + ": " + str(ex))
            raise RuntimeError(
                "Could not start ingest for: " + str(sbid) + str(ex))

    def abortObs(self, current=None):
        logger.debug("Abort " + str(self.current_sbid))
        if self.proc:
            if self.proc.poll() is None:
                logger.debug("Initiated abort for " + str(self.current_sbid))
                self.proc.terminate()
                self.proc = None
            else:
                rc = None
                if self.proc:
                    rc = self.proc.returncode
                logger.debug(
                    "abortObs ingest process returncode={}".format(rc)
                )

    # will return the sbid the ingest is processing if it's running
    # otherwise return -1
    def get_current_sbid(self):
        if self.proc:
            if self.proc.poll() is not None:
                self.current_sbid = -1
        else:
            self.current_sbid = -1
        return self.current_sbid

    def waitObs(self, timeout, current=None):
        if self.proc is None:  # no process is currently running
            logger.debug("waitObs called and no ingest process running")
            return True

        if self.proc.poll() is not None:
            logger.info("ingest process finished with returncode {}".format(
                self.proc.returncode
            ))
            return True     # process finished

        if timeout == 0:
            return False    # process not finished, but non blocking

        if timeout < 0:     # wait until ingest process has finished
            logger.debug("Blocking until ingest process has finished")
            self.proc.wait()
            self.current_sbid = -1
            logger.debug("Ingest process has finished")
            return True

        time_left = timeout/1000.0
        sleep_time = 0.1

        while time_left > 0:
#            logger.debug(
#                "waitObs with timeout - time left = {}".format(time_left)
#            )
            if time_left < sleep_time:
                time.sleep(time_left)
            else:
                time.sleep(sleep_time)

            time_left -= sleep_time
            if self.proc.poll() is not None:
                self.current_sbid = -1
                logger.debug("Ingest process has finished (after timeout)")
                return True     # process finished
        logger.debug("default return at end of waitObs")
        return False

    def write_config(self, file_name, sbid):
        file_name.write("sbid=" + str(sbid) + "\n")
        for k, v in self.params.items():
            # strip out prefixes 'common.' and 'cp.ingest'
            key = k
            key = key.replace('common.', '', 1)
            key = key.replace('cp.ingest.', '', 1)
            file_name.write(key + "=" + str(v) + "\n")

    def start_ingest(self, work_dir, sbid):
        command = self.params.get("cp.ingest.command")
        args = self.params.get("cp.ingest.args").split(' ')
        logfile = self.params.get("cp.ingest.logfile", "cpingestlauncher.log")

        os.environ["sbid"] = str(sbid)

        cmd = [command]
        cmd = cmd + args

        logger.info("start ingest for " + str(sbid) + ": " + str(cmd))
        with open(os.path.join(work_dir, logfile), "w") as log:
            self.proc = subprocess.Popen(cmd, shell=False,
                                         stderr=log, stdout=log, cwd=work_dir)

        self.current_sbid = sbid
