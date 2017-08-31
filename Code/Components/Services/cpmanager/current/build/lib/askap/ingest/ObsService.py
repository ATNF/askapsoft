import sys, traceback, Ice, os

from askap.parset import ParameterSet
from askap.slice import CP
from askap.interfaces.cp import ICPObsService

class CPObsServerImp(ICPObsService):
    def __init__(self, params):
        self._monitor = askap.iceutils.monitoringprovider.MONITOR
        self.params = params
        self.is_running = false
        self.current_sbid = -1

    def startObs(self, sbid, current=None):
        if self.is_running:
            raise RuntimeError("Ingest Pipeline already running")

        workdir = self.params.get_parameter("ingest.workdir") + "/" + sbid
        configFile = "cpingest.in"

        if not os.path.exists(workdir):
            try:
                os.makedirs(path)
            except:
                raise RuntimeError("Could not create directory: " + workdir)

        with open(os.path.join(workdir, configFile), 'w+') as config_file:
            file.write("sbid = " + sbid)
            self.write_config(config_file, sbid, "cp.ingest.")
            self.write_config(config_file, sbid, "common.")

        self.start_ingest(workdir, sbid)

    def abortObs(self, current=None):
        self.proc.terminate()
        if proc.poll() is None:
            self.sbid = -1

    # will return the sbid the ingest is processing if it's running
    # otherwise return -1
    def get_current_sbid(self):
        if proc.poll() is None:
            self.sbid = -1

        return sbid

    def waitObs(self, timeout, current=None):
        time.sleep(timeout)
        if proc.poll() is not None:
            return False    # process still alive
        else:
            return True     # proess finished


    def write_config(self, file, sbid, key_prefix):
        dict = ParameterSet.to_dict(params)
        dict = ParameterSet.slice_parset(dict, key_prefix)
        for k,v in d.items():
            file.write(k, " = ", v)

    def start_ingest(self, work_dir, sbid):
        command = os.path.expandvars(self.params.get_parameter("ingest.command"));
        args = os.path.expandvars(self.params.get_parameter("ingest.args"));
        logfile = os.path.expandvars(self.params.get_parameter("ingest.logfile", "cpingest.log"));

        os.environmnet["sbid"] = "" + sbid

        cmd = command + " " + args
        self.proc = subprocess.Popen(cmd, shell=False,
                            stderr=logfile, stdout=logfile, cwd=work_dir)
        this.sbid = sbid;
