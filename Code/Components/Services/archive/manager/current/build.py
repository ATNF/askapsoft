import sys
from askapdev.rbuild.builders import Ant as Builder
from askapdev.rbuild.builders import Data as DataBuilder

builder = Builder(".")
builder.add_run_script("cpmanager.sh", "askap/cp/manager/CpManager")

builder.build()

# deployment-specific files, i.e. start scripts
databuilder = DataBuilder("scripts")
# Monkey patch the data builder so it doesn't try to run any tests.
# It doesn't implement any, and it causes errors when running functional tests.
# See https://jira.csiro.au/browse/ASKAPSDP-2080
databuilder._functest = lambda: None
databuilder._test = lambda: None
databuilder.do_clean = False
databuilder.build()
