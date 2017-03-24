from askapdev.rbuild.builders import Setuptools as Builder

builder = Builder()
builder.remote_archive = "pywcs-1.12.tar.gz"
builder.nowarnings = True
builder.build()
