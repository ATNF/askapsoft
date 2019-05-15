# @file
# build script for AutoBuild

from askapdev.rbuild.builders import Scons as Builder

b = Builder(".")
casarest = b.dep.get_install_path("casarest")
b.add_option("-I%s/casarest"%(casarest))
b.build()
