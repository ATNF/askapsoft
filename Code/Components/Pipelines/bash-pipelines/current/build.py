import os, shutil
from askapdev.rbuild.builders import Data as DataBuilder
from askapdev.rbuild.builders import Builder

builder = Builder(".")
builder.build()
databuilder = DataBuilder("scripts")
databuilder.build()
