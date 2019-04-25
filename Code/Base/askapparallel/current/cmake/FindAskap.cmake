# - Try to find readline, a library for easy editing of command lines.
# Variables used by this module:
#  ASKAP_ROOT_DIR     - Readline root directory
# Variables defined by this module:
#  ASKAP_FOUND - system has LOFAR Common
#  ASKAP_INCLUDE_DIR  - the LOFAR Common/include directory (cached)
#  ASKAP_INCLUDE_DIRS - the LOFAR Common include directories
#                          (identical to ASKAP_INCLUDE_DIR)
#  ASKAP_LIBRARY      - the LOFAR common  library (cached)
#  ASKAP_LIBRARIES    - the LOFAR common library plus the libraries it 
#                          depends on

# Copyright (C) 2019


if(NOT ASKAP_FOUND)

	find_path(ASKAP_INCLUDE_DIR askap/AskapLogging.h
		HINTS ${ASKAP_ROOT_DIR} PATH_SUFFIXES include/askap/)
	find_library(ASKAP_LIBRARY askap_askap
		HINTS ${ASKAP_ROOT_DIR} PATH_SUFFIXES lib)
	mark_as_advanced(ASKAP_INCLUDE_DIR ASKAP_LIBRARY )

	set(ASKAP_INCLUDE_DIRS ${ASKAP_INCLUDE_DIR})
	set(ASKAP_LIBRARIES ${ASKAP_LIBRARY})
        if(CMAKE_VERSION VERSION_LESS "2.8.3")
	   find_package_handle_standard_args(ASKAP DEFAULT_MSG ASKAP_LIBRARY ASKAP_INCLUDE_DIR)
        else ()
	   include(FindPackageHandleStandardArgs)
	   find_package_handle_standard_args(ASKAP DEFAULT_MSG ASKAP_LIBRARY ASKAP_INCLUDE_DIR)
        endif ()


endif(NOT ASKAP_FOUND)
