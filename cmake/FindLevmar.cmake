# LEVMAR_FOUND               - Levmar library was found
# LEVMAR_INCLUDE_DIR         - Path to Levmar include dir
# LEVMAR_LIBRARy           - List of Levmar libraries


IF (LEVMAR_INCLUDE_DIR AND LEVMAR_LIBRARY)
	# in cache already
	SET(LEVMAR_FOUND TRUE)
ELSE (LEVMAR_INCLUDE_DIR AND LEVMAR_LIBRARY)
	IF (WIN32)
		FIND_LIBRARY(LEVMAR_LIBRARY NAMES levmar.lib)
		FIND_PATH(LEVMAR_INCLUDE_DIR NAMES levmar.h)
	ELSE(WIN32)
		FIND_LIBRARY(LEVMAR_LIBRARY
			WIN32_DEBUG_POSTFIX d
			NAMES  levmar.lib
			HINTS /usr/lib /usr/lib64
		)
		FIND_PATH(LEVMAR_INCLUDE_DIR levmar.h
			HINTS /usr/include /usr/local/include
			PATH_SUFFIXES levmar
		)
	ENDIF (WIN32)
ENDIF (LEVMAR_INCLUDE_DIR AND LEVMAR_LIBRARY)
