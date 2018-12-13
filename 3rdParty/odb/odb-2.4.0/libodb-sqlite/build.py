import os

from askapdev.rbuild.builders import Autotools as Builder
import askapdev.rbuild.utils as utils

builder = Builder(pkgname="libodb-sqlite-2.4.0")
# dependencies
libodb = builder.dep.get_install_path("libodb")
libsqlite = builder.dep.get_install_path("libsqlite3")

builder.add_option('"CPPFLAGS=-I{0}/include -I{1}/include"'.format(libodb, libsqlite))

platform=utils.get_platform()

if (platform['distribution'] == "Ubuntu"):
   builder.add_option('"LDFLAGS=-L{0}/lib/ -L{1}/lib/x86_64-linux-gnu/"'.format(libodb, libsqlite))


builder.remote_archive = "libodb-sqlite-2.4.0.tar.gz"
builder.nowarnings = True
builder.build()
