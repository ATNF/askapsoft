from askapdev.rbuild.builders import Setuptools as Builder

builder = Builder(".")
builder.add_install_file("files/")
builder.build()
