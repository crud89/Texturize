###################################################################################################
#####                                                                                         #####
##### Codecs library, containing basic interfaces for asset codecs and persistence.           #####
#####                                                                                         #####
###################################################################################################

cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
set(PROJECT_NAME Texturize.Codecs)
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

# Finding packages from <ModuleName>_ROOT variables first.
cmake_policy(SET CMP0074 NEW)

# Find modules.
SET(HDF5_USE_STATIC_LIBRARIES OFF)
SET(LIB_TYPE SHARED)
STRING(TOLOWER ${LIB_TYPE} SEARCH_TYPE)

find_package(HDF5 NAMES hdf5 COMPONENTS C CXX ${SEARCH_TYPE})

IF (HDF5_FOUND)
  SET (LINK_LIBS ${LINK_LIBS} ${HDF5_C_${LIB_TYPE}_LIBRARY} ${HDF5_CXX_${LIB_TYPE}_LIBRARY})
  MESSAGE(STATUS "  HDF5 modules:       ${LINK_LIBS}...")
ELSE()
  MESSAGE(FATAL_ERROR "  ERROR: Required HDF5 modules have not been found.")
ENDIF()

# Link to the right libraries.
IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
  list (APPEND HDF5_DLLS 
        "hdf5_D.dll"
        "hdf5_cpp_D.dll"
  )
ELSE()
  list (APPEND HDF5_DLLS  
        "hdf5.dll"
        "hdf5_cpp.dll"
  )
ENDIF()

add_definitions(-DH5_BUILT_AS_DYNAMIC_LIB -D_HDF5USEDLL_)

# Set header directories.
include_directories(
  ${INCLUDE_DIR}
  ${TXTRZ_ANALYSIS_INCLUDE_DIRS}
  ${HDF5_INCLUDE_DIRS}
)

file(GLOB SRC_FILES
    ${SOURCES_DIR}/*.cpp
    ${INCLUDE_DIR}/*.hpp
    ${PROJECT_SOURCE_DIR}/*.h
    ${PROJECT_SOURCE_DIR}/*.cpp
)

# Make the project a shared library.
add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

# Copy dependency libraries post build.
FOREACH(HDF5_DLL ${HDF5_DLLS})
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${HDF5_ROOT}/../bin/${HDF5_DLL}"
          "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_BUILD_TYPE}/${HDF5_DLL}")
ENDFOREACH()

# Setup target dependencies.
target_link_libraries(${PROJECT_NAME} Texturize.Analysis ${LINK_LIBS})

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

# Install headers.
install(DIRECTORY ${INCLUDE_DIR} DESTINATION ${CMAKE_INSTALL_PREFIX})

# Export configuration.
export(TARGETS ${PROJECT_NAME} FILE ${CONFIG_NAME}.cmake)

# Store the include directories.
set(TXTRZ_CODECS_INCLUDE_DIRS ${INCLUDE_DIR}
                              ${TXTRZ_ANALYSIS_INCLUDE_DIRS}
                              ${HDF5_INCLUDE_DIRS}
  CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE
)


