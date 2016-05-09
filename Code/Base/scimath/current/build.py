# @file
# build script for AutoBuild

from askapdev.rbuild.builders import Scons as Builder
import askapdev.rbuild.utils as utils

platform =  utils.get_platform()

b = Builder(".")
b.build()

