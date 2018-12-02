from askapdev.rbuild.builders import Setuptools as Builder

builder = Builder()
builder.remote_archive = "pytz-2016.10.tar.gz"
builder.build()
