import os

from askapdev.rbuild.builders import Autotools as Builder

builder = Builder(pkgname="./sqlite-autoconf-3200000")
builder.remote_archive = "sqlite-autoconf-3200000.tar.gz"
builder.nowarnings = True
builder.build()
