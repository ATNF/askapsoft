import os

from askapdev.rbuild.builders import Autotools as Builder

builder = Builder(pkgname="libodb-mysql-2.4.0")

libodb = builder.dep.get_install_path("libodb")
libmysqlc = builder.dep.get_install_path("libmysqlc")
builder.add_option('"CPPFLAGS=-I{0}/include -I{1}/include"'.format(libodb, libmysqlc))
builder.add_option('"LDFLAGS=-L{0}/lib -L{1}/lib"'.format(libodb, libmysqlc))

builder.remote_archive = "libodb-mysql-2.4.0.tar.gz"
builder.nowarnings = True
builder.build()
