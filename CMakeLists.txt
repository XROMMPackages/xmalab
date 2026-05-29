CMAKE_MINIMUM_REQUIRED(VERSION 3.25)

PROJECT(XMALab
    VERSION 3.0.0
    DESCRIPTION "Marker-based fluoroscopy and XROMM analysis software"
    LANGUAGES C CXX
)

#Set Project version and project variables
SET(PROJECT_BUILD_TIME "11/05/2026")
SET(PROJECT_VERSION_COUNT 50)
SET(PROJECT_BETA_VERSION 0)
SET(PROJECT_VENDOR "Peter Falkingham")
SET(PROJECT_COPYRIGHT_YEAR "2014")
SET(PROJECT_DOMAIN_FIRST "xromm")
SET(PROJECT_DOMAIN_SECOND "org")
SET(PROJECT_DOMAIN "${PROJECT_DOMAIN_FIRST}.${PROJECT_DOMAIN_SECOND}")


#SET BUILD-TYPE TO RELEASE IF NOT DEFINED
if(NOT CMAKE_BUILD_TYPE)
	SET(CMAKE_BUILD_TYPE Release)
ENDIF()
IF(NOT CMAKE_BUILD_TYPE MATCHES "^[Dd][Ee][Bb][Uu][Gg]")
	ADD_DEFINITIONS(-DQT_NO_DEBUG_OUTPUT)
	SET(CMAKE_BUILD_RELEASE TRUE)
else()
	SET(CMAKE_BUILD_RELEASE FALSE)
ENDIF()
MESSAGE(STATUS "Building ${PROJECT_NAME} in ${CMAKE_BUILD_TYPE} mode")


#Set C++17
SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_C_FLAGS_RELEASE "-O3")

#Set paths
SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin)
SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")
IF(APPLE AND CMAKE_INSTALL_PREFIX MATCHES "/usr/local")
	SET(CMAKE_INSTALL_PREFIX "/Applications")
ENDIF()

#Add definitions for Apple
IF(CMAKE_COMPILER_IS_GNUCXX OR APPLE)
	ADD_DEFINITIONS( -Wno-deprecated-register)
ENDIF()
IF (APPLE)
	ADD_DEFINITIONS(-D__APPLE__)
ENDIF (APPLE)

# Set CMP0074 policy to NEW for proper handling of package root variables
cmake_policy(SET CMP0074 NEW)

# User-supplied paths for non-config-mode libraries (fallback for manual cmake-gui usage)
set(MY_INCLUDE_DIR "" CACHE PATH "Optional: path to extra include directories (leave blank if using package manager)")
set(MY_LIBRARY_DIR "" CACHE PATH "Optional: path to extra library directories (leave blank if using package manager)")

# ---------------------------------------------------------------------------
# Dependency detection
# When using vcpkg toolchain (via CMakePresets), these find_package() calls
# will automatically locate libraries in the vcpkg installed directory.
# ---------------------------------------------------------------------------

# QuaZip - use config mode if available (vcpkg provides QuaZip-Qt6Config.cmake)
find_package(QuaZip-Qt6 QUIET)
if(NOT QuaZip-Qt6_FOUND)
	# Fallback: manual detection for non-vcpkg builds
	find_path(QUAZIP_INCLUDE_DIR NAMES quazip.h PATH_SUFFIXES quazip quazip1-qt6)
	find_library(QUAZIP_LIBRARIES NAMES quazip1-qt6 quazip)
	if(NOT QUAZIP_INCLUDE_DIR OR NOT QUAZIP_LIBRARIES)
		message(FATAL_ERROR "QuaZip not found. Install via vcpkg (vcpkg install quazip) or set QUAZIP_INCLUDE_DIR/QUAZIP_LIBRARIES.")
	endif()
endif()

# GLEW - static linking
find_package(GLEW QUIET)
if(NOT GLEW_FOUND)
	# Fallback: manual detection with vcpkg static + common paths
	find_path(GLEW_INCLUDE_DIR NAMES GL/glew.h
		PATHS
			"$ENV{VCPKG_ROOT}/installed/x64-windows-static/include"
			"$ENV{VCPKG_ROOT}/installed/x64-windows/include"
		PATH_SUFFIXES GL
	)
	find_library(GLEW_LIBRARIES NAMES glew32s glew32 glew GLEW
		PATHS
			"$ENV{VCPKG_ROOT}/installed/x64-windows-static/lib"
			"$ENV{VCPKG_ROOT}/installed/x64-windows/lib"
	)
	if(NOT GLEW_INCLUDE_DIR OR NOT GLEW_LIBRARIES)
		message(FATAL_ERROR "GLEW not found. Install via vcpkg (vcpkg install glew) or set GLEW_INCLUDE_DIR/GLEW_LIBRARIES.")
	endif()
