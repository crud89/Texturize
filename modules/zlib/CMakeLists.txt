CMAKE_MINIMUM_REQUIRED(VERSION 2.4.4)
SET(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS ON)
PROJECT(zlib C)
SET(VERSION "1.2.11")

# We do not want to install zlib headers and resources
SET(SKIP_INSTALL_HEADERS  ON)
SET(SKIP_INSTALL_FILES    ON)

INCLUDE(CheckTypeSize)
INCLUDE(CheckFunctionExists)
INCLUDE(CheckIncludeFile)
INCLUDE(CheckCSourceCompiles)

CHECK_INCLUDE_FILE(sys/types.h HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILE(stdint.h    HAVE_STDINT_H)
CHECK_INCLUDE_FILE(stddef.h    HAVE_STDDEF_H)

# Check to see if we have large file support
SET(CMAKE_REQUIRED_DEFINITIONS -D_LARGEFILE64_SOURCE=1)

# We add these other definitions here because CheckTypeSize.cmake
# in CMake 2.4.x does not automatically do so and we want
# compatibility with CMake 2.4.x.
IF(HAVE_SYS_TYPES_H)
  LIST(APPEND CMAKE_REQUIRED_DEFINITIONS -DHAVE_SYS_TYPES_H)
ENDIF(HAVE_SYS_TYPES_H)

IF(HAVE_STDINT_H)
  LIST(APPEND CMAKE_REQUIRED_DEFINITIONS -DHAVE_STDINT_H)
ENDIF(HAVE_STDINT_H)

IF(HAVE_STDDEF_H)
  LIST(APPEND CMAKE_REQUIRED_DEFINITIONS -DHAVE_STDDEF_H)
ENDIF(HAVE_STDDEF_H)

CHECK_TYPE_SIZE(off64_t OFF64_T)

IF(HAVE_OFF64_T)
  ADD_DEFINITIONS(-D_LARGEFILE64_SOURCE=1)
ENDIF(HAVE_OFF64_T)

SET(CMAKE_REQUIRED_DEFINITIONS)

# Check for fseeko
CHECK_FUNCTION_EXISTS(fseeko HAVE_FSEEKO)

IF(NOT HAVE_FSEEKO)
  ADD_DEFINITIONS(-DNO_FSEEKO)
ENDIF(NOT HAVE_FSEEKO)

# Check for unistd.h
CHECK_INCLUDE_FILE(unistd.h Z_HAVE_UNISTD_H)

IF(MSVC)
  # Disabled, since zlib is imported for some references
  #SET(CMAKE_DEBUG_POSTFIX "d")
  ADD_DEFINITIONS(-D_CRT_SECURE_NO_DEPRECATE)
  ADD_DEFINITIONS(-D_CRT_NONSTDC_NO_DEPRECATE)
  INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
ENDIF(MSVC)

IF(NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
  # If we're doing an out of source build and the user has a zconf.h
  # in their source tree...
  IF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/zconf.h)
    MESSAGE(STATUS "Renaming")
    MESSAGE(STATUS "    ${CMAKE_CURRENT_SOURCE_DIR}/zconf.h")
    MESSAGE(STATUS "to 'zconf.h.included' because this file is included with zlib")
    MESSAGE(STATUS "but CMake generates it automatically in the build directory.")
    FILE(RENAME ${CMAKE_CURRENT_SOURCE_DIR}/zconf.h ${CMAKE_CURRENT_SOURCE_DIR}/zconf.h.included)
  ENDIF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/zconf.h)
ENDIF(NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)

SET(ZLIB_PC ${CMAKE_CURRENT_BINARY_DIR}/zlib.pc)
CONFIGURE_FILE( ${CMAKE_CURRENT_SOURCE_DIR}/zlib.pc.cmakein ${ZLIB_PC} @ONLY)
CONFIGURE_FILE(	${CMAKE_CURRENT_SOURCE_DIR}/zconf.h.cmakein ${CMAKE_CURRENT_BINARY_DIR}/zconf.h @ONLY)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR})

#============================================================================
# zlib
#============================================================================

SET(ZLIB_PUBLIC_HDRS 
  ${CMAKE_CURRENT_BINARY_DIR}/zconf.h
  zlib.h
)

SET(ZLIB_PRIVATE_HDRS
  crc32.h
  deflate.h
  gzguts.h
  inffast.h
  inffixed.h
  inflate.h
  inftrees.h
  trees.h
  zutil.h
)

SET(ZLIB_SRCS
  adler32.c
  compress.c
  crc32.c
  deflate.c
  gzclose.c
  gzlib.c
  gzread.c
  gzwrite.c
  inflate.c
  infback.c
  inftrees.c
  inffast.c
  trees.c
  uncompr.c
  zutil.c
)

IF(NOT MINGW)
  SET(ZLIB_DLL_SRCS win32/zlib1.rc)
ENDIF(NOT MINGW)

IF(CMAKE_COMPILER_IS_GNUCC)
  IF(ASM686)
    SET(ZLIB_ASMS contrib/asm686/match.S)
  ELSEIF(AMD64)
    SET(ZLIB_ASMS contrib/amd64/amd64-match.S)
  ENDIF(ASM686)

  IF(ZLIB_ASMS)
    ADD_DEFINITIONS(-DASMV)
    SET_SOURCE_FILES_PROPERTIES(${ZLIB_ASMS} PROPERTIES LANGUAGE C COMPILE_FLAGS -DNO_UNDERLINE)
  ENDIF(ZLIB_ASMS)
