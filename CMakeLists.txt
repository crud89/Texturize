cmake_minimum_required(VERSION 3.12)
project(Texturize)

set (Texturize_VERSION_MAJOR 1)
set (Texturize_VERSION_MINOR 0)

if (NOT DEFINED CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install)
endif()

message("Installing to ${CMAKE_INSTALL_PREFIX}")

set(CMAKE_INSTALL_INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_INSTALL_PREFIX}/bin/")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_INSTALL_PREFIX}/bin/")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_INSTALL_PREFIX}/lib/")
set(CMAKE_EXECUTABLE_OUTPUT_DIRECTORY "${CMAKE_INSTALL_PREFIX}/bin/")
set(CMAKE_PDB_OUTPUT_DIRECTORY "${CMAKE_INSTALL_PREFIX}/bin/")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/modules")

if (OpenCV_FOUND)
    message("Importing OpenCV from '${OpenCV_DIR}'...")
else()
    set(OpenCV_DIR "${CMAKE_MODULE_PATH}/opencv")
    message("No OpenCV directory provided, defaulting to '${OpenCV_DIR}'...")
endif()

if (OpenEXR_FOUND)
    message("Importing OpenEXR from '${OPENEXR_LOCATION}'...")
    set(ILMBASE_LOCATION ${OPENEXR_LOCATION})
else()
    set(OPENEXR_LOCATION "${CMAKE_MODULE_PATH}")
    set(ILMBASE_LOCATION "${CMAKE_MODULE_PATH}")
    message("No OpenEXR directory provided, defaulting to '${OPENEXR_LOCATION}'...")
endif()

if (TBB_FOUND)
    message("Importing tbb from '${TBB_DIR}'...")
else()
    set(TBB_DIR "${CMAKE_MODULE_PATH}/tbb")
    message("No tbb directory provided, defaulting to '${TBB_DIR}'...")
endif()

if (HDF5_FOUND)
    message("Importing hdf5 from '${HDF5_ROOT}'...")
else()
    set(HDF5_ROOT "${CMAKE_MODULE_PATH}/hdf5/cmake")
    message("No hdf5 directory provided, defaulting to '${HDF5_ROOT}'...")
endif()

add_subdirectory(Texturize.Core)
add_subdirectory(Texturize.Codecs)
add_subdirectory(Texturize.Analysis)
add_subdirectory(Texturize.Sampling)
add_subdirectory(Texturize.Codecs.EXR)

add_subdirectory(Texturize.Sandbox)

add_dependencies(Texturize.Codecs 		        Texturize.Core)
add_dependencies(Texturize.Analysis 		    Texturize.Core)
add_dependencies(Texturize.Sampling 		    Texturize.Core)
add_dependencies(Texturize.Codecs		        Texturize.Analysis)
add_dependencies(Texturize.Codecs.EXR 		    Texturize.Core)
add_dependencies(Texturize.Codecs.EXR 		    Texturize.Codecs)
add_dependencies(Texturize.Sampling 		    Texturize.Analysis)

add_dependencies(Texturize.Sandbox 		        Texturize.Core)
add_dependencies(Texturize.Sandbox 		        Texturize.Codecs)
add_dependencies(Texturize.Sandbox   		    Texturize.Codecs.EXR)
add_dependencies(Texturize.Sandbox 		        Texturize.Analysis)
add_dependencies(Texturize.Sandbox 		        Texturize.Sampling)