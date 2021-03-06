import os.path

from askapdev.rbuild.builders import Setuptools as Builder
import askapdev.rbuild.utils as utils

builder = Builder()
builder.remote_archive = "scipy-0.18.1.tar.gz"

platform = utils.get_platform()
if platform['system'] != 'Darwin':

    blas      = builder.dep.get_install_path("blas")
    lapack    = builder.dep.get_install_path("lapack")
    blaslib   = os.path.join(blas,   'lib/libblas.a')
    lapacklib = os.path.join(lapack, 'lib/liblapack.a')

    builder.add_env("BLAS", blaslib)
    builder.add_env("LAPACK", lapacklib)
    builder.add_file("files/site.cfg")
    
if platform['system'] == 'Darwin':

    if (int(platform['tversion'][1]) >= 10):
        builder.add_env("FFLAGS",'-Wa,-q')
builder.nowarnings = True
builder.build()
