###################################################################################################
#####                                                                                         #####
##### Default value definitions for CMake build options.                                      #####
#####                                                                                         #####
###################################################################################################

OPTION(BUILD_ZLIB				"Include zlib in the build. You may disable this option if you provide zlib on your own."		ON)
OPTION(WITH_TAPKEE				"Build advanced dimensionality reductors based on tapkee."										ON)

OPTION(BUILD_APP_SANDBOX		"Builds the sandbox app."																		OFF)
OPTION(BUILD_APP_FILTERMR8		"Builds MR8 filter bank app."																	ON)
OPTION(BUILD_APP_KMEANS			"Builds k-means clustering app."																ON)
OPTION(BUILD_APP_CONTROL_MAP	"Builds control map generation app."															ON)

IF("${OpenCV_DIR}" STREQUAL "")
  SET(OpenCV_DIR "${CMAKE_MODULE_PATH}/opencv")
ENDIF("${OpenCV_DIR}" STREQUAL "")

IF("${OPENEXR_LOCATION}" STREQUAL "")
  SET(OPENEXR_LOCATION "${CMAKE_MODULE_PATH}")
ENDIF("${OPENEXR_LOCATION}" STREQUAL "")

IF("${TBB_DIR}" STREQUAL "")
  SET(TBB_DIR "${CMAKE_MODULE_PATH}")
ENDIF("${TBB_DIR}" STREQUAL "")

IF("${HDF5_ROOT}" STREQUAL "")
  SET(HDF5_ROOT "${CMAKE_MODULE_PATH}")
ENDIF("${HDF5_ROOT}" STREQUAL "")

IF("${ZLIB_ROOT}" STREQUAL "")
  SET(ZLIB_ROOT "${CMAKE_MODULE_PATH}/zlib")
ENDIF("${ZLIB_ROOT}" STREQUAL "")

IF("${EIGEN3_ROOT}" STREQUAL "")
  SET(EIGEN3_ROOT "${CMAKE_MODULE_PATH}")
ENDIF("${EIGEN3_ROOT}" STREQUAL "")

IF("${TAPKEE_DIR}" STREQUAL "")
  SET(TAPKEE_DIR "${CMAKE_MODULE_PATH}")
ENDIF("${TAPKEE_DIR}" STREQUAL "")