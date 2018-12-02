import os
from askapdev.rbuild.builders import Setuptools as Builder

# This directory is expected by the build system
if not os.path.exists("catalogues"):
    os.makedirs("catalogues")

# This directory is expected by the build system
if not os.path.exists("images"):
    os.makedirs("images")
    
builder = Builder(".")
builder.build()
