from askapdev.rbuild.builders import Setuptools as Builder
import askapdev.rbuild.utils as utils

builder = Builder()
builder.remote_archive = "python-casacore-2.1.0.tar.gz"

# Need this to ensure _build() runs with the "python setup.py build_ext" command
builder.has_extension = True

wcslib = builder.dep.get_install_path("wcslib")
cfitsio = builder.dep.get_install_path("cfitsio")
boost   = builder.dep.get_install_path("boost")
#numpy   = builder.dep.get_install_path("numpy")
casacore = builder.dep.get_install_path("casacore")

builder.add_option("-I%s/include:%s/include:%s/include:%s/include"%(casacore,boost,wcslib,cfitsio))
builder.add_option("-L%s/lib:%s/lib:%s/lib:%s/lib"%(casacore,boost,wcslib,cfitsio))

#platform =  utils.get_platform()
#if platform['system'] == 'Darwin':
#  builder.add_env("CFLAGS",'-stdlib=libc++')


builder.build()
