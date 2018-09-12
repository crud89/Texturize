cmake_minimum_required(VERSION 3.6.2 FATAL_ERROR)
set(PROJECT_NAME Texturize.Codecs)
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

# Find modules.
set(HDF5_USE_STATIC_LIBRARIES OFF)
set (LIB_TYPE SHARED)
string(TOLOWER ${LIB_TYPE} SEARCH_TYPE)

find_package (HDF5 NAMES hdf5 COMPONENTS C CXX ${SEARCH_TYPE})

if (HDF5_FOUND)
  set (LINK_LIBS ${LINK_LIBS} ${HDF5_C_${LIB_TYPE}_LIBRARY} ${HDF5_CXX_${LIB_TYPE}_LIBRARY})
  message("Linking ${PROJECT_NAME} to ${LINK_LIBS}...")
else()
  message(FATAL_ERROR "Required hdf5 modules have not been found.")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  list (APPEND HDF5_DLLS 
        "hdf5_D.dll"
        "hdf5_cpp_D.dll"
  )
else()
  list (APPEND HDF5_DLLS  
        "hdf5.dll"
        "hdf5_cpp.dll"
  )
endif()

include_directories(
  include
  ${TXTRZ_ANALYSIS_INCLUDE_DIRS}
  ${HDF5_INCLUDE_DIRS}
  ${CMAKE_MODULE_PATH}/cvBinaryStorage/
)

file(GLOB SRC_FILES
    ${SOURCES_DIR}/*.cpp
    ${INCLUDE_DIR}/*.hpp
    ${PROJECT_DIR}/*.h
    ${PROJECT_DIR}/*.cpp
    ${CMAKE_MODULE_PATH}/cvBinaryStorage/ipersistence.cpp
)

add_definitions(-DH5_BUILT_AS_DYNAMIC_LIB -D_HDF5USEDLL_)

add_library(${PROJECT_NAME} SHARED
   ${SRC_FILES}
)

foreach(HDF5_DLL ${HDF5_DLLS})
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${HDF5_ROOT}/../bin/${HDF5_DLL}"
          "${CMAKE_INSTALL_PREFIX}/bin/${CMAKE_BUILD_TYPE}/${HDF5_DLL}")
endforeach()

target_link_libraries(${PROJECT_NAME} Texturize.Analysis ${LINK_LIBS})

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

set(TXTRZ_CODECS_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include 
                              ${TXTRZ_ANALYSIS_INCLUDE_DIRS}
                              ${HDF5_INCLUDE_DIRS}
                              ${CMAKE_MODULE_PATH}/cvBinaryStorage/
  CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE
)


