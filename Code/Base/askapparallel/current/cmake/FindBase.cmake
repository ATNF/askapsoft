# - Try to find readline, a library for easy editing of command lines.
# Variables used by this module:
#  BASE_ROOT_DIR     - Readline root directory
# Variables defined by this module:
#  BASE_ASKAP_FOUND - system has LOFAR Common
#  BASE_ASKAP_INCLUDE_DIR  - the LOFAR Common/include directory (cached)
#  BASE_ASKAP_INCLUDE_DIRS - the LOFAR Common include directories
#                          (identical to BASE_ASKAP_INCLUDE_DIR)
#  BASE_ASKAP_LIBRARY      - the LOFAR common  library (cached)
#  BASE_ASKAP_LIBRARIES    - the LOFAR common library plus the libraries it 
#                          depends on

# Copyright (C) 2019


if(NOT BASE_FOUND)

	find_path(BASE_INCLUDE_DIR askap/AskapLogging.h
		HINTS ${BASE_ROOT_DIR} PATH_SUFFIXES include/askap/)
	find_library(BASE_LIBRARY askap_askap
		HINTS ${BASE_ROOT_DIR} PATH_SUFFIXES lib)
	mark_as_advanced(BASE_INCLUDE_DIR BASE_LIBRARY )

	set(BASE_INCLUDE_DIRS ${BASE_INCLUDE_DIR})
	set(BASE_LIBRARIES ${BASE_LIBRARY})
        if(CMAKE_VERSION VERSION_LESS "2.8.3")
	   find_package_handle_standard_args(BASE DEFAULT_MSG BASE_LIBRARY BASE_INCLUDE_DIR)
        else ()
	   include(FindPackageHandleStandardArgs)
	   find_package_handle_standard_args(BASE DEFAULT_MSG BASE_LIBRARY BASE_INCLUDE_DIR)
        endif ()


endif(NOT BASE_FOUND)
