CMAKE_MINIMUM_REQUIRED(VERSION 2.8.8)

PROJECT(XMALab)

#Set Project version and project variables
SET(PROJECT_BUILD_TIME "10/23/2021")
SET(PROJECT_VERSION_MAJOR "2")
SET(PROJECT_VERSION_MINOR "1")
SET(PROJECT_VERSION_PATCH "0")
SET(PROJECT_VERSION_COUNT 50)
SET(PROJECT_BETA_VERSION 0)
SET(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
SET(PROJECT_VENDOR "Brown University")
SET(PROJECT_COPYRIGHT_YEAR "2014")
SET(PROJECT_DOMAIN_FIRST "xromm")
SET(PROJECT_DOMAIN_SECOND "org")
SET(PROJECT_DOMAIN "${PROJECT_DOMAIN_FIRST}.${PROJECT_DOMAIN_SECOND}")


#SET BUILD-TYPE TO RELEASE IF NOT DEFINED
if(NOT CMAKE_BUILD_TYPE)
	SET(CMAKE_BUILD_TYPE Release)
ENDIF(NOT CMAKE_BUILD_TYPE)
IF(NOT CMAKE_BUILD_TYPE MATCHES "^[Dd][Ee][Bb][Uu][Gg]")
    ADD_DEFINITIONS(-DQT_NO_DEBUG_OUTPUT)
    SET(CMAKE_BUILD_RELEASE TRUE)
else()
    SET(CMAKE_BUILD_RELEASE FALSE)
ENDIF()
MESSAGE(STATUS "Building ${PROJECT_NAME} in ${CMAKE_BUILD_TYPE} mode")


#Set C++11
SET(CMAKE_CXX_STANDARD 11)
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

#Add Quazip
FIND_PACKAGE( QuaZip REQUIRED )

#ADD OPENCV 
FIND_PACKAGE( OpenCV REQUIRED )
FIND_PACKAGE( OpenGL )
FIND_PACKAGE( GLEW REQUIRED )
SET(GLEW_USE_STATIC_LIBS FALSE)
FIND_PACKAGE( Levmar REQUIRED)

#ADD QT4 SUPPORT
FIND_PACKAGE(Qt5 REQUIRED COMPONENTS Core GUI OpenGL Widgets PrintSupport)
cmake_policy(SET CMP0020 OLD)
#SET(QT_USE_QTOPENGL TRUE)
SET(CMAKE_AUTOMOC ON)

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
	# Allows for bundle re-creation just by running "make". Also installs bundle icon
	ADD_CUSTOM_TARGET(osx_bundle_dirs
		COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/bin/XMALab.app/Contents/Resources
		COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/bin/XMALab.app/Contents/MacOS
		COMMAND cp ${CMAKE_SOURCE_DIR}/src/ui/resource-files/icons/${MACOSX_BUNDLE_ICON_FILE}
            ${CMAKE_CURRENT_BINARY_DIR}/bin/XMALab.app/Contents/Resources/${MACOSX_BUNDLE_ICON_FILE})
ENDIF (APPLE)

#Set Definitions
ADD_DEFINITIONS(${QT_DEFINITIONS})
ADD_DEFINITIONS(-DPROJECT_VERSION=\"${PROJECT_VERSION}\" -DPROJECT_BUILD_TIME=\"${PROJECT_BUILD_TIME}\")
IF("${PROJECT_BETA_VERSION}" GREATER 0)
	ADD_DEFINITIONS(-DPROJECT_BETA_VERSION=\"${PROJECT_BETA_VERSION}\")
ENDIF()

ADD_DEFINITIONS(-DQCUSTOMPLOT_USE_OPENGL)

#Set Includes and CMAKE_CURRENT_SOURCE_DIR
SET(CMAKE_CURRENT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
INCLUDE_DIRECTORIES(
	${CMAKE_CURRENT_BINARY_DIR} 
	${CMAKE_CURRENT_SOURCE_DIR} 
	${QUAZIP_INCLUDE_DIRS} 
	${GLEW_INCLUDE_DIR} 
	${OPENCV_INCLUDE_DIRS} 
	${LEVMAR_INCLUDE_DIR} 
	${Qt5Core_INCLUDE_DIRS}
	${Qt5Widgets_INCLUDE_DIRS}
	${Qt5OpenGL_INCLUDE_DIRS}
	${Qt5GUI_INCLUDE_DIRS}
)

#Get Files from Subdirectory
ADD_SUBDIRECTORY(src)

#Create QT4 UI related tasks
SET(XMALab_RESOURCE_FILES
		${CMAKE_CURRENT_BINARY_DIR}/qrc_calibrationtool_resources.cxx
		src/ui//calibrationtool_resources.qrc
)
QT5_WRAP_UI(XMALab_FORMS_HEADERS_GEN ${XMALab_FORMS})
SET(XMALab_RESOURCES
		src/ui/calibrationtool_resources.qrc
)
QT5_ADD_RESOURCES(XMALab_RESOURCES_RCC ${XMALab_RESOURCES})


#Build and link
ADD_EXECUTABLE(${PROJECT_NAME} ${GUI_TYPE}
		${XMALAB_SOURCES} 
    	${XMALab_RESOURCES_RCC}
		${XMALab_FORMS_HEADERS_GEN}
		src/ui/calibrationtool_resources.rc
)

IF(APPLE)
  ADD_DEPENDENCIES(${PROJECT_NAME} osx_bundle_dirs )
ENDIF (APPLE)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} 
		${GLEW_LIBRARIES} 
		${QUAZIP_LIBRARIES}
		${LEVMAR_LIBRARY}
		${Qt5Core_LIBRARIES}
		${Qt5Widgets_LIBRARIES}
		${Qt5OpenGL_LIBRARIES}
		${Qt5GUI_LIBRARIES}
		${Qt5PrintSupport_LIBRARIES}
		${OpenCV_LIBS} 
		${OPENGL_LIBRARIES} 
)

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
	SOURCE_GROUP(UI_FILES FILES ${XMALab_FORMS_HEADERS_GEN})
	SOURCE_GROUP(UI_FILES FILES ${XMALab_FORMS_HEADERS_GEN})
	SOURCE_GROUP(UI_FILES FILES ${XMALab_RESOURCE_FILES} src/ui/calibrationtool_resources.rc )
	SOURCE_GROUP(UI_FILES FILES ${CMAKE_CURRENT_BINARY_DIR}/XMALab_automoc.cpp)
ENDIF()
