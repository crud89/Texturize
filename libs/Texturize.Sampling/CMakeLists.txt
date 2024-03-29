###################################################################################################
#####                                                                                         #####
##### Sampling library, containing the synthesis workflow interface and related types.        #####
#####                                                                                         #####
###################################################################################################

CMAKE_MINIMUM_REQUIRED(VERSION 3.12 FATAL_ERROR)
SET(PROJECT_NAME Texturize.Sampling)
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
##### Build type specific preprocessor definitions.                                           #####
###################################################################################################

ADD_DEFINITIONS(
  -DTEXTURIZE_EXPORTS 
  -D_WINDOWS 
  -D_USRDLL
  -DUNICODE 
  -D_UNICODE
)

ADD_DEFINITIONS(-D_USE_MATH_DEFINES)

# This disables the warning STL4009 about std::allocator<void> beeing deprecated for now. Not sure
# if this warning is actually telling the truth here, for now just disable it.
ADD_DEFINITIONS(-D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)

###################################################################################################
##### Define build output.                                                                    #####
###################################################################################################

# Set header directories.
INCLUDE_DIRECTORIES(
  ${INCLUDE_DIR}
  ${TXTRZ_ANALYSIS_INCLUDE_DIRS}
)

FILE(GLOB SRC_FILES
  ${SOURCES_DIR}/*.cpp
  ${INCLUDE_DIR}/*.hpp
  ${PROJECT_SOURCE_DIR}/*.h
  ${PROJECT_SOURCE_DIR}/*.cpp
)

# Make the project a shared library.
ADD_LIBRARY(${PROJECT_NAME} SHARED ${SRC_FILES})

# Setup target dependencies.
TARGET_LINK_LIBRARIES(${PROJECT_NAME} Texturize.Analysis)

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
INSTALL(TARGETS ${PROJECT_NAME} EXPORT ${CONFIG_NAME}
  ARCHIVE DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
  LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
  RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
)

# Install headers.
INSTALL(DIRECTORY ${INCLUDE_DIR} DESTINATION ${CMAKE_INSTALL_PREFIX})

###################################################################################################
##### Define export package.                                                                  #####
###################################################################################################
EXPORT(TARGETS ${PROJECT_NAME} FILE ${CONFIG_NAME}.cmake)

# Store the include directories.
SET(TXTRZ_SAMPLING_INCLUDE_DIRS 
  ${INCLUDE_DIR} 
  ${TXTRZ_ANALYSIS_INCLUDE_DIRS}
  CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE
)