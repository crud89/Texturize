###################################################################################################
#####                                                                                         #####
##### The script sets up the frameworks project hierarchy. Currently only Windows builds have #####
##### been tested. To layout the projects, consider using the `cmake.ps1` script.             #####
#####                                                                                         #####
###################################################################################################

cmake_minimum_required(VERSION 3.12)
project(Texturize)

SET(Texturize_VERSION_MAJOR 1)
SET(Texturize_VERSION_MINOR 1)

MESSAGE(STATUS "---------------------------------------------------------------------------------------------------")
MESSAGE(STATUS "Building Texturize Version ${Texturize_VERSION_MAJOR}.${Texturize_VERSION_MINOR}")
MESSAGE(STATUS "---------------------------------------------------------------------------------------------------")
MESSAGE(STATUS "")

# Defaulting build type to release.
IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE "Release")
  MESSAGE(STATUS "  Build type not specified: Defaulting to `Release`.")
  MESSAGE(STATUS "")
ENDIF()

# Set compiler flags
IF(MSVC)
   SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /W3 /MD /Od /EHsc")
   SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /W3 /GL /Od /Oi /Gy /EHsc")
ENDIF()

#IF(NOT MSVC)
#   SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
#   IF("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
#       SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
#   ENDIF()
#ENDIF()

# Ensure that the build is out of source.
STRING(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}" IS_IN_SOURCE_BUILD)

IF(NOT IS_IN_SOURCE_BUILD)
    GET_FILENAME_COMPONENT(PARENT_DIR ${CMAKE_SOURCE_DIR} PATH)
    STRING(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${PARENT_DIR}" IS_IN_SOURCE_BUILD)
ENDIF(NOT IS_IN_SOURCE_BUILD)

IF(IS_IN_SOURCE_BUILD)
    MESSAGE(FATAL_ERROR "FATAL: ")
ENDIF(IS_IN_SOURCE_BUILD)

# Builds appear to be good, setup environment variables.
# If the install prefix is not set, set it relative to the binary directory.
# If it is set, but relative, append it to the binary directory.
# If it is set to an absolute path, don't change anything.
IF(NOT DEFINED CMAKE_INSTALL_PREFIX)
    SET(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Install directory")
ELSEIF(NOT IS_ABSOLUTE CMAKE_INSTALL_PREFIX)
    SET(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_PREFIX}")
ENDIF()

SET(CMAKE_INSTALL_INCLUDEDIR            "${CMAKE_INSTALL_PREFIX}/include")
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY      "${CMAKE_INSTALL_PREFIX}/lib/")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY      "${CMAKE_INSTALL_PREFIX}/bin/")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY      "${CMAKE_INSTALL_PREFIX}/bin/")
SET(CMAKE_EXECUTABLE_OUTPUT_DIRECTORY   "${CMAKE_INSTALL_PREFIX}/bin/")
SET(CMAKE_PDB_OUTPUT_DIRECTORY          "${CMAKE_INSTALL_PREFIX}/bin/")

LIST(APPEND CMAKE_MODULE_PATH           "${CMAKE_SOURCE_DIR}/modules")

MESSAGE(STATUS "  Source directory:     ${CMAKE_SOURCE_DIR}")
MESSAGE(STATUS "  Build directory:      ${CMAKE_BINARY_DIR}")
MESSAGE(STATUS "  Install directory:    ${CMAKE_INSTALL_PREFIX}")
MESSAGE(STATUS "")

# Searching for modules.
MESSAGE(STATUS "---------------------------------------------------------------------------------------------------")
MESSAGE(STATUS "Looking for modules")
MESSAGE(STATUS "---------------------------------------------------------------------------------------------------")
MESSAGE(STATUS "")
MESSAGE(STATUS "  Modules directory:    ${CMAKE_MODULE_PATH}")
MESSAGE(STATUS "")

IF(OpenCV_FOUND)
    MESSAGE(STATUS "  OpenCV:               Found")
ELSE()
    SET(OpenCV_DIR "${CMAKE_MODULE_PATH}/opencv")
    MESSAGE(STATUS "  OpenCV:               Not found")
ENDIF()

IF(OpenEXR_FOUND)
    MESSAGE(STATUS "  OpenEXR:              Found")
    SET(ILMBASE_LOCATION ${OPENEXR_LOCATION})
ELSE()
    SET(OPENEXR_LOCATION "${CMAKE_MODULE_PATH}")
    SET(ILMBASE_LOCATION "${CMAKE_MODULE_PATH}")
    MESSAGE(STATUS "  OpenEXR:              Not found")
ENDIF()

IF(TBB_FOUND)
    MESSAGE(STATUS "  TBB:                  Found")
ELSE()
    SET(TBB_DIR "${CMAKE_MODULE_PATH}/tbb")
    MESSAGE(STATUS "  TBB:                  Not found")
ENDIF()

IF(HDF5_FOUND)
    MESSAGE(STATUS "  HDF5:                 Found")
ELSE()
    SET(HDF5_ROOT "${CMAKE_MODULE_PATH}/hdf5/cmake")
    MESSAGE(STATUS "  HDF5:                 Not found")
ENDIF()

MESSAGE(STATUS "")
MESSAGE(STATUS "---------------------------------------------------------------------------------------------------")
MESSAGE(STATUS "Trying to importing modules from the following directories:")
MESSAGE(STATUS "---------------------------------------------------------------------------------------------------")
MESSAGE(STATUS "")
MESSAGE(STATUS "  OpenCV:               ${OpenCV_DIR}")
MESSAGE(STATUS "  OpenEXR:              ${OPENEXR_LOCATION}")
MESSAGE(STATUS "  TBB:                  ${TBB_DIR}")
MESSAGE(STATUS "  HDF5:                 ${HDF5_ROOT}")
MESSAGE(STATUS "")
MESSAGE(STATUS "---------------------------------------------------------------------------------------------------")
MESSAGE(STATUS "Configuring projects...")

# Setup project hierarchy.
add_subdirectory(Texturize.Core)
add_subdirectory(Texturize.Codecs)
add_subdirectory(Texturize.Analysis)
add_subdirectory(Texturize.Sampling)
add_subdirectory(Texturize.Codecs.EXR)

add_subdirectory(Texturize.Sandbox)

add_dependencies(Texturize.Codecs 		        Texturize.Core)
add_dependencies(Texturize.Analysis 		    Texturize.Core)
add_dependencies(Texturize.Sampling 		    Texturize.Core)
add_dependencies(Texturize.Codecs		        Texturize.Analysis)
add_dependencies(Texturize.Codecs.EXR 		    Texturize.Core)
add_dependencies(Texturize.Codecs.EXR 		    Texturize.Codecs)
add_dependencies(Texturize.Sampling 		    Texturize.Analysis)

add_dependencies(Texturize.Sandbox 		        Texturize.Core)
add_dependencies(Texturize.Sandbox 		        Texturize.Codecs)
add_dependencies(Texturize.Sandbox   		    Texturize.Codecs.EXR)
add_dependencies(Texturize.Sandbox 		        Texturize.Analysis)
add_dependencies(Texturize.Sandbox 		        Texturize.Sampling)