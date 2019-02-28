# adapted from FindOpenEXR.cmake in Pixar's USD distro.
# 
# The original license is as follows:
#
# Copyright 2016 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.
#

###################################################################################################
##### Lookup the installation directory of OpenEXR.                                           #####
###################################################################################################
FIND_PATH(OPENEXR_INCLUDE_DIR "OpenEXR/ImfHeader.h"
  HINTS "${OPENEXR_LOCATION}" "$ENV{OPENEXR_LOCATION}" "$ENV{OPENEXR_ROOT}"
  PATH_SUFFIXES "include/"
  NO_DEFAULT_PATH
  NO_SYSTEM_ENVIRONMENT_PATH
  DOC "OpenEXR installation directory"
)

# In case the directory has not been found, print a warning.
IF(NOT EXISTS ${OPENEXR_INCLUDE_DIR})
  MESSAGE(WARNING "  WARNING: OpenEXR headers have not been found.")
ELSE(NOT EXISTS ${OPENEXR_INCLUDE_DIR})
  SET(OPENEXR_CONFIG_FILE "${OPENEXR_INCLUDE_DIR}/OpenEXR/OpenEXRConfig.h")

  # The configuration header should be existing under this location, if the directory is valid. If so,
  # extract the framework version from it.
  IF(EXISTS ${OPENEXR_CONFIG_FILE})
    FILE(STRINGS ${OPENEXR_CONFIG_FILE} TMP REGEX "#define OPENEXR_VERSION_STRING.*$")
    STRING(REGEX MATCHALL "[0-9.]+" OPENEXR_VERSION ${TMP})
    FILE(STRINGS ${OPENEXR_CONFIG_FILE} TMP REGEX "#define OPENEXR_VERSION_MAJOR.*$")
    STRING(REGEX MATCHALL "[0-9]" OPENEXR_MAJOR_VERSION ${TMP})
    FILE(STRINGS ${OPENEXR_CONFIG_FILE} TMP REGEX "#define OPENEXR_VERSION_MINOR.*$")
    STRING(REGEX MATCHALL "[0-9]" OPENEXR_MINOR_VERSION ${TMP})
  ENDIF(EXISTS ${OPENEXR_CONFIG_FILE})
ENDIF(NOT EXISTS ${OPENEXR_INCLUDE_DIR})

###################################################################################################
##### Lookup the existing libraries for OpenEXR.                                              #####
###################################################################################################
FOREACH(OPENEXR_LIB
  Half
  Imath
  Iex
  IexMath
  IlmThread
  IlmImf 
  IlmImfUtil
)
  # Lookup the library by its name.
  FIND_LIBRARY(OPENEXR_${OPENEXR_LIB}_LIBRARY 
    NAMES ${OPENEXR_LIB}-${OPENEXR_MAJOR_VERSION}_${OPENEXR_MINOR_VERSION}  ${OPENEXR_LIB}
    HINTS "${OPENEXR_LOCATION}" "$ENV{OPENEXR_LOCATION}"
    PATH_SUFFIXES "lib/"
    NO_DEFAULT_PATH
    NO_SYSTEM_ENVIRONMENT_PATH
    DOC "OPENEXR '${OPENEXR_LIB}' library path."
  )

  # If the library has been found, remember it for later processing.
  IF(OPENEXR_${OPENEXR_LIB}_LIBRARY)
    LIST(APPEND OPENEXR_LIBRARIES ${OPENEXR_${OPENEXR_LIB}_LIBRARY})
  ENDIF(OPENEXR_${OPENEXR_LIB}_LIBRARY)

  # Lookup the debug version.
  FIND_LIBRARY(OPENEXR_${OPENEXR_LIB}_DEBUG_LIBRARY
    NAMES ${OPENEXR_LIB}-${OPENEXR_MAJOR_VERSION}_${OPENEXR_MINOR_VERSION}_d  ${OPENEXR_LIB}_d
    HINTS "${OPENEXR_LOCATION}" "$ENV{OPENEXR_LOCATION}"
    PATH_SUFFIXES "lib/" "debug/lib/"
    NO_DEFAULT_PATH
    NO_SYSTEM_ENVIRONMENT_PATH
    DOC "OPENEXR's ${OPENEXR_LIB} debug library path"
  )

  IF(OPENEXR_${OPENEXR_LIB}_DEBUG_LIBRARY)
    LIST(APPEND OPENEXR_LIBRARIES ${OPENEXR_${OPENEXR_LIB}_DEBUG_LIBRARY})
  ENDIF(OPENEXR_${OPENEXR_LIB}_DEBUG_LIBRARY)
ENDFOREACH(OPENEXR_LIB)

