# - Try to find readline, a library for easy editing of command lines.
# Variables used by this module:
#  LOFAR_ROOT_DIR     - Readline root directory
# Variables defined by this module:
#  LofarCommon_FOUND - system has LOFAR Common
#  LofarCommon_INCLUDE_DIR  - the LOFAR Common/include directory (cached)
#  LofarCommon_INCLUDE_DIRS - the LOFAR Common include directories
#                          (identical to LofarCommon_INCLUDE_DIR)
#  LofarCommon_LIBRARY      - the LOFAR common  library (cached)
#  LofarCommon_LIBRARIES    - the LOFAR common library plus the libraries it 
#                          depends on

# Copyright (C) 2019


if(NOT LofarCommon_FOUND)

	find_path(LofarCommon_INCLUDE_DIR Common/ParameterSet.h
		HINTS ${LOFAR_ROOT_DIR} PATH_SUFFIXES Common/include)
	find_library(LofarCommon_LIBRARY Common
                HINTS ${LOFAR_ROOT_DIR} PATH_SUFFIXES lib)
	mark_as_advanced(LofarCommon_INCLUDE_DIR LofarCommon_LIBRARY )

        set(LofarCommon_INCLUDE_DIRS ${LOFAR_INCLUDE_DIR})
	set(LOFAR_LIBRARIES ${LofarCommon_LIBRARY})
	set(LofarCommon_FOUND TRUE)

	if(CMAKE_VERSION VERSION_LESS "2.8.3")
		find_package_handle_standard_args(LofarCommon DEFAULT_MSG LofarCommon_LIBRARY LofarCommon_INCLUDE_DIR)
        else ()
	   include(FindPackageHandleStandardArgs)
	   find_package_handle_standard_args(LofarCommon  DEFAULT_MSG LofarCommon_LIBRARY LofarCommon_INCLUDE_DIR)
        endif ()


endif(NOT LofarCommon_FOUND)

