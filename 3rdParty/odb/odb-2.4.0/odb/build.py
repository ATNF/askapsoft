import os

from askapdev.rbuild.builders import Autotools as Builder

builder = Builder(pkgname="odb-2.4.0")

# cutl dependency
libcutl = builder.dep.get_install_path("libcutl")
builder.add_option("CPPFLAGS=-I{0}/include".format(libcutl))
builder.add_option("LDFLAGS=-L{0}/lib".format(libcutl))

builder.add_option("CXXFLAGS=-O1")

builder.remote_archive = "odb-2.4.0.tar.gz"
builder.nowarnings = True
builder.build()
