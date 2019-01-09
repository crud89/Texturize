###################################################################################################
#####                                                                                         #####
##### The script sets up the frameworks project hierarchy. Currently only Windows builds have #####
##### been tested. To layout the projects, consider using the `cmake.ps1` script.             #####
#####                                                                                         #####
###################################################################################################

CMAKE_MINIMUM_REQUIRED(VERSION 3.12)
PROJECT(Texturize)

# Major and minor project version.
SET(Texturize_VERSION_MAJOR 1)
SET(Texturize_VERSION_MINOR 2)

MESSAGE(STATUS "---------------------------------------------------------------------------------------------------")
MESSAGE(STATUS "Building Texturize Version ${Texturize_VERSION_MAJOR}.${Texturize_VERSION_MINOR}")
MESSAGE(STATUS "---------------------------------------------------------------------------------------------------")
MESSAGE(STATUS "")

# Defaulting build type to release.
IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE "Release")
  MESSAGE(STATUS "  Build type not specified: Defaulting to `Release`.")
  MESSAGE(STATUS "")
ENDIF(NOT CMAKE_BUILD_TYPE)

# Get the config string from the build profile.
STRING(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)

# Set compiler flags
IF(MSVC)
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /W3 /Od /EHsc")
  SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /W3 /GL /Od /Oi /Gy /EHsc")