# So #include <half.h> works
LIST(APPEND OPENEXR_INCLUDE_DIRS "${OPENEXR_INCLUDE_DIR}")
LIST(APPEND OPENEXR_INCLUDE_DIRS "${OPENEXR_INCLUDE_DIR}/OpenEXR")

# Validate the libraries.
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OpenEXR
  REQUIRED_VARS     OPENEXR_INCLUDE_DIRS OPENEXR_LIBRARIES
  VERSION_VAR       OPENEXR_VERSION
)

###################################################################################################
##### Setup the target properties for the libraries.                                          #####
###################################################################################################
FOREACH(OPENEXR_LIB
  Half
  Imath
  Iex
  IexMath
  IlmThread
  IlmImf 
  IlmImfUtil
)
  # Set properties for the shared library.
  IF(OPENEXR_${OPENEXR_LIB}_LIBRARY OR OPENEXR_${OPENEXR_LIB}_DEBUG_LIBRARY)
    ADD_LIBRARY(OpenEXR::${OPENEXR_LIB} SHARED IMPORTED)
    SET_TARGET_PROPERTIES(OpenEXR::${OPENEXR_LIB} PROPERTIES IMPORTED ON)
    SET_TARGET_PROPERTIES(OpenEXR::${OPENEXR_LIB} PROPERTIES IMPORTED_IMPLIB_RELEASE ${OPENEXR_${OPENEXR_LIB}_LIBRARY})
    SET_TARGET_PROPERTIES(OpenEXR::${OPENEXR_LIB} PROPERTIES IMPORTED_IMPLIB_DEBUG ${OPENEXR_${OPENEXR_LIB}_DEBUG_LIBRARY})
    SET_TARGET_PROPERTIES(OpenEXR::${OPENEXR_LIB} PROPERTIES MAP_IMPORTED_CONFIG_RELWITHDEBINFO RELEASE)
    SET_PROPERTY(TARGET OpenEXR::${OPENEXR_LIB} APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${OPENEXR_INCLUDE_DIRS}")
    
    IF(WIN32)
      # Lookup the release and debug DLLs.
      FIND_FILE(OPENEXR_${OPENEXR_LIB}_LINK_LIBRARY 
        NAMES "${OPENEXR_LIB}-${OPENEXR_MAJOR_VERSION}_${OPENEXR_MINOR_VERSION}.dll" "${OPENEXR_LIB}.dll"
        HINTS "${OPENEXR_LOCATION}" "$ENV{OPENEXR_LOCATION}"
        PATH_SUFFIXES "bin/" "debug/bin/"
        NO_DEFAULT_PATH
        NO_SYSTEM_ENVIRONMENT_PATH
        DOC "OPENEXR '${OPENEXR_LIB}' link library path."
      )

      IF(OPENEXR_${OPENEXR_LIB}_LINK_LIBRARY)
        SET_TARGET_PROPERTIES(OpenEXR::${OPENEXR_LIB} PROPERTIES IMPORTED_LOCATION_RELEASE ${OPENEXR_${OPENEXR_LIB}_LINK_LIBRARY})
      ENDIF(OPENEXR_${OPENEXR_LIB}_LINK_LIBRARY)

      FIND_FILE(OPENEXR_${OPENEXR_LIB}_DEBUG_LINK_LIBRARY 
        NAMES "${OPENEXR_LIB}-${OPENEXR_MAJOR_VERSION}_${OPENEXR_MINOR_VERSION}_d.dll" "${OPENEXR_LIB}_d.dll"
        HINTS "${OPENEXR_LOCATION}" "$ENV{OPENEXR_LOCATION}"
        PATH_SUFFIXES "bin/" "debug/bin/"
        NO_DEFAULT_PATH
        NO_SYSTEM_ENVIRONMENT_PATH
        DOC "OPENEXR '${OPENEXR_LIB}' link library path."
      )

      IF(OPENEXR_${OPENEXR_LIB}_DEBUG_LINK_LIBRARY)
        SET_TARGET_PROPERTIES(OpenEXR::${OPENEXR_LIB} PROPERTIES IMPORTED_LOCATION_DEBUG ${OPENEXR_${OPENEXR_LIB}_DEBUG_LINK_LIBRARY})
      ENDIF(OPENEXR_${OPENEXR_LIB}_DEBUG_LINK_LIBRARY)
    ENDIF(WIN32)
    
    LIST(APPEND OPENEXR_LIBS OpenEXR::${OPENEXR_LIB})
  ELSE(OPENEXR_${OPENEXR_LIB}_LIBRARY)
    MESSAGE(WARNING "  ${OPENEXR_LIB} was not found.")
  ENDIF(OPENEXR_${OPENEXR_LIB}_LIBRARY)
ENDFOREACH(OPENEXR_LIB)
