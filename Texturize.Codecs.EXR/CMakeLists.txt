###################################################################################################
#####                                                                                         #####
##### EXR codec to store images with arbitrary channel and pixel depth.                       #####
#####                                                                                         #####
###################################################################################################

CMAKE_MINIMUM_REQUIRED(VERSION 3.12 FATAL_ERROR)
SET(PROJECT_NAME Texturize.Codecs.EXR)
PROJECT(${PROJECT_NAME} CXX)
SET(CONFIG_NAME ${PROJECT_NAME}Config)

MESSAGE(STATUS "---------------------------------------------------------------------------------------------------")
MESSAGE(STATUS "")
MESSAGE(STATUS "Setting up project: ${PROJECT_NAME}...")

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

IF(BUILD_TYPE STREQUAL DEBUG)
  ADD_DEFINITIONS(-D_DEBUG)
ENDIF(BUILD_TYPE STREQUAL DEBUG)

###################################################################################################
##### Resolve package dependencies.                                                           #####
###################################################################################################

# Finding packages from <ModuleName>_ROOT variables first.
CMAKE_POLICY(SET CMP0074 NEW)
SET(MODULE_NAMES)
SET(MODULE_INSTALLER)
SET(MODULE_INCLUDES)

###################################################################################################
##### Find OpenEXR.                                                                           #####
###################################################################################################
FIND_PACKAGE(OpenEXR REQUIRED)

IF(OPENEXR_FOUND)
  MESSAGE(STATUS "  OpenEXR modules:      ${OPENEXR_LIBS}")
  LIST(APPEND MODULE_NAMES ${OPENEXR_LIBS})
ELSE(OPENEXR_FOUND)
  MESSAGE(FATAL_ERROR "  ERROR: Required OpenEXR modules have not been found.")
ENDIF(OPENEXR_FOUND)

GET_PACKAGE_INFO("${MODULE_NAMES}" MODULE_INCLUDES MODULE_INSTALLER)
MESSAGE(STATUS "")

###################################################################################################
##### Define build output.                                                                    #####
###################################################################################################

# Set header directories.
INCLUDE_DIRECTORIES(
  ${INCLUDE_DIR}
  ${TXTRZ_CODECS_INCLUDE_DIRS}
  ${MODULE_INCLUDES}
)

FILE(GLOB SRC_FILES
  ${SOURCES_DIR}/*.cpp
  ${SOURCES_DIR}/Stream.hpp
  ${INCLUDE_DIR}/*.hpp
  ${PROJECT_SOURCE_DIR}/*.h
  ${PROJECT_SOURCE_DIR}/*.cpp
)

# Make the project a shared library.
ADD_LIBRARY(${PROJECT_NAME} SHARED ${SRC_FILES})

# Setup target dependencies.
TARGET_LINK_LIBRARIES(${PROJECT_NAME} Texturize.Codecs ${MODULE_NAMES})

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
INSTALL(FILES ${MODULE_INSTALLER} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")

###################################################################################################
##### Define export package.                                                                  #####
###################################################################################################
EXPORT(TARGETS ${PROJECT_NAME} FILE ${CONFIG_NAME}.cmake)

# Store the include directories.
SET(TXTRZ_CODECS_EXR_INCLUDE_DIRS 
  ${INCLUDE_DIR}
  ${TXTRZ_CODECS_INCLUDE_DIRS}
  ${MODULE_INCLUDES}
  CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE
)

# Cleanup.
UNSET(MODULE_NAMES)
UNSET(MODULE_INSTALLER)
UNSET(MODULE_INCLUDES)