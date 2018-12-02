# @file
# build script for AutoBuild
from askapdev.rbuild.builders import Scons as Builder
from askapdev.rbuild.builders import Data as DataBuilder

# C++ stuff
b = Builder(".")
b.build()

# deployment-specific files, i.e. start scripts
databuilder = DataBuilder("scripts")
databuilder.do_clean = False
databuilder.build()

