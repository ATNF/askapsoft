from askapdev.rbuild.builders import Setuptools as Builder

builder = Builder()
builder.remote_archive = "dogpile.cache-0.4.2.tar.gz"
builder.build()
