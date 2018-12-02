import os

from askapdev.rbuild.builders import Autotools as Builder

builder = Builder(pkgname="libodb-2.4.0")
builder.remote_archive = "libodb-2.4.0.tar.gz"
builder.nowarnings = True
builder.build()
