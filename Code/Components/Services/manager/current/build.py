from askapdev.rbuild.builders import Ant as Builder
from askapdev.rbuild.builders import Data as DataBuilder

builder = Builder(".")
builder.add_run_script("cpmanager.sh", "askap/cp/manager/CpManager")

builder.build()

# deployment-specific files, i.e. start scripts
databuilder = DataBuilder("scripts")
databuilder.do_clean = False
databuilder.build()
