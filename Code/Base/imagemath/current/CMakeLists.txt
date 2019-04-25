cmake_minimum_required (VERSION 3.1.0)

project(base-imagemath VERSION 0.0.1 LANGUAGES C CXX)

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/askap-cmake)
    execute_process(COMMAND git pull 
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/askap-cmake)
else ()	
    execute_process(COMMAND git clone https://bitbucket.csiro.au/scm/askapsdp/askap-cmake.git
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif ()
set (CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/askap-cmake")

option (CXX11 "Compile as C++11 if possible" YES)
option (ENABLE_SHARED "Build shared libraries" YES)
option (ENABLE_RPATH "Include rpath in executables and shared libraries" YES)


# uninstall target
if(NOT TARGET uninstall)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY)

    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()



if (ENABLE_SHARED)
option (BUILD_SHARED_LIBS "" YES)
    if (ENABLE_RPATH)
        # Set RPATH to use for installed targets; append linker search path
        set(CMAKE_INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib" )
        set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
        set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
    endif (ENABLE_RPATH)
endif(ENABLE_SHARED)


# find packages
#find_package(Boost REQUIRED COMPONENTS system signals program_options thread chrono )
find_package(LofarCommon REQUIRED)
find_package(log4cxx REQUIRED)
find_package(Casacore REQUIRED COMPONENTS  ms images mirlib coordinates fits lattices measures scimath scimath_f tables casa)
find_package(Askap REQUIRED)
find_package(LofarBlob REQUIRED)

# include directories
include_directories( ${CMAKE_SOURCE_DIR} )
include_directories (${Boost_INCLUDE_DIRS})
add_definitions(-DHAVE_BOOST)
include_directories(${log4cxx_INCLUDE_DIRS})
add_definitions(-DHAVE_LOG4CXX)
include_directories(${LofarCommon_INCLUDE_DIRS})
include_directories(${CASACORE_INCLUDE_DIRS})
add_definitions(-DHAVE_AIPSPP)
include_directories(${ASKAP_INCLUDE_DIRS})
include_directories(${LofarBlob_INCLUDE_DIRS})

if (CASACORE3 OR CXX11)
	set(CMAKE_CXX_STANDARD 11)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

add_library(askap_imagemath

primarybeam/PrimaryBeamFactory.cc
primarybeam/PrimaryBeam.cc
primarybeam/GaussianPB.cc
utils/ImagemathUtils.cc
)

install (FILES

linmos/LinmosAccumulator.h
linmos/LinmosAccumulator.tcc
DESTINATION include/askap/imagemath/linmos

)

install (FILES

utils/ImagemathUtils.h

DESTINATION include/askap/imagemath/utils
)

install (FILES

primarybeam/GaussianPB.h
primarybeam/PrimaryBeam.h
primarybeam/PrimaryBeamFactory.h
DESTINATION include/askap/imagemath/primarybeam
)



target_link_libraries(
	askap_imagemath
	${CASACORE_LIBRARIES}
	${ASKAP_LIBRARY}
	${log4cxx_LIBRARY}
	${Boost_LIBRARIES}
	${LofarCommon_LIBRARY}
)

install (
TARGETS askap_imagemath
RUNTIME DESTINATION bin
LIBRARY DESTINATION lib
ARCHIVE DESTINATION lib
LIBRARY PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)


