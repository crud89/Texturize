###################################################################################################
#####                                                                                         #####
##### Core library, defining common types, like exports, exceptions and so on.                #####
#####                                                                                         #####
###################################################################################################

cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
SET(PROJECT_NAME Texturize.Core)
project(${PROJECT_NAME} CXX)
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

# Configure
MESSAGE(STATUS "  Configuring version.hpp...")
MESSAGE(STATUS "")

configure_file (
  "${INCLUDE_DIR}/version.hpp.template"
  "${INCLUDE_DIR}/version.hpp"
)

# Build type specific preprocessor definitions.
IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_definitions(
   -DTEXTURIZE_EXPORTS 
   -D_DEBUG 
   -D_WINDOWS 
   -D_USRDLL 
   -DTEXTURIZECORE_EXPORTS 
   -DUNICODE
   -D_UNICODE
  )
ELSE()
  add_definitions(
   -DTEXTURIZE_EXPORTS
   -D_WINDOWS 
   -D_USRDLL 
   -DTEXTURIZECORE_EXPORTS 
   -DUNICODE
   -D_UNICODE
  )
ENDIF()

# Set header directory.
include_directories(INCLUDE_DIR)

# Lookup source files.
file(GLOB SRC_FILES
    ${SOURCES_DIR}/*.cpp
    ${INCLUDE_DIR}/*.hpp
    ${INCLUDE_DIR}/errors.mc
    ${PROJECT_SOURCE_DIR}/*.h
    ${PROJECT_SOURCE_DIR}/*.cpp
)

# Make the project a shared library.
add_library(${PROJECT_NAME} SHARED
   ${SRC_FILES}
)

# Setup target includes.
target_include_directories(${PROJECT_NAME} PUBLIC
    ${INCLUDE_DIR}
    $<INSTALL_INTERFACE:include>
    PRIVATE ${SOURCES_DIR})

# Install artifacts.
install(TARGETS ${PROJECT_NAME} EXPORT ${CONFIG_NAME}
    ARCHIVE  DESTINATION ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE}
    LIBRARY  DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE}
    RUNTIME  DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE})

# Install headers
install(DIRECTORY ${INCLUDE_DIR} DESTINATION ${CMAKE_INSTALL_PREFIX} FILES_MATCHING PATTERN "*.hpp")

# Export configuration.
export(TARGETS ${PROJECT_NAME} FILE "${CONFIG_NAME}.cmake")

# Store the include directories.
SET(TXTRZ_CORE_INCLUDE_DIRS ${INCLUDE_DIR} CACHE INTERNAL "${PROJECT_NAME}: Include directory" FORCE)