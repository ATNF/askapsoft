from askapdev.rbuild.builders import Setuptools as Builder

builder = Builder()
builder.remote_archive = "dogpile.core-0.4.1.tar.gz"
builder.build()
