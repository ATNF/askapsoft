from askapdev.rbuild.builders import Setuptools as Builder

builder = Builder()
builder.remote_archive = "matplotlib-2.0.0.tar.gz"
builder.nowarnings = True

builder.build()