endif()

# OpenCV
find_package(OpenCV REQUIRED)

# OpenGL
find_package(OpenGL REQUIRED)

# Levmar - custom find module (vcpkg doesn't provide config files)
find_package(Levmar REQUIRED)

# Qt6
FIND_PACKAGE(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets Concurrent PrintSupport)
cmake_policy(SET CMP0020 NEW)
SET(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
SET(CMAKE_AUTORCC ON)
SET(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ui-files)

#Allow a console window on Windows for debugging
OPTION(WITH_CONSOLE "Build with a command line for debugging" OFF)
IF(WIN32)
	IF(WITH_CONSOLE)
		ADD_DEFINITIONS(-DWITH_CONSOLE)
	ELSE()
		SET(GUI_TYPE WIN32)
		SET(QT_USE_QMAIN true)
	ENDIF()
ENDIF(WIN32)

#Add Resources to App for OSX
IF (APPLE)
	SET(GUI_TYPE MACOSX_BUNDLE)
	SET( MACOSX_BUNDLE_ICON_FILE XMALab-Icon.icns )
	# Copy the icon file so macdeployqt and CMake bundle creation can find it
	ADD_CUSTOM_TARGET(osx_bundle_dirs
		COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/bin/XMALab.app/Contents/Resources
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/src/ui/resource-files/icons/${MACOSX_BUNDLE_ICON_FILE}
			${CMAKE_CURRENT_BINARY_DIR}/bin/XMALab.app/Contents/Resources/${MACOSX_BUNDLE_ICON_FILE})
ENDIF (APPLE)

#Set Definitions
ADD_DEFINITIONS(${QT_DEFINITIONS})
ADD_DEFINITIONS(-DPROJECT_VERSION=\"${PROJECT_VERSION}\" -DPROJECT_BUILD_TIME=\"${PROJECT_BUILD_TIME}\")
IF("${PROJECT_BETA_VERSION}" GREATER 0)
	ADD_DEFINITIONS(-DPROJECT_BETA_VERSION=\"${PROJECT_BETA_VERSION}\")
ENDIF()

ADD_DEFINITIONS(-DQCUSTOMPLOT_USE_OPENGL)
ADD_DEFINITIONS(-DGLEW_STATIC)

#Set Includes and CMAKE_CURRENT_SOURCE_DIR
SET(CMAKE_CURRENT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Build target include directories (modern CMake - no global pollution)
SET(XMALAB_INCLUDE_DIRS
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
)

# Add dependency include dirs (only for non-config-mode packages)
if(NOT QuaZip-Qt6_FOUND AND QUAZIP_INCLUDE_DIR)
	list(APPEND XMALAB_INCLUDE_DIRS ${QUAZIP_INCLUDE_DIR})
endif()
if(NOT GLEW_FOUND AND GLEW_INCLUDE_DIR)
	list(APPEND XMALAB_INCLUDE_DIRS ${GLEW_INCLUDE_DIR})
endif()
if(OPENCV_INCLUDE_DIRS)
	list(APPEND XMALAB_INCLUDE_DIRS ${OPENCV_INCLUDE_DIRS})
endif()
if(LEVMAR_INCLUDE_DIR)
	list(APPEND XMALAB_INCLUDE_DIRS ${LEVMAR_INCLUDE_DIR})
endif()
if(MY_INCLUDE_DIR)
	list(APPEND XMALAB_INCLUDE_DIRS ${MY_INCLUDE_DIR})
endif()

#Get Files from Subdirectory
ADD_SUBDIRECTORY(src)



# Build and link
ADD_EXECUTABLE(${PROJECT_NAME} ${GUI_TYPE}
		${XMALAB_SOURCES}
		src/ui/calibrationtool_resources.qrc
		src/ui/calibrationtool_resources.rc
)

# Apply include directories to the target (modern CMake)
target_include_directories(${PROJECT_NAME} PRIVATE ${XMALAB_INCLUDE_DIRS})

# ---------------------------------------------------------------------------
# Post-build deployment steps
# ---------------------------------------------------------------------------

# Windows: Run windeployqt to copy Qt DLLs, plugins, and platform files
IF(WIN32)
	find_program(WINDEPLOYQT_EXECUTABLE
		NAMES windeployqt windeployqt6
		HINTS
			"$ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/tools/Qt6/bin"
			"$ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/bin"
			"${Qt6_DIR}/../../../bin"
		PATH_SUFFIXES bin
	)
	if(WINDEPLOYQT_EXECUTABLE)
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-compiler-runtime --no-system-d3d-compiler --no-translations --no-opengl-sw --verbose 0 "$<TARGET_FILE:${PROJECT_NAME}>"
			COMMENT "Deploying Qt dependencies with windeployqt..."
		)
	endif()
ENDIF()

# macOS: Run macdeployqt to bundle Qt frameworks
IF(APPLE)
	find_program(MACDEPLOYQT_EXECUTABLE
		NAMES macdeployqt macdeployqt6
		HINTS
			"$ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/tools/Qt6/bin"
			"$ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/bin"
			"${Qt6_DIR}/../../../bin"
		PATH_SUFFIXES bin
	)
	if(MACDEPLOYQT_EXECUTABLE)
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND "${MACDEPLOYQT_EXECUTABLE}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../.." -always-overwrite
			COMMENT "Deploying Qt dependencies with macdeployqt..."
		)
	endif()
ENDIF()

# Compiler-specific optimizations
if (MSVC)
    if (CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(${PROJECT_NAME} PRIVATE
            /O2         # Max speed
            /GL         # Whole program optimization
            /Oi         # Use intrinsics
            /Ot         # Favor speed over size
            /Oy         # Omit frame pointers
            /Gy         # Function-level linking
			/MP 	   # Multi-processor compilation
        )
        target_link_options(${PROJECT_NAME} PRIVATE
            /LTCG       # Link-time code generation
            /OPT:REF    # Eliminate unused functions/data
            /OPT:ICF    # Fold identical functions
        )
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE
            /RTC1       # Runtime error checks
        )
    endif()
endif()

IF(APPLE)
  ADD_DEPENDENCIES(${PROJECT_NAME} osx_bundle_dirs )
ENDIF (APPLE)

# Link libraries (modern CMake - mix of imported targets and legacy variables)
TARGET_LINK_LIBRARIES(${PROJECT_NAME}
	Qt6::Core
	Qt6::Widgets
	Qt6::OpenGL
	Qt6::OpenGLWidgets
	Qt6::Concurrent
	Qt6::Gui
	Qt6::PrintSupport
	${OpenCV_LIBS}
	OpenGL::GL
	glu32
)

# Add legacy-style dependencies (config-mode packages provide their own targets)
if(QuaZip-Qt6_FOUND)
	target_link_libraries(${PROJECT_NAME} QuaZip::QuaZip)
else()
	target_link_libraries(${PROJECT_NAME} ${QUAZIP_LIBRARIES})
endif()

if(GLEW_FOUND)
	target_link_libraries(${PROJECT_NAME} GLEW::GLEW)
else()
	target_link_libraries(${PROJECT_NAME} ${GLEW_LIBRARIES})
endif()

if(LEVMAR_LIBRARY)
	target_link_libraries(${PROJECT_NAME} ${LEVMAR_LIBRARY})
endif()

if(MY_LIBRARY_DIR)
	link_directories(${MY_LIBRARY_DIR})
endif()

# Create groups for VS
IF(MSVC OR MSVC_IDE)
	FOREACH(source IN LISTS XMALAB_SOURCES)
		GET_FILENAME_COMPONENT(source_path "${source}" PATH)
		STRING(REPLACE "/" "\\" source_path_msvc "${source_path}")
		STRING(REPLACE "src\\" "_" source_path_msvc2 "${source_path_msvc}")
		SOURCE_GROUP("${source_path_msvc2}" FILES "${source}")
	ENDFOREACH()

	#CMakeLists.txt
	FILE(GLOB_RECURSE CMAKE_LISTS RELATIVE ${CMAKE_SOURCE_DIR}
		"CMakeLists.txt"
	)
	SET(CMAKE_LISTS ${CMAKE_LISTS} license.txt README.md)
	ADD_CUSTOM_TARGET(CMakeLists.txt SOURCES ${CMAKE_LISTS})
	FOREACH(source IN LISTS CMAKE_LISTS)
		GET_FILENAME_COMPONENT(source_path "${source}" PATH)
		STRING(REPLACE "/" "\\" source_path_msvc "${source_path}")
		STRING(REPLACE "src\\" "_" source_path_msvc2 "${source_path_msvc}")
		SOURCE_GROUP("${source_path_msvc2}" FILES "${source}")
	ENDFOREACH()

	SOURCE_GROUP(UI_FILES FILES ${XMALab_FORMS})
	SOURCE_GROUP(UI_FILES FILES src/ui/calibrationtool_resources.qrc src/ui/calibrationtool_resources.rc )
	SOURCE_GROUP(UI_FILES FILES ${CMAKE_CURRENT_BINARY_DIR}/XMALab_automoc.cpp)
ENDIF()
