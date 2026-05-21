# QUAZIP_FOUND               - QuaZip library was found
# QUAZIP_INCLUDE_DIR         - Path to QuaZip include dir
# QUAZIP_LIBRARIES           - List of QuaZip libraries
#
# Note: Prefer find_package(QuaZip-Qt6) when using vcpkg, which provides
# proper config files. This module is a fallback for manual builds.

IF (QUAZIP_INCLUDE_DIR AND QUAZIP_LIBRARIES)
	# in cache already
	SET(QUAZIP_FOUND TRUE)
ELSE()
	# Search common installation locations across platforms
	FIND_PATH(QUAZIP_INCLUDE_DIR
		NAMES quazip.h
		PATH_SUFFIXES quazip quazip1-qt6 QuaZip-Qt6
	)

	FIND_LIBRARY(QUAZIP_LIBRARIES
		NAMES quazip1-qt6 quazip quazip-qt6
	)

	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(QuaZip
		DEFAULT_MSG
		QUAZIP_LIBRARIES QUAZIP_INCLUDE_DIR
	)

	mark_as_advanced(QUAZIP_INCLUDE_DIR QUAZIP_LIBRARIES)
ENDIF()
