# - Try to find log4cxx, a library for easy editing of command lines.
# Variables used by this module:
#  log4cxx_ROOT_DIR     - Readline root directory
# Variables defined by this module:
#  log4cxx_FOUND - system has log4cxx Common
#  log4cxx_INCLUDE_DIR  - the log4cxx Common/include directory (cached)
#  log4cxx_INCLUDE_DIRS - the log4cxx Common include directories
#                          (identical to log4cxx_INCLUDE_DIR)
#  log4cxx_LIBRARY      - the log4cxx common  library (cached)
#  log4cxx_LIBRARIES    - the log4cxx common library plus the libraries it 
#                          depends on

# Copyright (C) 2019


if(NOT log4cxx_FOUND)
        message(STATUS "Searching for log4cxx/logger.h")
	find_path(log4cxx_INCLUDE_DIR log4cxx/logger.h)
	find_library(log4cxx_LIBRARY log4cxx)
        include( FindPackageHandleStandardArgs )
        FIND_PACKAGE_HANDLE_STANDARD_ARGS( log4cxx log4cxx_INCLUDE_DIR log4cxx_LIBRARY )
        if( LOG4CXX_FOUND )
            set( log4cxx_FOUND TRUE )
        endif()
endif(NOT log4cxx_FOUND)
