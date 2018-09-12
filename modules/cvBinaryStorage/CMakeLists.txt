CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

SET(PROJECT_NAME "binstorage")
PROJECT("${PROJECT_NAME}")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# add opencv package to the project
FIND_PACKAGE( OpenCV REQUIRED )

FIND_PACKAGE(HDF5 QUIET COMPONENTS CXX)

IF                          (HDF5_FOUND)
  INCLUDE_DIRECTORIES       (SYSTEM ${HDF5_CXX_INCLUDE_DIR})
  ADD_LIBRARY               (hdf5 STATIC IMPORTED)
  SET_TARGET_PROPERTIES     (hdf5 PROPERTIES IMPORTED_LOCATION ${HDF5_hdf5_LIBRARY})
  ADD_LIBRARY               (hdf5_cpp STATIC IMPORTED)
  SET_TARGET_PROPERTIES     (hdf5_cpp PROPERTIES IMPORTED_LOCATION ${HDF5_hdf5_cpp_LIBRARY})
  SET                       (HDF5_libraries     hdf5 hdf5_cpp)
ENDIF                       (HDF5_FOUND)

INCLUDE_DIRECTORIES( ${OpenCV_INCLUDE_DIRS} )
MESSAGE(STATUS "Project Name: ${PROJECT_NAME}")
MESSAGE(STATUS "    OpenCV version: ${OpenCV_VERSION} ")
MESSAGE(STATUS "    libraries: ${OpenCV_LIBS}")
MESSAGE(STATUS "    include path: ${OpenCV_INCLUDE_DIRS}")

FILE(GLOB files
  "*.h"
  "*.hpp"
  "*.cpp"
)
FILE(GLOB hidden
	".*"
)
FILE(GLOB resources
  "*.*"
)

ADD_EXECUTABLE(${PROJECT_NAME} ${files} )
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${OpenCV_LIBS}  )
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${HDF5_libraries} )

LIST(REMOVE_ITEM resources ${files} ${hidden} "${CMAKE_SOURCE_DIR}/CMakeLists.txt")
FILE(COPY ${resources} DESTINATION "Debug")
FILE(COPY ${resources} DESTINATION "Release")
