###################################################################################################
#####                                                                                         #####
##### Core library, defining common types, like exports, exceptions and so on.                #####
#####                                                                                         #####
###################################################################################################

CMAKE_MINIMUM_REQUIRED(VERSION 3.12 FATAL_ERROR)
SET(PROJECT_NAME Texturize.Core)
PROJECT(${PROJECT_NAME} LANGUAGES CXX VERSION ${Texturize_VERSION})
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
##### Version info configuration.                                                             #####
###################################################################################################

MESSAGE(STATUS "  Configuring version.hpp...")
MESSAGE(STATUS "")

configure_file (
  "${INCLUDE_DIR}/version.hpp.template"
  "${INCLUDE_DIR}/version.hpp"
)

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
##### Define build output.                                                                    #####
###################################################################################################

# Set header directory.
INCLUDE_DIRECTORIES(INCLUDE_DIR)

# Lookup source files.
FILE(GLOB SRC_FILES
  ${SOURCES_DIR}/*.cpp
  ${INCLUDE_DIR}/*.hpp
  ${INCLUDE_DIR}/errors.mc
  ${PROJECT_SOURCE_DIR}/*.h
  ${PROJECT_SOURCE_DIR}/*.cpp
)

# Make the project a shared library.
ADD_LIBRARY(${PROJECT_NAME} SHARED ${SRC_FILES})

# Append "_d" to artifact names for debug builds.
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX ".Dbg")

# Setup target includes.
TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} 
  PUBLIC ${INCLUDE_DIR} $<INSTALL_INTERFACE:include>
  PRIVATE ${SOURCES_DIR}
)

###################################################################################################
##### Define installer image.                                                                 #####
###################################################################################################

# Install artifacts.
INSTALL(TARGETS ${PROJECT_NAME} EXPORT ${CONFIG_NAME}
  ARCHIVE  DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
  LIBRARY  DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
  RUNTIME  DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
)

# Install headers
INSTALL(DIRECTORY ${INCLUDE_DIR} DESTINATION ${CMAKE_INSTALL_PREFIX} FILES_MATCHING PATTERN "*.hpp")

###################################################################################################
##### Define export package.                                                                  #####
###################################################################################################

# Export configuration.
EXPORT(TARGETS ${PROJECT_NAME} FILE "${CONFIG_NAME}.cmake")

# Store the include directories.
SET(TXTRZ_CORE_INCLUDE_DIRS ${INCLUDE_DIR} CACHE INTERNAL "${PROJECT_NAME}: Include directory" FORCE)