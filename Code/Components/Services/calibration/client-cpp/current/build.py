# @file
# Package build script.

from askapdev.rbuild.builders import Scons as AccessorBuilder

accessor = AccessorBuilder(".")
accessor.build()
