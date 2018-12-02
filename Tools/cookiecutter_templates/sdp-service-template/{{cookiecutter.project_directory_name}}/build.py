# @file build.py
# build script for AutoBuild

from askapdev.rbuild.builders import Scons as Builder

# nothing much to do here. Most build details are in the SConstruct file
b = Builder(".")

# You can add additional items here if there are files that rbuild should remove during a clean.
# For example, the *.pyc files generated in the functional tests.
# b.add_extra_clean_targets("...")

b.build()
