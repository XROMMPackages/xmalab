# LEVMAR_FOUND               - QuaZip library was found
# LEVMAR_INCLUDE_DIR         - Path to QuaZip include dir
# LEVMAR_LIBRARy           - List of QuaZip libraries


IF (LEVMAR_INCLUDE_DIR AND LEVMAR_LIBRARy)
	# in cache already
	SET(LEVMAR_FOUND TRUE)
ELSE (LEVMAR_INCLUDE_DIR AND LEVMAR_LIBRARy)
	IF (WIN32)
		FIND_LIBRARY(LEVMAR_LIBRARy NAMES levmar.lib)
		FIND_PATH(LEVMAR_INCLUDE_DIR NAMES levmar.h)
	ELSE(WIN32)
		FIND_LIBRARY(LEVMAR_LIBRARy
			WIN32_DEBUG_POSTFIX d
			NAMES  levmar.lib
			HINTS /usr/lib /usr/lib64
		)
		FIND_PATH(LEVMAR_INCLUDE_DIR levmar.h
			HINTS /usr/include /usr/local/include
			PATH_SUFFIXES quazip
		)
	ENDIF (WIN32)
ENDIF (LEVMAR_INCLUDE_DIR AND LEVMAR_LIBRARy)
