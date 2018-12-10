cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
set(PROJECT_NAME Texturize.Sampling)
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

# Finding packages from <ModuleName>_ROOT variables first.
cmake_policy(SET CMP0074 NEW)

include_directories(
  include
  ${TXTRZ_ANALYSIS_INCLUDE_DIRS}
)

file(GLOB SRC_FILES
    ${SOURCES_DIR}/*.cpp
    ${INCLUDE_DIR}/*.hpp
    ${PROJECT_DIR}/*.h
    ${PROJECT_DIR}/*.cpp
)

add_definitions(-D_USE_MATH_DEFINES)

add_library(${PROJECT_NAME} SHARED
   ${SRC_FILES}
)

target_link_libraries(${PROJECT_NAME} Texturize.Analysis ${OpenCV_LIBS})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${PROJECT_DIR}/include>
    $<INSTALL_INTERFACE:include>
    PRIVATE ${SOURCES_DIR})

install(TARGETS ${PROJECT_NAME} EXPORT ${CONFIG_NAME}
    ARCHIVE  DESTINATION ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE}
    LIBRARY  DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE}
    RUNTIME  DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE})

export(TARGETS ${PROJECT_NAME} FILE ${CONFIG_NAME}.cmake)

install(DIRECTORY ${INCLUDE_DIR} DESTINATION ${CMAKE_INSTALL_PREFIX})

set(TXTRZ_SAMPLING_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include 
                                ${TXTRZ_ANALYSIS_INCLUDE_DIRS}
  CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE
)