ENDIF(CMAKE_COMPILER_IS_GNUCC)

IF(MSVC)
  IF(ASM686)
    ENABLE_LANGUAGE(ASM_MASM)
    SET(ZLIB_ASMS contrib/masmx86/inffas32.asm contrib/masmx86/match686.asm)
  ELSEIF(AMD64)
    ENABLE_LANGUAGE(ASM_MASM)
    SET(ZLIB_ASMS contrib/masmx64/gvmat64.asm contrib/masmx64/inffasx64.asm)
  ENDIF(ASM686)

  IF(ZLIB_ASMS)
    ADD_DEFINITIONS(-DASMV -DASMINF)
  ENDIF(ZLIB_ASMS)
ENDIF(MSVC)

# parse the full version number from zlib.h and include in ZLIB_FULL_VERSION
FILE(READ ${CMAKE_CURRENT_SOURCE_DIR}/zlib.h _zlib_h_contents)
STRING(REGEX REPLACE ".*#define[ \t]+ZLIB_VERSION[ \t]+\"([-0-9A-Za-z.]+)\".*" "\\1" ZLIB_FULL_VERSION ${_zlib_h_contents})

IF(MINGW)
  # This gets us DLL resource information when compiling on MinGW.
  IF(NOT CMAKE_RC_COMPILER)
    SET(CMAKE_RC_COMPILER windres.exe)
  ENDIF(NOT CMAKE_RC_COMPILER)

  ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/zlib1rc.obj COMMAND ${CMAKE_RC_COMPILER} -D GCC_WINDRES -I ${CMAKE_CURRENT_SOURCE_DIR} -I ${CMAKE_CURRENT_BINARY_DIR} -o ${CMAKE_CURRENT_BINARY_DIR}/zlib1rc.obj -i ${CMAKE_CURRENT_SOURCE_DIR}/win32/zlib1.rc)
  SET(ZLIB_DLL_SRCS ${CMAKE_CURRENT_BINARY_DIR}/zlib1rc.obj)
ENDIF(MINGW)

ADD_LIBRARY(zlib SHARED ${ZLIB_SRCS} ${ZLIB_ASMS} ${ZLIB_DLL_SRCS} ${ZLIB_PUBLIC_HDRS} ${ZLIB_PRIVATE_HDRS})
ADD_LIBRARY(zlibstatic STATIC ${ZLIB_SRCS} ${ZLIB_ASMS} ${ZLIB_PUBLIC_HDRS} ${ZLIB_PRIVATE_HDRS})
SET_TARGET_PROPERTIES(zlib PROPERTIES DEFINE_SYMBOL ZLIB_DLL)
SET_TARGET_PROPERTIES(zlib PROPERTIES SOVERSION 1)

IF(NOT CYGWIN)
  # This property causes shared libraries on Linux to have the full version
  # encoded into their final filename.  We disable this on Cygwin because
  # it causes cygz-${ZLIB_FULL_VERSION}.dll to be created when cygz.dll
  # seems to be the default.
  #
  # This has no effect with MSVC, on that platform the version info for
  # the DLL comes from the resource file win32/zlib1.rc
  SET_TARGET_PROPERTIES(zlib PROPERTIES VERSION ${ZLIB_FULL_VERSION})
ENDIF(NOT CYGWIN)

IF(UNIX)
    # On unix-like platforms the library is almost always called libz
  SET_TARGET_PROPERTIES(zlib zlibstatic PROPERTIES OUTPUT_NAME z)
  IF(NOT APPLE)
     SET_TARGET_PROPERTIES(zlib PROPERTIES LINK_FLAGS "-Wl,--version-script,\"${CMAKE_CURRENT_SOURCE_DIR}/zlib.map\"")
  ENDIF(NOT APPLE)
ELSEIF(BUILD_SHARED_LIBS AND WIN32)
    # Creates zlib1.dll when building shared library version
    SET_TARGET_PROPERTIES(zlib PROPERTIES SUFFIX "1.dll")
ENDIF(UNIX)

IF(NOT SKIP_INSTALL_LIBRARIES AND NOT SKIP_INSTALL_ALL)
  INSTALL(TARGETS zlib zlibstatic
    RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
    LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" 
  )
ENDIF(NOT SKIP_INSTALL_LIBRARIES AND NOT SKIP_INSTALL_ALL)

IF(NOT SKIP_INSTALL_HEADERS AND NOT SKIP_INSTALL_ALL)
  INSTALL(FILES ${ZLIB_PUBLIC_HDRS} DESTINATION "${INSTALL_INC_DIR}")
ENDIF(NOT SKIP_INSTALL_HEADERS AND NOT SKIP_INSTALL_ALL)

IF(NOT SKIP_INSTALL_FILES AND NOT SKIP_INSTALL_ALL)
  INSTALL(FILES zlib.3 DESTINATION "${INSTALL_MAN_DIR}/man3")
ENDIF(NOT SKIP_INSTALL_FILES AND NOT SKIP_INSTALL_ALL)

IF(NOT SKIP_INSTALL_FILES AND NOT SKIP_INSTALL_ALL)
  INSTALL(FILES ${ZLIB_PC} DESTINATION "${INSTALL_PKGCONFIG_DIR}")
ENDIF(NOT SKIP_INSTALL_FILES AND NOT SKIP_INSTALL_ALL)