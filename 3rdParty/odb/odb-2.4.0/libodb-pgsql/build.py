import os

from askapdev.rbuild.builders import Autotools as Builder

builder = Builder(pkgname="libodb-pgsql-2.4.0")

# setup the libodb dependency
libodb = builder.dep.get_install_path("libodb")
builder.add_option("CPPFLAGS=-I{0}/include".format(libodb))
builder.add_option("LDFLAGS=-L{0}/lib".format(libodb))

builder.remote_archive = "libodb-pgsql-2.4.0.tar.gz"
builder.nowarnings = True
builder.build()
