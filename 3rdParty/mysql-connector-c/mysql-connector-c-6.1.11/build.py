import os.path

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
