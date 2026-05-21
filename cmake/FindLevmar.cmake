# LEVMAR_FOUND               - Levmar library was found
# LEVMAR_INCLUDE_DIR         - Path to Levmar include dir
# LEVMAR_LIBRARY             - Levmar library

IF (LEVMAR_INCLUDE_DIR AND LEVMAR_LIBRARY)
	# in cache already
	SET(LEVMAR_FOUND TRUE)
ELSE()
	FIND_PATH(LEVMAR_INCLUDE_DIR
		NAMES levmar.h
		PATH_SUFFIXES levmar
	)
	FIND_LIBRARY(LEVMAR_LIBRARY
		NAMES levmar levmar.lib
	)

	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(Levmar
		DEFAULT_MSG
		LEVMAR_LIBRARY LEVMAR_INCLUDE_DIR
	)

	mark_as_advanced(LEVMAR_INCLUDE_DIR LEVMAR_LIBRARY)
ENDIF()
