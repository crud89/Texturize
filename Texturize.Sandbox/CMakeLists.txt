###################################################################################################
#####                                                                                         #####
##### Sandbox application to play around with the framework.                                  #####
#####                                                                                         #####
###################################################################################################

cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
set(PROJECT_NAME Texturize.Sandbox)
project(${PROJECT_NAME} CXX)
set(CONFIG_NAME ${PROJECT_NAME}Config)

MESSAGE(STATUS "---------------------------------------------------------------------------------------------------")
MESSAGE(STATUS "")
MESSAGE(STATUS "Setting up project: ${PROJECT_NAME}...")

# Specify source folders, relative to the project sources directory.
SET(INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include")
SET(SOURCES_DIR "${PROJECT_SOURCE_DIR}/src")

MESSAGE(STATUS "  Include directory:    ${INCLUDE_DIR}")
MESSAGE(STATUS "  Source directory:     ${SOURCES_DIR}")
MESSAGE(STATUS "")

# Build type specific preprocessor definitions.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_definitions(
    -DTEXTURIZE_EXPORTS 
    -D_DEBUG 
    -D_WINDOWS 
    -D_USRDLL 
    -DTEXTURIZECORE_EXPORTS 
    -DUNICODE
    -D_UNICODE
  )
else()
  add_definitions(
    -DTEXTURIZE_EXPORTS 
    -D_WINDOWS 
    -D_USRDLL 
    -DTEXTURIZECORE_EXPORTS 
    -DUNICODE
    -D_UNICODE
  )
endif()

add_definitions(-D_USE_MATH_DEFINES)

# Finding packages from <ModuleName>_ROOT variables first.
cmake_policy(SET CMP0074 NEW)

# Set header directories.
include_directories(
  ${INCLUDE_DIR}
  ${TXTRZ_SAMPLING_INCLUDE_DIRS}
  ${TXTRZ_CODECS_INCLUDE_DIRS}
  ${TXTRZ_CODECS_EXR_INCLUDE_DIRS}
)

file(GLOB SRC_FILES
    ${SOURCES_DIR}/*.cpp
    ${INCLUDE_DIR}/*.hpp
    ${PROJECT_DIR}/*.h
    ${PROJECT_DIR}/*.cpp
)

# Make the project an executable.
add_executable(${PROJECT_NAME} Texturize.Sandbox.cpp ${SRC_FILES})

# Setup target dependencies.
target_link_libraries(${PROJECT_NAME} Texturize.Sampling Texturize.Codecs Texturize.Codecs.EXR ${OpenCV_LIBS})

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

# Install ressources.
install(
    CODE "file( GLOB MODEL_FILES \"${CMAKE_SOURCE_DIR}/models\" )"
    CODE "file( INSTALL \${MODEL_FILES} DESTINATION \"${CMAKE_INSTALL_PREFIX}\" )"
    CODE "file( INSTALL \"${CMAKE_SOURCE_DIR}/Texturize.props\" DESTINATION \"${CMAKE_INSTALL_PREFIX}\" )"
)
# Export configuration.
export(TARGETS ${PROJECT_NAME} FILE ${CONFIG_NAME}.cmake)