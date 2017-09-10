import os.path

from askapdev.rbuild.builders import CMake as Builder
import askapdev.rbuild.utils as utils

# CMake doesn't know about ROOT_DIR for blas and lapack, so need to
# explicitly name them.  Want to use the dynamic libraries in order 
# to avoid link problems with missing FORTRAN symbols.
platform =  utils.get_platform()
libblas = "libblas.so"
liblapack = "liblapack.so"
if platform['system'] == 'Darwin':
    libblas = libblas.replace(".so", ".dylib")
    liblapack = liblapack.replace(".so", ".dylib")

builder = Builder(".")
builder.remote_archive = "casa-components-1.6.0.tar.gz"

cfitsio = builder.dep.get_install_path("cfitsio")
wcslib  = builder.dep.get_install_path("wcslib")
if not platform['system'] == 'Darwin':
    blas    = builder.dep.get_install_path("blas")
    lapack  = builder.dep.get_install_path("lapack")
fftw3   = builder.dep.get_install_path("fftw3")
casacore = builder.dep.get_install_path("casacore")

# CMake doesn't know about ROOT_DIR for these packages, so be explicit
if not platform['system'] == 'Darwin':
    builder.add_option("-DBLAS_LIBRARIES=%s" % os.path.join(blas, 'lib', libblas))
    builder.add_option("-DLAPACK_LIBRARIES=%s" % os.path.join(lapack, 'lib', liblapack))
# these work
builder.add_option("-DCFITSIO_ROOT_DIR=%s" % cfitsio)
builder.add_option("-DWCSLIB_ROOT_DIR=%s" % wcslib)
# but FFTW3_ROOT_DIR don't for the include part
builder.add_option("-DFFTW3_DISABLE_THREADS=ON")
builder.add_option("-DFFTW3_ROOT_DIR=%s" % fftw3)
builder.add_option("-DFFTW3_INCLUDE_DIRS=%s/include" % fftw3)
builder.add_option("-DUSE_FFTW3=ON")
builder.add_option("-DCASACORE_ROOT_DIR=%s/" % casacore)
# save some time
builder.add_option("-DBUILD_TESTING=OFF")
builder.nowarnings = True

# Force use of raw GNU compilers. This is due to bug #5798 soon on the Cray XC30.
# Builds using the newer cmake (2.8.12) fail when cmake uses the Cray compiler
# wrappers
#builder.add_option("-DCMAKE_C_COMPILER=gcc")
#builder.add_option("-DCMAKE_CXX_COMPILER=g++")

if platform['system'] == 'Darwin':
   
    if (int(platform['tversion'][1]) >= 10):
        builder.add_option("-DCMAKE_Fortran_FLAGS=-Wa,-q")

builder.add_option("-DCXX11=ON")
builder.build()
