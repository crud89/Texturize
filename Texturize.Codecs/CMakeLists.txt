###################################################################################################
#####                                                                                         #####
##### Codecs library, containing basic interfaces for asset codecs and persistence.           #####
#####                                                                                         #####
###################################################################################################

CMAKE_MINIMUM_REQUIRED(VERSION 3.12 FATAL_ERROR)
SET(PROJECT_NAME Texturize.Codecs)
PROJECT(${PROJECT_NAME} CXX)
SET(CONFIG_NAME ${PROJECT_NAME}Config)

MESSAGE(STATUS "---------------------------------------------------------------------------------------------------")
MESSAGE(STATUS "")
MESSAGE(STATUS "Setting up project: ${PROJECT_NAME}...")

# Set compiler flags
IF(MSVC)
  # Switch from Multi-Thread DLL (/MD) to Multi-Thread Debug DLL (/MDd) for debug builds.
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
  SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
ENDIF(MSVC)

# Specify source folders, relative to the project sources directory.
SET(INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include")
SET(SOURCES_DIR "${PROJECT_SOURCE_DIR}/src")

MESSAGE(STATUS "  Include directory:    ${INCLUDE_DIR}")
MESSAGE(STATUS "  Source directory:     ${SOURCES_DIR}")
MESSAGE(STATUS "")

###################################################################################################
##### Build type specific preprocessor definitions.                                           #####
###################################################################################################

ADD_DEFINITIONS(
  -DTEXTURIZE_EXPORTS 
  -D_WINDOWS 
  -D_USRDLL
  -DTEXTURIZECORE_EXPORTS 
  -DUNICODE 
  -D_UNICODE
)

###################################################################################################
##### Resolve package dependencies.                                                           #####
###################################################################################################

# Finding packages from <ModuleName>_ROOT variables first.
CMAKE_POLICY(SET CMP0074 NEW)
SET(MODULE_NAMES)
SET(MODULE_INSTALLER)
SET(MODULE_INCLUDES)

###################################################################################################
##### Find HDF5.                                                                              #####
###################################################################################################
SET(HDF5_USE_STATIC_LIBRARIES OFF)
SET(HDF5_PKG_TYPE SHARED)
STRING(TOLOWER ${HDF5_PKG_TYPE} HDF5_SEARCH_TYPE)

FIND_PACKAGE(HDF5 NAMES hdf5 COMPONENTS C CXX ${HDF5_SEARCH_TYPE})

SET(HDF5_LIBS ${HDF5_C_${HDF5_PKG_TYPE}_LIBRARY} ${HDF5_CXX_${HDF5_PKG_TYPE}_LIBRARY})
MESSAGE(STATUS "  HDF5 modules:       ${HDF5_LIBS}...")
LIST(APPEND MODULE_NAMES ${HDF5_LIBS})

IF(NOT HDF5_USE_STATIC_LIBRARIES)
  ADD_DEFINITIONS(-D_HDF5USEDLL_)
ENDIF(NOT HDF5_USE_STATIC_LIBRARIES)

IF(HDF5_LIB_TYPE)
  ADD_DEFINITIONS(-DH5_BUILT_AS_DYNAMIC_LIB)
ENDIF(HDF5_LIB_TYPE)

GET_PACKAGE_INFO("${MODULE_NAMES}" MODULE_INCLUDES MODULE_INSTALLER)
MESSAGE(STATUS "")

###################################################################################################
##### Define build output.                                                                    #####
###################################################################################################

# Set header directories.
INCLUDE_DIRECTORIES(
  ${INCLUDE_DIR}
  ${TXTRZ_ANALYSIS_INCLUDE_DIRS}
  ${MODULE_INCLUDES}
)

FILE(GLOB SRC_FILES
  ${SOURCES_DIR}/*.cpp
  ${INCLUDE_DIR}/*.hpp
  ${PROJECT_SOURCE_DIR}/*.h
  ${PROJECT_SOURCE_DIR}/*.cpp
)

# Make the project a shared library.
ADD_LIBRARY(${PROJECT_NAME} SHARED ${SRC_FILES})

# Append "_d" to artifact names for debug builds.
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX ".Dbg")

# Setup target dependencies.
TARGET_LINK_LIBRARIES(${PROJECT_NAME} Texturize.Analysis ${MODULE_NAMES})

# Setup target includes.
TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} 
  PUBLIC ${INCLUDE_DIR} $<INSTALL_INTERFACE:include>
  PRIVATE ${SOURCES_DIR}
)

###################################################################################################
##### Define installer image.                                                                 #####
###################################################################################################
INSTALL(TARGETS ${PROJECT_NAME} EXPORT ${CONFIG_NAME}
  ARCHIVE DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
  LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
  RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
)

# Install headers.
INSTALL(DIRECTORY ${INCLUDE_DIR} DESTINATION ${CMAKE_INSTALL_PREFIX})

# Install 3rd party assemblies.
FOREACH(_MODULE ${MODULE_NAMES})
  GET_PROPERTY(${_MODULE}_IMPORTED_LOCATION TARGET ${_MODULE} PROPERTY IMPORTED_LOCATION)

  IF(EXISTS "${${_MODULE}_IMPORTED_LOCATION}")
    INSTALL(FILES "${${_MODULE}_IMPORTED_LOCATION}" DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
  ENDIF(EXISTS "${${_MODULE}_IMPORTED_LOCATION}")
  
  GET_PROPERTY(${_MODULE}_IMPORTED_LOCATION TARGET ${_MODULE} PROPERTY IMPORTED_LOCATION_DEBUG)
  IF(EXISTS "${${_MODULE}_IMPORTED_LOCATION}")
    INSTALL(FILES "${${_MODULE}_IMPORTED_LOCATION}" CONFIGURATIONS "Debug" DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
  ENDIF(EXISTS "${${_MODULE}_IMPORTED_LOCATION}")
  
  GET_PROPERTY(${_MODULE}_IMPORTED_LOCATION TARGET ${_MODULE} PROPERTY IMPORTED_LOCATION_RELEASE)
  IF(EXISTS "${${_MODULE}_IMPORTED_LOCATION}")
    INSTALL(FILES "${${_MODULE}_IMPORTED_LOCATION}" CONFIGURATIONS "Release|RelWithDebInfo|MinSizeRel" DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
  ENDIF(EXISTS "${${_MODULE}_IMPORTED_LOCATION}")
ENDFOREACH(_MODULE "${MODULE_NAMES}")

###################################################################################################
##### Define export package.                                                                  #####
###################################################################################################
EXPORT(TARGETS ${PROJECT_NAME} FILE ${CONFIG_NAME}.cmake)

# Store the include directories.
SET(TXTRZ_CODECS_INCLUDE_DIRS 
  ${INCLUDE_DIR}
  ${TXTRZ_ANALYSIS_INCLUDE_DIRS}
  ${MODULE_INCLUDES}
  CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE
)

# Cleanup.
UNSET(MODULE_NAMES)
UNSET(MODULE_INSTALLER)
UNSET(MODULE_INCLUDES)