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

# Set compiler flags
IF(MSVC)
  # Switch from Multi-Thread (/MT) to Multi-Thread Debug (/MTd) for debug builds.
  #SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
  #SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
  
  # NOTE: While the above should be correct for executables, there will be heap corruption errors
  #		  in debug mode, so we leave this on /MD(d) for now.
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

ADD_DEFINITIONS(-D_USE_MATH_DEFINES)

###################################################################################################
##### Define build output.                                                                    #####
###################################################################################################

# Set header directories.
INCLUDE_DIRECTORIES(
  ${INCLUDE_DIR}
  ${TXTRZ_SAMPLING_INCLUDE_DIRS}
  ${TXTRZ_CODECS_INCLUDE_DIRS}
  ${TXTRZ_CODECS_EXR_INCLUDE_DIRS}
)

FILE(GLOB SRC_FILES
  ${SOURCES_DIR}/*.cpp
  ${INCLUDE_DIR}/*.hpp
  ${PROJECT_DIR}/*.h
  ${PROJECT_DIR}/*.cpp
)

# Make the project an executable.
ADD_EXECUTABLE(${PROJECT_NAME} Texturize.Sandbox.cpp ${SRC_FILES})

# Append "_d" to artifact names for debug builds.
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX ".Dbg")

# Setup target dependencies.
TARGET_LINK_LIBRARIES(${PROJECT_NAME} Texturize.Sampling Texturize.Codecs.EXR)

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

# Install ressources.
INSTALL(
  CODE "file( GLOB MODEL_FILES \"${CMAKE_SOURCE_DIR}/models\" )"
  CODE "file( INSTALL \${MODEL_FILES} DESTINATION \"${CMAKE_INSTALL_PREFIX}\" )"
  CODE "file( INSTALL \"${CMAKE_SOURCE_DIR}/Texturize.props\" DESTINATION \"${CMAKE_INSTALL_PREFIX}\" )"
)

###################################################################################################
##### Define export package.                                                                  #####
###################################################################################################
EXPORT(TARGETS ${PROJECT_NAME} FILE ${CONFIG_NAME}.cmake)