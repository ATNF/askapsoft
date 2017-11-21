# @file
# Package build script.

from askapdev.rbuild.builders import Ant as ServerBuilder

builder = ServerBuilder(".")

builder.build()