ELSE(MSVC)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

  IF("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
  ENDIF(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
ENDIF(MSVC)

# Ensure that the build is out of source.
STRING(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}" IS_IN_SOURCE_BUILD)

IF(NOT IS_IN_SOURCE_BUILD)
  GET_FILENAME_COMPONENT(PARENT_DIR ${CMAKE_SOURCE_DIR} PATH)
  STRING(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${PARENT_DIR}" IS_IN_SOURCE_BUILD)
ENDIF(NOT IS_IN_SOURCE_BUILD)

IF(IS_IN_SOURCE_BUILD)
  MESSAGE(FATAL_ERROR "FATAL: In source builds are not allowed.")
ENDIF(IS_IN_SOURCE_BUILD)

# Builds appear to be good, setup environment variables.
# If the install prefix is not set, set it relative to the binary directory.
# If it is set, but relative, append it to the binary directory.
# If it is set to an absolute path, don't change anything.
IF(NOT DEFINED CMAKE_INSTALL_PREFIX)
  SET(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Install directory")
ELSEIF(NOT IS_ABSOLUTE CMAKE_INSTALL_PREFIX)
  SET(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_PREFIX}")
ENDIF(NOT DEFINED CMAKE_INSTALL_PREFIX)

# Setup output directories.
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY      "${CMAKE_BINARY_DIR}/build/lib/")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY      "${CMAKE_BINARY_DIR}/build/bin/")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY      "${CMAKE_BINARY_DIR}/build/bin/")
SET(CMAKE_EXECUTABLE_OUTPUT_DIRECTORY   "${CMAKE_BINARY_DIR}/build/bin/")
SET(CMAKE_PDB_OUTPUT_DIRECTORY          "${CMAKE_BINARY_DIR}/build/bin/")

LIST(APPEND CMAKE_MODULE_PATH           "${CMAKE_SOURCE_DIR}/modules")

MESSAGE(STATUS "  Source directory:     ${CMAKE_SOURCE_DIR}")
MESSAGE(STATUS "  Build directory:      ${CMAKE_BINARY_DIR}")
MESSAGE(STATUS "  Install directory:    ${CMAKE_INSTALL_PREFIX}")
MESSAGE(STATUS "")

# Include macros and functions
MESSAGE(STATUS "  Looking up macros and functions...")
MESSAGE(STATUS "  > ${CMAKE_SOURCE_DIR}/GetModuleInfo.cmake")
INCLUDE("${CMAKE_SOURCE_DIR}/GetPackageInfo.cmake")
MESSAGE(STATUS "")

# Searching for modules.
MESSAGE(STATUS "---------------------------------------------------------------------------------------------------")
MESSAGE(STATUS "Looking for modules")
MESSAGE(STATUS "---------------------------------------------------------------------------------------------------")
MESSAGE(STATUS "")
MESSAGE(STATUS "  Modules directory:    ${CMAKE_MODULE_PATH}")
MESSAGE(STATUS "")

IF("${OpenCV_DIR}" STREQUAL "")
  SET(OpenCV_DIR "${CMAKE_MODULE_PATH}/opencv")
ENDIF("${OpenCV_DIR}" STREQUAL "")

IF("${OPENEXR_LOCATION}" STREQUAL "")
  SET(OPENEXR_LOCATION "${CMAKE_MODULE_PATH}")
ENDIF("${OPENEXR_LOCATION}" STREQUAL "")

IF("${TBB_DIR}" STREQUAL "")
  SET(TBB_DIR "${CMAKE_MODULE_PATH}")
ENDIF("${TBB_DIR}" STREQUAL "")

IF("${HDF5_ROOT}" STREQUAL "")
  SET(HDF5_ROOT "${CMAKE_MODULE_PATH}")
ENDIF("${HDF5_ROOT}" STREQUAL "")

IF("${ZLIB_ROOT}" STREQUAL "")
  SET(ZLIB_ROOT "${CMAKE_MODULE_PATH}/zlib")
ENDIF("${ZLIB_ROOT}" STREQUAL "")


MESSAGE(STATUS "")
MESSAGE(STATUS "  OpenCV:               ${OpenCV_DIR}")
MESSAGE(STATUS "  OpenEXR:              ${OPENEXR_LOCATION}")
MESSAGE(STATUS "  TBB:                  ${TBB_DIR}")
MESSAGE(STATUS "  HDF5:                 ${HDF5_ROOT}")
MESSAGE(STATUS "  zlib:                 ${ZLIB_ROOT}")
MESSAGE(STATUS "")
MESSAGE(STATUS "---------------------------------------------------------------------------------------------------")
MESSAGE(STATUS "Configuring projects...")

###################################################################################################
##### Setup project hierarchy.                                                                #####
###################################################################################################

ADD_SUBDIRECTORY(Texturize.Core)
ADD_SUBDIRECTORY(Texturize.Codecs)
ADD_SUBDIRECTORY(Texturize.Analysis)
ADD_SUBDIRECTORY(Texturize.Sampling)
ADD_SUBDIRECTORY(Texturize.Codecs.EXR)
ADD_SUBDIRECTORY(${ZLIB_ROOT})

ADD_SUBDIRECTORY(Texturize.Sandbox)

ADD_DEPENDENCIES(Texturize.Codecs 		        Texturize.Core)
ADD_DEPENDENCIES(Texturize.Analysis 		      Texturize.Core)
ADD_DEPENDENCIES(Texturize.Sampling 		      Texturize.Core)
ADD_DEPENDENCIES(Texturize.Codecs		          Texturize.Analysis)
ADD_DEPENDENCIES(Texturize.Codecs.EXR 		    Texturize.Core)
ADD_DEPENDENCIES(Texturize.Codecs.EXR 		    Texturize.Codecs)
ADD_DEPENDENCIES(Texturize.Codecs.EXR 		    zlib)
ADD_DEPENDENCIES(Texturize.Sampling 		      Texturize.Analysis)

ADD_DEPENDENCIES(Texturize.Sandbox 		        Texturize.Core)
ADD_DEPENDENCIES(Texturize.Sandbox 		        Texturize.Codecs)
ADD_DEPENDENCIES(Texturize.Sandbox   		      Texturize.Codecs.EXR)
ADD_DEPENDENCIES(Texturize.Sandbox 		        Texturize.Analysis)
ADD_DEPENDENCIES(Texturize.Sandbox 		        Texturize.Sampling)