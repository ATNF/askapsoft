import os

from askapdev.rbuild.builders import Autotools as Builder

builder = Builder(pkgname="libodb-boost-2.4.0")

# setup the libodb dependency
libodb = builder.dep.get_install_path("libodb")
builder.add_option("CPPFLAGS=-I{0}/include".format(libodb))

# setup the boost dependency
boost = builder.dep.get_install_path("boost")
builder.add_option("--with-boost={0}/include".format(boost))

# Set LDFLAGS
builder.add_option("LDFLAGS='-L{0}/lib -L{1}/lib'".format(
    libodb,
    boost))

builder.remote_archive = "libodb-boost-2.4.0.tar.gz"
builder.nowarnings = True
builder.build()
