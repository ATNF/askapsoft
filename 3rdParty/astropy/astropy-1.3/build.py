from askapdev.rbuild.builders import Setuptools as Builder

builder = Builder()
builder.remote_archive = "astropy-1.3.tar.gz"
builder.nowarnings = True

builder.build()
