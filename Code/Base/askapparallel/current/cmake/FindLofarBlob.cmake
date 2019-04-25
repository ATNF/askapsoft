# - Try to find readline, a library for easy editing of command lines.
# Variables used by this module:
#  LOFAR_ROOT_DIR     - Readline root directory
# Variables defined by this module:
#  LofarBlob_FOUND - system has LOFAR Common
#  LofarBlob_INCLUDE_DIR  - the LOFAR Common/include directory (cached)
#  LofarBlob_INCLUDE_DIRS - the LOFAR Common include directories
#                          (identical to LofarBlob_INCLUDE_DIR)
#  LofarBlob_LIBRARY      - the LOFAR common  library (cached)
#  LofarBlob_LIBRARIES    - the LOFAR common library plus the libraries it 
#                          depends on

# Copyright (C) 2019


if(NOT LofarBlob_FOUND)

	find_path(LofarBlob_INCLUDE_DIR Blob/BlobString.h
		HINTS ${LOFAR_ROOT_DIR} PATH_SUFFIXES Blob/include)
	find_library(LofarBlob_LIBRARY Blob
                HINTS ${LOFAR_ROOT_DIR} PATH_SUFFIXES lib)
	mark_as_advanced(LofarBlob_INCLUDE_DIR LofarBlob_LIBRARY )

	set(LofarBlob_INCLUDE_DIRS ${LofarBlob_INCLUDE_DIR})
	set(LOFAR_LIBRARIES ${LofarBlob_LIBRARY})

	if(CMAKE_VERSION VERSION_LESS "2.8.3")
		find_package_handle_standard_args(LofarBlob DEFAULT_MSG LofarBlob_LIBRARY LofarBlob_INCLUDE_DIR)
        else ()
	   include(FindPackageHandleStandardArgs)
	   find_package_handle_standard_args(LofarBlob  DEFAULT_MSG LofarBlob_LIBRARY LofarBlob_INCLUDE_DIR)
        endif ()


endif(NOT LofarBlob_FOUND)

