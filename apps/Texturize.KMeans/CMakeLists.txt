###################################################################################################
#####                                                                                         #####
##### Clusters an n-dimensional sample using k means clustering.                              #####
#####                                                                                         #####
###################################################################################################

CMAKE_MINIMUM_REQUIRED(VERSION 3.12 FATAL_ERROR)
SET(PROJECT_NAME Texturize.KMeans)
PROJECT(${PROJECT_NAME} CXX)
SET(CONFIG_NAME ${PROJECT_NAME}Config)

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
  -DUNICODE 
  -D_UNICODE
)

ADD_DEFINITIONS(-D_USE_MATH_DEFINES)

###################################################################################################
##### Define build output.                                                                    #####
###################################################################################################

# Set header directories.
LIST(APPEND INCLUDE_DIR 
  ${TXTRZ_SAMPLING_INCLUDE_DIRS}
  ${TXTRZ_CODECS_INCLUDE_DIRS}
  ${TXTRZ_CODECS_EXR_INCLUDE_DIRS}
)

SET(MODULE_INCLUDES)

IF(WITH_TAPKEE)
  LIST(APPEND MODULE_INCLUDES ${TXTRZ_ADAPTERS_TAPKEE_INCLUDE_DIRS})
ENDIF(WITH_TAPKEE)

INCLUDE_DIRECTORIES(
  ${INCLUDE_DIR}
  SYSTEM ${MODULE_INCLUDES}
)

FILE(GLOB SRC_FILES
  ${SOURCES_DIR}/*.cpp
  ${INCLUDE_DIR}/*.hpp
  ${PROJECT_DIR}/*.h
  ${PROJECT_DIR}/*.cpp
)

# Make the project an executable.
ADD_EXECUTABLE(${PROJECT_NAME} Texturize.KMeans.cpp ${SRC_FILES})

# Append "_d" to artifact names for debug builds.
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX ".Dbg")

# Setup target dependencies.
LIST(APPEND LINK_LIBS Texturize.Sampling Texturize.Codecs.EXR)

IF(WITH_TAPKEE)
  LIST(APPEND LINK_LIBS Texturize.Adapters.Tapkee)
ENDIF(WITH_TAPKEE)

TARGET_LINK_LIBRARIES(${PROJECT_NAME} "${LINK_LIBS}")

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