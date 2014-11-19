# - Try to find OpenCL
# This module tries to find an OpenCL implementation on your system. It supports
# AMD / ATI, Apple and NVIDIA implementations, but shoudl work, too.
#
# Once done this will define
#  QUAZIP_FOUND        - system has QuaZip
#  QUAZIP_INCLUDE_DIRS  - the OpenCL include directory
#  QUAZIP_LIBRARIES    - link these to use OpenCL
#
# WIN32 should work, but is untested

FIND_PACKAGE( PackageHandleStandardArgs )

IF (APPLE)
FIND_LIBRARY(QUAZIP_LIBRARIES quazip DOC "QuaZiplib")
FIND_PATH(QUAZIP_INCLUDE_DIR quazip.h DOC "Include for QuaZip")
FIND_PATH(QUAZIP_ZLIB_INCLUDE_DIR zlib.h DOC "Include for Zlib")

ELSE (APPLE)

	IF (WIN32)
		FIND_LIBRARY(QUAZIP_LIBRARIES quazip.lib DOC "QuaZiplib")
		FIND_PATH(QUAZIP_INCLUDE_DIR quazip.h DOC "Include for QuaZip")
		FIND_PATH(QUAZIP_ZLIB_INCLUDE_DIR zlib.h DOC "Include for Zlib")

	ELSE (WIN32)

            # Unix style platforms
            FIND_LIBRARY(QUAZIP_LIBRARIES quazip
              ENV LD_LIBRARY_PATH
            )

		 FIND_PATH(QUAZIP_INCLUDE_DIR quazip.h PATHS
		 FIND_PATH(QUAZIP_ZLIB_INCLUDE_DIR zlib.h PATHS")

	ENDIF (WIN32)
ENDIF (APPLE)

SET(QUAZIP_INCLUDE_DIRS ${QUAZIP_INCLUDE_DIR} ${QUAZIP_ZLIB_INCLUDE_DIR})
ENDIF (QUAZIP_INCLUDE_DIRS AND QUAZIP_LIBRARIES)
