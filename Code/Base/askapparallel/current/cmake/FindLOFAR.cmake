# - Try to find readline, a library for easy editing of command lines.
# Variables used by this module:
#  LOFAR_ROOT_DIR     - Readline root directory
# Variables defined by this module:
#  LOFAR_COMMON_FOUND - system has LOFAR Common
#  LOFAR_COMMON_INCLUDE_DIR  - the LOFAR Common/include directory (cached)
#  LOFAR_COMMON_INCLUDE_DIRS - the LOFAR Common include directories
#                          (identical to LOFAR_COMMON_INCLUDE_DIR)
#  LOFAR_COMMON_LIBRARY      - the LOFAR common  library (cached)
#  LOFAR_COMMON_LIBRARIES    - the LOFAR common library plus the libraries it 
#                          depends on

# Copyright (C) 2019


if(NOT LOFAR_COMMON_FOUND)

	find_path(LOFAR_COMMON_INCLUDE_DIR Common/ParameterSet.h
		HINTS ${LOFAR_ROOT_DIR} PATH_SUFFIXES Common/include)
	find_library(LOFAR_COMMON_LIBRARY Common
                HINTS ${LOFAR_ROOT_DIR} PATH_SUFFIXES lib)
	mark_as_advanced(LOFAR_COMMON_INCLUDE_DIR LOFAR_COMMON_LIBRARY )

        set(LOFAR_COMMON_INCLUDE_DIRS ${LOFAR_INCLUDE_DIR})
	set(LOFAR_LIBRARIES ${LOFAR_COMMON_LIBRARY})
	set(LOFAR_COMMON_FOUND TRUE)

endif(NOT LOFAR_COMMON_FOUND)
if(NOT LOFAR_BLOB_FOUND)

	find_path(LOFAR_BLOB_INCLUDE_DIR Blob/BlobString.h
		HINTS ${LOFAR_ROOT_DIR} PATH_SUFFIXES Blob/include)
	find_library(LOFAR_BLOB_LIBRARY Blob
                HINTS ${LOFAR_ROOT_DIR} PATH_SUFFIXES lib)
	mark_as_advanced(LOFAR_BLOB_INCLUDE_DIR LOFAR_BLOB_LIBRARY )

	set(LOFAR_BLOB_INCLUDE_DIRS ${LOFAR_BLOB_INCLUDE_DIR})
	set(LOFAR_LIBRARIES ${LOFAR_BLOB_LIBRARY})
	set(LOFAR_BLOB_FOUND TRUE)

endif(NOT LOFAR_BLOB_FOUND)
