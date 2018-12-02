import os

from askapdev.rbuild.builders import Autotools as Builder

builder = Builder(pkgname="libcutl-1.10.0")
builder.remote_archive = "libcutl-1.10.0.tar.gz"
builder.nowarnings = True
builder.build()
