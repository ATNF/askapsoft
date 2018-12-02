from askapdev.rbuild.builders import Setuptools as Builder

builder = Builder()
builder.remote_archive = "dogpile.cache-0.5.3.tar.gz"
builder.build()
