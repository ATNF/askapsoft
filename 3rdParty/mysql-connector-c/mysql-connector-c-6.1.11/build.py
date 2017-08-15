import os

from askapdev.rbuild.builders import CMake as Builder
import askapdev.rbuild.utils as utils

platform =  utils.get_platform()
builder = Builder(pkgname="mysql-connector-c-6.1.11-src")
builder.remote_archive = "mysql-connector-c-6.1.11-src.tar.gz"

# Force use of raw GNU compilers. This is due to bug #5798 seen on the Cray XC30
# (Galaxy). Builds using the newer cmake (2.8.12+) fail when cmake uses the Cray
# compiler wrappers
if platform['system'] != 'Darwin':
    builder.add_option("-DCMAKE_C_COMPILER=gcc")
    builder.add_option("-DCMAKE_CXX_COMPILER=g++")

builder.build()

# The MySQL client no longer builds the separate thread-safe libmysqlclient_r.so
# library. There is only a thread-safe library now (named without the _r
# suffix).  However, the ODB libraries still look for the old thread-safe
# name. And that can't be changed without patching the ODB configure script.
# The simplest solution is to create symlinks here.
try:
    lib_path = '{0}/{1}/lib'.format(builder._bdir, builder._installdir)
    source = os.path.join(lib_path, 'libmysqlclient.{0}')
    target = os.path.join(lib_path, 'libmysqlclient_r.{0}')

# symlink the static library
    os.symlink(source.format('a'), target.format('a'))

# symlink the dynamic library
    os.symlink(source.format('so'), target.format('so'))

except:
    pass
