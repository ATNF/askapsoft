project(base-askap)

cmake_minimum_required (VERSION 2.6.0)
include(CheckCXXCompilerFlag)
include(CheckCCompilerFlag)


set (CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 1)
set(PROJECT_VERSION_REVISION 1)
set(PROJECT_VERSION
  "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
option (CXX11 "Compile as C++11 if possible" YES)

# Detect if the compiler supports C++11 if we want to use it.
if (CXX11)
    check_cxx_compiler_flag(-std=c++11 HAS_CXX11)
    if (HAS_CXX11)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
      set(CXX11 ON)
    else()
	    message(STATUS "CASA_BUILD requires a c++11 compatible compiler")
      option(CXX11 "" NO)
    endif()
endif()

# find packages
find_package(Boost REQUIRED)
find_package(LOFAR REQUIRED)
find_package(log4cxx REQUIRED)
find_package(Casacore REQUIRED COMPONENTS  ms images mirlib coordinates fits lattices measures scimath scimath_f tables casa)

# include directories
include_directories( ${CMAKE_SOURCE_DIR} )
if (Boost_FOUND)
    include_directories (${Boost_INCLUDE_DIRS})
    add_definitions(-DHAVE_BOOST)
endif (Boost_FOUND)
if (log4cxx_FOUND)
    include_directories(${log4cxx_INCLUDE_DIRS})
    add_definitions(-DHAVE_LOG4CXX)
endif(log4cxx_FOUND)

if (LOFAR_FOUND)
	include_directories(${LOFAR_COMMON_INCLUDE_DIRS})
endif(LOFAR_FOUND)

if (CASACORE_FOUND)
	include_directories(${CASACORE_INCLUDE_DIRS})
endif(CASACORE_FOUND)

set (_modules askap)

# add some more tests and sub-directories

foreach (module ${_modules})
	add_subdirectory(${module})
endforeach (module)

