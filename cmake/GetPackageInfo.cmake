###################################################################################################
#####                                                                                         #####
##### Call this function after a package has been found by CMake's FIND_PACKAGE in order to   #####
##### extract the include directories and binary names for the package.                       #####
#####                                                                                         #####
##### Syntax example:                                                                         #####
##### > SET(MODULE_INCLUDES)                                                                  #####
##### > SET(MODULE_BINARIES)                                                                  #####
##### > LIST(APPEND MODULE_NAMES LibFoo LibBar)                                               #####
##### > GET_PACKAGE_INFO("${MODULE_NAMES}" MODULE_INCLUDES MODULE_BINARIES)                   #####
#####                                                                                         #####
##### In the example above, the MODULE_INCLUDES will contain a list of include directories,   #####
##### one for each module provided in MODULE_NAMES. MODULE_BINARIES will contain the imported #####
##### library locations.                                                                      #####
#####                                                                                         #####
###################################################################################################

FUNCTION(GET_PACKAGE_INFO ARG_MODULE_NAMES RET_INCLUDES RET_INSTALLER)
  SET(_INCLUDES "")
  SET(_BINARIES "")

  # Iterate each found module.
  FOREACH(_MODULE ${ARG_MODULE_NAMES})
    GET_PROPERTY(${_MODULE}_IMPORTED TARGET ${_MODULE} PROPERTY IMPORTED)

    # Check, if the library is imported.
    IF(NOT ${_MODULE}_IMPORTED)
      MESSAGE(WARNING "  WARNING: The module ${_MODULE} is expected to be imported, but seems to be part of the build. \
        It will be skipped, which may corrupt your build!")
    ELSE(NOT ${_MODULE}_IMPORTED)
      # Get the include directories of the module.
      GET_PROPERTY(${_MODULE}_INCLUDE_DIRECTORIES TARGET ${_MODULE} PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
      MESSAGE(STATUS "  [${_MODULE}] includes: ${${_MODULE}_INCLUDE_DIRECTORIES}")
      LIST(APPEND _INCLUDES ${${_MODULE}_INCLUDE_DIRECTORIES})
      
      # Check the module type.
      GET_PROPERTY(${_MODULE}_TYPE TARGET ${_MODULE} PROPERTY TYPE)
      #MESSAGE(STATUS "  [${_MODULE}] TYPE: ${${_MODULE}_TYPE}")

      IF(NOT "${${_MODULE}_TYPE}" STREQUAL "INTERFACE_LIBRARY")
        # Resolve the assembly directories of the module.
        GET_PROPERTY(${_MODULE}_IMPORTED_LOCATION TARGET ${_MODULE} PROPERTY IMPORTED_LOCATION)
		
        # If there is no imported assembly, try again by importing for a current configuration.
        IF("${${_MODULE}_IMPORTED_LOCATION}" STREQUAL "")
			GET_PROPERTY(${_MODULE}_IMPORTED_LOCATION TARGET ${_MODULE} PROPERTY IMPORTED_LOCATION_${BUILD_TYPE})
        ENDIF("${${_MODULE}_IMPORTED_LOCATION}" STREQUAL "")
		
        # If there is still no imported assembly, try yet another time by importing for a mapped configuration.
        IF("${${_MODULE}_IMPORTED_LOCATION}" STREQUAL "")
          IF(${BUILD_TYPE} STREQUAL "RELWITHDEBINFO" OR ${BUILD_TYPE} STREQUAL "MINSIZEREL")
            SET(_BUILD_TYPE "RELEASE")
          ELSE(${BUILD_TYPE} STREQUAL "RELWITHDEBINFO" OR ${BUILD_TYPE} STREQUAL "MINSIZEREL")
            SET(_BUILD_TYPE "DEBUG")
          ENDIF(${BUILD_TYPE} STREQUAL "RELWITHDEBINFO" OR ${BUILD_TYPE} STREQUAL "MINSIZEREL")

          GET_PROPERTY(${_MODULE}_IMPORTED_LOCATION TARGET ${_MODULE} PROPERTY IMPORTED_LOCATION_${_BUILD_TYPE})
        ENDIF("${${_MODULE}_IMPORTED_LOCATION}" STREQUAL "")

        # If an assembly has been found, add it to the install list.
        IF(NOT "${${_MODULE}_IMPORTED_LOCATION}" STREQUAL "")
          MESSAGE(STATUS "  [${_MODULE}] assemblies: ${${_MODULE}_IMPORTED_LOCATION}")
          LIST(APPEND _BINARIES ${${_MODULE}_IMPORTED_LOCATION})
        ELSE(NOT "${${_MODULE}_IMPORTED_LOCATION}" STREQUAL "")
          MESSAGE(STATUS "  [${_MODULE}] assemblies: -")
        ENDIF(NOT "${${_MODULE}_IMPORTED_LOCATION}" STREQUAL "")
      ENDIF(NOT "${${_MODULE}_TYPE}" STREQUAL "INTERFACE_LIBRARY")
    ENDIF(NOT ${_MODULE}_IMPORTED)
  ENDFOREACH(_MODULE ${MODULE_NAMES})
  
  # Propagate variables to parent scope.
  SET(${RET_INCLUDES}  ${_INCLUDES} PARENT_SCOPE)
  SET(${RET_INSTALLER} ${_BINARIES} PARENT_SCOPE)
ENDFUNCTION(GET_PACKAGE_INFO ARG_MODULE_NAMES RET_INCLUDES RET_INSTALLER)