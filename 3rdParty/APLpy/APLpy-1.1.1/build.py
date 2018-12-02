from askapdev.rbuild.builders import Setuptools as Builder

builder = Builder()
builder.remote_archive = "APLpy-1.1.1.tar.gz"
builder.build()
