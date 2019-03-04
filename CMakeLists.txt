###################################################################################################
#####                                                                                         #####
##### The script sets up the frameworks project hierarchy. Currently only Windows builds have #####
##### been tested. To layout the projects, consider using the `cmake.ps1` script.             #####
#####                                                                                         #####
###################################################################################################

CMAKE_MINIMUM_REQUIRED(VERSION 3.12)

# Lookup project version.
INCLUDE("cmake/TexturizeVersion.cmake")

# Configure the project
PROJECT(Texturize LANGUAGES CXX VERSION ${Texturize_VERSION})

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
  IF(MSVC_VERSION GREATER_EQUAL "1900")
    INCLUDE(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("/std:c++latest" _cpp_latest_flag_supported)
    IF(_cpp_latest_flag_supported)
      ADD_COMPILE_OPTIONS("/std:c++latest")
    ENDIF(_cpp_latest_flag_supported)
  ELSEIF(MSVC_VERSION GREATER_EQUAL "1900")
    MESSAGE(FATAL_ERROR "Microsoft Visual Studio version \"1900\" or newer is required, but only \"${MSVC_VERSION}\" was detected.")
  ENDIF(MSVC_VERSION GREATER_EQUAL "1900")

  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /W3 /Od /EHsc")
  SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /W3 /GL /Od /Oi /Gy /EHsc")

  # Eigen produces can produce a build error C1128 in debug builds.
  IF(WITH_TAPKEE)
	SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /bigobj")
  ENDIF(WITH_TAPKEE)
ELSE(MSVC)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

  IF("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
  ENDIF("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
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
ELSEIF(NOT IS_ABSOLUTE ${CMAKE_INSTALL_PREFIX})
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
INCLUDE("${CMAKE_SOURCE_DIR}/cmake/GetPackageInfo.cmake")
MESSAGE(STATUS "  > ${CMAKE_SOURCE_DIR}/TexturizeSettings.cmake")
INCLUDE("${CMAKE_SOURCE_DIR}/cmake/TexturizeSettings.cmake")
MESSAGE(STATUS "")

# Searching for modules.
MESSAGE(STATUS "---------------------------------------------------------------------------------------------------")
MESSAGE(STATUS "Looking for modules")
MESSAGE(STATUS "---------------------------------------------------------------------------------------------------")
MESSAGE(STATUS "")
MESSAGE(STATUS "  Modules directory:    ${CMAKE_MODULE_PATH}")
MESSAGE(STATUS "")
MESSAGE(STATUS "")
MESSAGE(STATUS "  OpenCV:               ${OpenCV_DIR}")
MESSAGE(STATUS "  OpenEXR:              ${OPENEXR_LOCATION}")
MESSAGE(STATUS "  TBB:                  ${TBB_DIR}")
MESSAGE(STATUS "  HDF5:                 ${HDF5_ROOT}")
MESSAGE(STATUS "  zlib:                 ${ZLIB_ROOT}")
MESSAGE(STATUS "  tapkee:               ${TAPKEE_DIR}")
MESSAGE(STATUS "")
MESSAGE(STATUS "---------------------------------------------------------------------------------------------------")
MESSAGE(STATUS "Configuring projects...")

###################################################################################################
##### Setup project hierarchy.                                                                #####
###################################################################################################

# Framework libraries
ADD_SUBDIRECTORY(libs/Texturize.Core)
ADD_SUBDIRECTORY(libs/Texturize.Codecs)
ADD_SUBDIRECTORY(libs/Texturize.Analysis)
ADD_SUBDIRECTORY(libs/Texturize.Sampling)
ADD_SUBDIRECTORY(libs/Texturize.Codecs.EXR)

#SET_TARGET_PROPERTIES(Texturize.Core Texturize.Codecs Texturize.Analysis Texturize.Sampling Texturize.Codecs.EXR PROPERTIES FOLDER "libs")

ADD_DEPENDENCIES(Texturize.Codecs 					Texturize.Core)
ADD_DEPENDENCIES(Texturize.Analysis 				Texturize.Core)
ADD_DEPENDENCIES(Texturize.Sampling 				Texturize.Core)
ADD_DEPENDENCIES(Texturize.Codecs					Texturize.Analysis)

ADD_DEPENDENCIES(Texturize.Codecs.EXR 				Texturize.Core)
ADD_DEPENDENCIES(Texturize.Codecs.EXR 				Texturize.Codecs)

IF(BUILD_ZLIB)
  ADD_SUBDIRECTORY(${ZLIB_ROOT})
  ADD_DEPENDENCIES(Texturize.Codecs.EXR 			zlib)
ENDIF(BUILD_ZLIB)

ADD_DEPENDENCIES(Texturize.Sampling 				Texturize.Analysis)

# Adapters
IF(WITH_TAPKEE)
  ADD_SUBDIRECTORY(adapters/Texturize.Adapters.Tapkee)
  ADD_DEPENDENCIES(Texturize.Adapters.Tapkee		Texturize.Core)
  ADD_DEPENDENCIES(Texturize.Adapters.Tapkee		Texturize.Codecs)
  ADD_DEPENDENCIES(Texturize.Adapters.Tapkee		Texturize.Analysis)
  ADD_DEPENDENCIES(Texturize.Adapters.Tapkee		Texturize.Sampling)

  IF(BUILD_APP_SANDBOX)
    ADD_DEPENDENCIES(Texturize.Sandbox				Texturize.Adapters.Tapkee)
  ENDIF(BUILD_APP_SANDBOX)
ENDIF(WITH_TAPKEE)

# Apps
IF(BUILD_APP_SANDBOX)
  ADD_SUBDIRECTORY(apps/Texturize.Sandbox)
  
  ADD_DEPENDENCIES(Texturize.Sandbox 		        Texturize.Core)
  ADD_DEPENDENCIES(Texturize.Sandbox 		        Texturize.Codecs)
  ADD_DEPENDENCIES(Texturize.Sandbox   				Texturize.Codecs.EXR)
  ADD_DEPENDENCIES(Texturize.Sandbox 		        Texturize.Analysis)
  ADD_DEPENDENCIES(Texturize.Sandbox 		        Texturize.Sampling)

  #SET_TARGET_PROPERTIES(Texturize.Sandbox PROPERTIES FOLDER "apps")
ENDIF(BUILD_APP_SANDBOX)

IF(BUILD_APP_FILTERMR8)
  ADD_SUBDIRECTORY(apps/Texturize.FilterMR8)
  
  ADD_DEPENDENCIES(Texturize.FilterMR8 		        Texturize.Core)
  ADD_DEPENDENCIES(Texturize.FilterMR8 		        Texturize.Codecs)
  ADD_DEPENDENCIES(Texturize.FilterMR8 		        Texturize.Analysis)

  #SET_TARGET_PROPERTIES(Texturize.FilterMR8 PROPERTIES FOLDER "apps")
ENDIF(BUILD_APP_FILTERMR8)

IF(BUILD_APP_KMEANS)
  ADD_SUBDIRECTORY(apps/Texturize.KMeans)
  
  ADD_DEPENDENCIES(Texturize.KMeans 		        Texturize.Core)
  ADD_DEPENDENCIES(Texturize.KMeans 		        Texturize.Codecs)
  ADD_DEPENDENCIES(Texturize.KMeans 		        Texturize.Analysis)

  #SET_TARGET_PROPERTIES(Texturize.KMeans PROPERTIES FOLDER "apps")
ENDIF(BUILD_APP_KMEANS)

IF(BUILD_APP_CONTROL_MAP)
  IF(NOT WITH_TAPKEE)
	MESSAGE(FATAL_ERROR "The Control Map app requires the tapkee adapter. Enable WITH_TAPKEE to build the app.")
  ENDIF(NOT WITH_TAPKEE)
  
  ADD_SUBDIRECTORY(apps/Texturize.ControlMap)
  
  ADD_DEPENDENCIES(Texturize.ControlMap		        Texturize.Core)
  ADD_DEPENDENCIES(Texturize.ControlMap 		    Texturize.Codecs)
  ADD_DEPENDENCIES(Texturize.ControlMap		        Texturize.Analysis)

  #SET_TARGET_PROPERTIES(Texturize.ControlMap PROPERTIES FOLDER "apps")
ENDIF(BUILD_APP_CONTROL_MAP)

IF(BUILD_APP_GUIDANCE_REFINE)
  ADD_SUBDIRECTORY(apps/Texturize.GuidanceRefine)
  
  ADD_DEPENDENCIES(Texturize.GuidanceRefine	        Texturize.Core)
  ADD_DEPENDENCIES(Texturize.GuidanceRefine		    Texturize.Codecs)
  ADD_DEPENDENCIES(Texturize.GuidanceRefine		    Texturize.Analysis)

  #SET_TARGET_PROPERTIES(Texturize.GuidanceRefine PROPERTIES FOLDER "apps")
ENDIF(BUILD_APP_GUIDANCE_REFINE)
