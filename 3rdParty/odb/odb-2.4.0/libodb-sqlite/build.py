import os

from askapdev.rbuild.builders import Autotools as Builder

builder = Builder(pkgname="libodb-sqlite-2.4.0")

# setup the libodb dependency
libodb = builder.dep.get_install_path("libodb")
builder.add_option("CPPFLAGS=-I{0}/include".format(libodb))
builder.add_option("LDFLAGS=-L{0}/lib".format(libodb))
# --with-libodb isn't working, but CPPFLAGS and LDFLAGS are OK
# builder.add_option("--with-libodb={0}".format(libodb))

builder.remote_archive = "libodb-sqlite-2.4.0.tar.gz"
builder.nowarnings = True
builder.build()
