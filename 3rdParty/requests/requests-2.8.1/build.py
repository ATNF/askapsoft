from askapdev.rbuild.builders import Setuptools as Builder

builder = Builder()
builder.remote_archive = "requests-2.8.1.tar.gz"
builder.build()
