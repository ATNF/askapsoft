from askapdev.rbuild.builders import Setuptools as Builder

builder = Builder()
builder.remote_archive = "scons_3.0.1.tar.gz"
builder.nowarnings = True
builder.build()
