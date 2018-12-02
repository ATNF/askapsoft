import os
import shutil
import subprocess

import askapdev.rbuild.utils as utils
from askapdev.rbuild.builders import Autotools as Builder

package_name="Healpix_3.31"
path = "{0}/src/cxx".format(package_name)

builder = Builder(
    archivename="Healpix_3.31_2016Aug26.tar.gz",
    pkgname=package_name,
    buildsubdir="src/cxx",
    installcommand=None)

# Select the HEALPix target. The configure script ignores all the standard
# variables such as LDFLAGS and CXXFLAGS, so selecting the target is the only
# reliable way to influence the compilation flags.  At Healpix 3.31, relevant
# targets are: auto, basic_gcc, generic_gcc, linux_icc, optimized_gcc, osx, and
# osx_icc
# For new versions, the src/cxx/config directory should be checked for available
# targets
platform = utils.get_platform()
target = "osx" if platform['system'] == 'Darwin' else "generic_gcc"
os.putenv("HEALPIX_TARGET", target)
builder.add_option("HEALPIX_TARGET={0}".format(target))


def run_autoconf():
    "HealPix requires autoconf prior to configure"
    subprocess.call("autoconf", cwd=path)

def install():
    """HealPix C++ makefile does not provide an install target.
    It also ignores the standard --prefix option to configure :(
    So we copy the files manually, getting the source directory from the
    build target."""
    src_path = os.path.join(path, target)
    dst_path = os.path.join(os.getcwd(), "install")

    # copytree requires that the destination not exist
    if os.path.exists(dst_path):
        shutil.rmtree(dst_path)

    shutil.copytree(src_path, dst_path, symlinks=True)


# setup the cfitsio dependency
# Some build targets honour the --with-libcfitsio flag, but others do not :(
# The targets that ignore --with-libcfitsio use the following 3 variables instead
cfitsio = builder.dep.get_install_path("cfitsio")
os.putenv("EXTERNAL_CFITSIO", "yes")
os.putenv("CFITSIO_EXT_LIB", "-L{0}/lib/ -lcfitsio".format(cfitsio))
os.putenv("CFITSIO_EXT_INC", "-I{0}/include/".format(cfitsio))
builder.add_option("--with-libcfitsio={0}".format(cfitsio))

builder.add_option("--enable-noisy-make")
builder.remote_archive = "Healpix_3.31_2016Aug26.tar.gz"
builder.nowarnings = True
builder.add_precallback(run_autoconf)
builder.add_postcallback(install)

builder.build()
