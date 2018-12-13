#!/bin/bash
# Ensure you have these dependencies on your mac:
brew install mpich 
brew install scons
brew install cfitsio --with-reentrant
brew install boost
brew install boost-python
ln -s /usr/local/lib/libboost_python27.dylib /usr/local/lib/libboost_python.dylib
brew install healpix
brew install doxygen
brew install cmake
brew install ice // this may not be sufficient and you may need:
brew install zeroc-ice/tap/ice —with-java —with-additional-compilers 
