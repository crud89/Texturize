###################################################################################################
#####                                                                                         #####
##### Analysis library, containing basic image processing types and algorithms.               #####
#####                                                                                         #####
###################################################################################################

cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
set(PROJECT_NAME Texturize.Analysis)
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

# Find modules.
find_package(OpenCV REQUIRED core imgproc ximgproc imgcodecs highgui features2d calib3d flann)
find_package(TBB REQUIRED tbb)

IF(OpenCV_FOUND)
  MESSAGE(STATUS "  OpenCV modules:       ${OpenCV_LIBS}...")
ELSE()
  MESSAGE(FATAL_ERROR "  ERROR: Required OpenCV modules have not been found.")
ENDIF()

if(TBB_FOUND)
  MESSAGE(STATUS "  TBB modules:          ${TBB_IMPORTED_TARGETS}")
ELSE()
  MESSAGE(FATAL_ERROR "  ERROR: Required TBB modules have not been found.")
ENDIF()

# Link to the right libraries.
IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
  list (APPEND OpenCV_DLLS 
        "opencv_core${OpenCV_VERSION_MAJOR}${OpenCV_VERSION_MINOR}${OpenCV_VERSION_PATCH}d.dll"
        "opencv_imgproc${OpenCV_VERSION_MAJOR}${OpenCV_VERSION_MINOR}${OpenCV_VERSION_PATCH}d.dll"
        "opencv_ximgproc${OpenCV_VERSION_MAJOR}${OpenCV_VERSION_MINOR}${OpenCV_VERSION_PATCH}d.dll"
        "opencv_imgcodecs${OpenCV_VERSION_MAJOR}${OpenCV_VERSION_MINOR}${OpenCV_VERSION_PATCH}d.dll"
        "opencv_highgui${OpenCV_VERSION_MAJOR}${OpenCV_VERSION_MINOR}${OpenCV_VERSION_PATCH}d.dll"
        "opencv_features2d${OpenCV_VERSION_MAJOR}${OpenCV_VERSION_MINOR}${OpenCV_VERSION_PATCH}d.dll"
        "opencv_calib3d${OpenCV_VERSION_MAJOR}${OpenCV_VERSION_MINOR}${OpenCV_VERSION_PATCH}d.dll"
        "opencv_flann${OpenCV_VERSION_MAJOR}${OpenCV_VERSION_MINOR}${OpenCV_VERSION_PATCH}d.dll"
  )
ELSE()
  list (APPEND OpenCV_DLLS  
        "opencv_core${OpenCV_VERSION_MAJOR}${OpenCV_VERSION_MINOR}${OpenCV_VERSION_PATCH}.dll"
        "opencv_imgproc${OpenCV_VERSION_MAJOR}${OpenCV_VERSION_MINOR}${OpenCV_VERSION_PATCH}.dll"
        "opencv_ximgproc${OpenCV_VERSION_MAJOR}${OpenCV_VERSION_MINOR}${OpenCV_VERSION_PATCH}.dll"
        "opencv_imgcodecs${OpenCV_VERSION_MAJOR}${OpenCV_VERSION_MINOR}${OpenCV_VERSION_PATCH}.dll"
        "opencv_highgui${OpenCV_VERSION_MAJOR}${OpenCV_VERSION_MINOR}${OpenCV_VERSION_PATCH}.dll"
        "opencv_features2d${OpenCV_VERSION_MAJOR}${OpenCV_VERSION_MINOR}${OpenCV_VERSION_PATCH}.dll"
        "opencv_calib3d${OpenCV_VERSION_MAJOR}${OpenCV_VERSION_MINOR}${OpenCV_VERSION_PATCH}.dll"
        "opencv_flann${OpenCV_VERSION_MAJOR}${OpenCV_VERSION_MINOR}${OpenCV_VERSION_PATCH}.dll"
  )
ENDIF()

IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
  get_target_property(TBB_DLLS ${TBB_IMPORTED_TARGETS} IMPORTED_LOCATION_DEBUG)
ELSE()
  get_target_property(TBB_DLLS ${TBB_IMPORTED_TARGETS} IMPORTED_LOCATION_RELEASE)
ENDIF()

# Set header directories.
include_directories(
  ${INCLUDE_DIR}
  ${TXTRZ_CORE_INCLUDE_DIRS}
  ${OpenCV_INCLUDE_DIRS}
  ${TBB_INCLUDE_DIRS}
)

file(GLOB SRC_FILES
    ${SOURCES_DIR}/*.cpp
    ${INCLUDE_DIR}/*.hpp
    ${PROJECT_SOURCE_DIR}/*.h
    ${PROJECT_SOURCE_DIR}/*.cpp
)

# Make the project a shared library.
add_library(${PROJECT_NAME} SHARED
   ${SRC_FILES}
)

# Copy dependency libraries post build.
FOREACH(OpenCV_DLL ${OpenCV_DLLS})
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${OpenCV_DIR}/${OpenCV_ARCH}/${OpenCV_RUNTIME}/bin/${OpenCV_DLL}"
          "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_BUILD_TYPE}/${OpenCV_DLL}")
ENDFOREACH()

FOREACH(TBB_DLL ${TBB_DLLS})
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${TBB_DLL}"
          "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_BUILD_TYPE}/")
ENDFOREACH()

# Setup target dependencies.
target_link_libraries(${PROJECT_NAME} Texturize.Core 
    ${OpenCV_LIBS} 
    ${TBB_IMPORTED_TARGETS})

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
SET(TXTRZ_ANALYSIS_INCLUDE_DIRS ${INCLUDE_DIR}
                                ${TXTRZ_CORE_INCLUDE_DIRS}
                                ${OpenCV_INCLUDE_DIRS}
  CACHE INTERNAL "${PROJECT_NAME}: Include directories" FORCE
)