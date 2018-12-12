cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
set(PROJECT_NAME Texturize.Analysis)
project(${PROJECT_NAME})
set(CONFIG_NAME ${PROJECT_NAME}Config)

# Folders files
set(INCLUDE_DIR include)
set(SOURCES_DIR src)
set(PROJECT_DIR )

project(${PROJECT_NAME} CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
  message(STATUS "Build type not specified: Use Release by default.")
endif(NOT CMAKE_BUILD_TYPE)

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

if(MSVC)
   set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /W3 /MD /Od /EHsc")
   set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /W3 /GL /Od /Oi /Gy /EHsc")
endif(MSVC)

if(NOT MSVC)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
   if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
       set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
   endif()
endif(NOT MSVC)

# Find modules.
find_package(OpenCV REQUIRED core imgproc ximgproc imgcodecs highgui features2d calib3d flann)
find_package(TBB REQUIRED tbb)

if (OpenCV_FOUND)
  message("Linking ${PROJECT_NAME} to ${OpenCV_LIBS}...")
else()
  message(FATAL_ERROR "Required OpenCV modules have not been found.")
endif()

if (TBB_FOUND)
  message("Linking ${PROJECT_NAME} to ${TBB_IMPORTED_TARGETS}...")
else()
  message(FATAL_ERROR "Required tbb modules have not been found.")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
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
else()
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
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  get_target_property(TBB_DLLS ${TBB_IMPORTED_TARGETS} IMPORTED_LOCATION_DEBUG)
else()
  get_target_property(TBB_DLLS ${TBB_IMPORTED_TARGETS} IMPORTED_LOCATION_RELEASE)
endif()

include_directories(
  include
  ${TXTRZ_CORE_INCLUDE_DIRS}
  ${OpenCV_INCLUDE_DIRS}
  ${TBB_INCLUDE_DIRS}
)

file(GLOB SRC_FILES
    ${SOURCES_DIR}/*.cpp
    ${INCLUDE_DIR}/*.hpp
    ${PROJECT_DIR}/*.h
    ${PROJECT_DIR}/*.cpp
)

add_library(${PROJECT_NAME} SHARED
   ${SRC_FILES}
)

#foreach(OpenCV_DLL ${OpenCV_DLLS})
#  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#      COMMAND ${CMAKE_COMMAND} -E copy_if_different
#          "${OpenCV_DIR}/bin/${CMAKE_BUILD_TYPE}/${OpenCV_DLL}"
#          "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_BUILD_TYPE}/${OpenCV_DLL}")
#endforeach()

#foreach(TBB_DLL ${TBB_DLLS})
#  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#      COMMAND ${CMAKE_COMMAND} -E copy_if_different
#          "${TBB_DLL}"
#          "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_BUILD_TYPE}/")
#endforeach()

target_link_libraries(${PROJECT_NAME} Texturize.Core 
    ${OpenCV_LIBS} 
    ${TBB_IMPORTED_TARGETS})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${PROJECT_DIR}/include
    $<INSTALL_INTERFACE:include>
    PRIVATE ${SOURCES_DIR})

install(TARGETS ${PROJECT_NAME} EXPORT ${CONFIG_NAME}
    ARCHIVE  DESTINATION ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE}
    LIBRARY  DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE}
    RUNTIME  DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE})

export(TARGETS ${PROJECT_NAME} FILE ${CONFIG_NAME}.cmake)

install(DIRECTORY ${INCLUDE_DIR} DESTINATION ${CMAKE_INSTALL_PREFIX})

set(TXTRZ_ANALYSIS_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include 
                                ${TXTRZ_CORE_INCLUDE_DIRS}
                                ${OpenCV_INCLUDE_DIRS}
  CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE
)