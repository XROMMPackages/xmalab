PROJECT(XMALab)

#SET BUILD-TYPE TO RELEASE IF NOT DEFINED
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")

if(NOT CMAKE_BUILD_TYPE MATCHES "^[Dd][Ee][Bb][Uu][Gg]")
        add_definitions(-DQT_NO_DEBUG_OUTPUT)
        set(CMAKE_BUILD_RELEASE TRUE)
else()
        set(CMAKE_BUILD_RELEASE FALSE)
endif()

if(APPLE AND CMAKE_INSTALL_PREFIX MATCHES "/usr/local")
        set(CMAKE_INSTALL_PREFIX "/Applications")
endif()

FIND_PACKAGE( QuaZip REQUIRED )

set(CMAKE_C_FLAGS_RELEASE "-O3")

message(STATUS "Building ${PROJECT_NAME} in ${CMAKE_BUILD_TYPE} mode")
cmake_minimum_required(VERSION 2.8.8)

#ADD OPENCV SUPPORT
FIND_PACKAGE( OpenCV REQUIRED )
FIND_PACKAGE( OpenGL )

#Shared Libs off
#set(OpenCV_SHARED OFF)
#set(BUILD_SHARED_LIBS OFF)

#ADD QT4 SUPPORT
FIND_PACKAGE(Qt4 REQUIRED COMPONENTS QtCore QtGui QtOpenGL )
set(QT_USE_QTOPENGL TRUE)
INCLUDE(${QT_USE_FILE})
SET(CMAKE_AUTOMOC ON)

set(PROJECT_BUILD_TIME "12/03/2014")
set(PROJECT_VERSION_MAJOR "1")
set(PROJECT_VERSION_MINOR "0")
set(PROJECT_VERSION_PATCH "0")
set(PROJECT_VERSION_COUNT 1)
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
set(PROJECT_VENDOR "Brown University")
set(PROJECT_COPYRIGHT_YEAR "2014")
set(PROJECT_DOMAIN_FIRST "xromm")
set(PROJECT_DOMAIN_SECOND "org")
set(PROJECT_DOMAIN "${PROJECT_DOMAIN_FIRST}.${PROJECT_DOMAIN_SECOND}")


# Set some Win32 Specific Settings
IF(WIN32)
	SET(GUI_TYPE WIN32)
	SET(QT_USE_QMAIN true)
	INCLUDE(${QT_USE_FILE})
ENDIF(WIN32)

IF (APPLE)
	SET(GUI_TYPE MACOSX_BUNDLE)
	set( MACOSX_BUNDLE_ICON_FILE XMALab-Icon.icns )
	# Allows for bundle re-creation just by running "make". Also installs bundle icon
	add_custom_target(osx_bundle_dirs
		COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/bin/XMALab.app/Contents/Resources
		COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/bin/XMALab.app/Contents/MacOS
		COMMAND cp ${CMAKE_SOURCE_DIR}/src/ui/resource-files/icons/${MACOSX_BUNDLE_ICON_FILE}
            ${CMAKE_CURRENT_BINARY_DIR}/bin/XMALab.app/Contents/Resources/${MACOSX_BUNDLE_ICON_FILE})
ENDIF (APPLE)

if(CMAKE_COMPILER_IS_GNUCXX OR APPLE)
        add_definitions(-Wall -Wextra)
endif()

ADD_DEFINITIONS(${QT_DEFINITIONS})
ADD_DEFINITIONS(-DPROJECT_VERSION=\"${PROJECT_VERSION}\" -DPROJECT_BUILD_TIME=\"${PROJECT_BUILD_TIME}\")

SET(CMAKE_CURRENT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${QUAZIP_INCLUDE_DIRS})

#SET GENERIC HEADER AND SOURCE FILES
SET(Calibration_HEADERS 
	src/core/Project.h
	src/core/Settings.h
	src/core/Camera.h
	src/core/Image.h
	src/core/CalibrationImage.h
	src/core/UndistortionObject.h
	src/core/CalibrationObject.h
)

SET(Calibration_SOURCES
	src/core/Project.cpp
	src/core/Settings.cpp
	src/core/Camera.cpp
	src/core/Image.cpp
	src/core/CalibrationImage.cpp
	src/core/UndistortionObject.cpp
	src/core/CalibrationObject.cpp
)

SOURCE_GROUP(_CORE FILES ${Calibration_HEADERS} ${Calibration_SOURCES})

SET(Processing_HEADERS 
	src/processing/BlobDetection.h
	src/processing/LocalUndistortion.h
	src/processing/CubeCalibration.h
	src/processing/Calibration.h
)

SET(Processing_SOURCES
	src/processing/BlobDetection.cpp
	src/processing/LocalUndistortion.cpp
	src/processing/CubeCalibration.cpp
	src/processing/Calibration.cpp
)

SOURCE_GROUP(_PROCESSING FILES ${Processing_HEADERS} ${Processing_SOURCES})

#HEADER AND SOURCE FILES FOR UI
SET(Calibration_FORMS_HEADERS 
	src/ui/MainWindow.h
	src/ui/ErrorDialog.h
	src/ui/ConfirmationDialog.h
	src/ui/ProgressDialog.h
	src/ui/NewProjectDialog.h
	src/ui/CameraBox.h
	src/ui/GLSharedWidget.h
	src/ui/CameraViewWidget.h
	src/ui/GLCameraView.h
	src/ui/SequenceNavigationFrame.h
	src/ui/WorkspaceNavigationFrame.h
	src/ui/State.h
	src/ui/ProjectFileIO.h
	src/ui/WizardDockWidget.h
	src/ui/WizardUndistortionFrame.h
	src/ui/WizardCalibrationCubeFrame.h
	src/ui/UndistortSequenceDialog.h
	src/ui/SettingsDialog.h
	src/ui/CalibrationInfoFrame.h
	src/ui/UndistortionInfoFrame.h
	src/ui/AboutDialog.h
	src/ui/WorldViewDockGLWidget.h
	src/ui/WorldViewDockWidget.h
	src/ui/ConsoleDockWidget.h	
)

SET(Calibration_FORMS_SOURCES
		src/ui/main.cpp
     	src/ui/MainWindow.cpp
     	src/ui/ErrorDialog.cpp
		src/ui/ConfirmationDialog.cpp
		src/ui/ProgressDialog.cpp
		src/ui/NewProjectDialog.cpp
		src/ui/CameraBox.cpp
		src/ui/GLSharedWidget.cpp
		src/ui/CameraViewWidget.cpp
		src/ui/GLCameraView.cpp
		src/ui/SequenceNavigationFrame.cpp
		src/ui/WorkspaceNavigationFrame.cpp
		src/ui/State.cpp
		src/ui/ProjectFileIO.cpp
		src/ui/WizardDockWidget.cpp
		src/ui/WizardUndistortionFrame.cpp
		src/ui/WizardCalibrationCubeFrame.cpp
		src/ui/UndistortSequenceDialog.cpp
		src/ui/SettingsDialog.cpp
		src/ui/CalibrationInfoFrame.cpp
		src/ui/UndistortionInfoFrame.cpp	
		src/ui/AboutDialog.cpp	
		src/ui/WorldViewDockGLWidget.cpp	
		src/ui/WorldViewDockWidget.cpp	
		src/ui/ConsoleDockWidget.cpp	
)

SOURCE_GROUP(_UI FILES ${Calibration_FORMS_HEADERS} ${Calibration_FORMS_SOURCES})

#QT DESIGNER UI FILES
SET(Calibration_FORMS 
		src/ui/ui-files/MainWindow.ui
		src/ui/ui-files/ErrorDialog.ui
		src/ui/ui-files/ConfirmationDialog.ui
		src/ui/ui-files/NewProjectDialog.ui
		src/ui/ui-files/CameraBox.ui
		src/ui/ui-files/CameraViewWidget.ui
		src/ui/ui-files/SequenceNavigationFrame.ui
		src/ui/ui-files/WorkspaceNavigationFrame.ui
		src/ui/ui-files/WizardDockWidget.ui
		src/ui/ui-files/WizardUndistortionFrame.ui
		src/ui/ui-files/WizardCalibrationCubeFrame.ui
		src/ui/ui-files/UndistortSequenceDialog.ui
		src/ui/ui-files/SettingsDialog.ui
		src/ui/ui-files/CalibrationInfoFrame.ui
		src/ui/ui-files/UndistortionInfoFrame.ui
		src/ui/ui-files/ProgressDockWidget.ui
		src/ui/ui-files/AboutDialog.ui
		src/ui/ui-files/ConsoleDockWidget.ui
)

#Only for groupinh
SET(Calibration_FORMS_MOC
		build/XMALab_automoc.cpp
		build/ui_MainWindow.h
		build/ui_ErrorDialog.h
		build/ui_ConfirmationDialog.h
		build/ui_ProgressDockWidget.h
		build/ui_NewProjectDialog.h
		build/ui_CameraBox.h
		build/ui_CameraViewWidget.h
		build/ui_SequenceNavigationFrame.h
		build/ui_WorkspaceNavigationFrame.h
		build/ui_WizardDockWidget.h
		build/ui_WizardUndistortionFrame.h
		build/ui_WizardCalibrationCubeFrame.h
		build/ui_UndistortSequenceDialog.h
		build/ui_SettingsDialog.h
		build/ui_CalibrationInfoFrame.h
		build/ui_UndistortionInfoFrame.h
		build/ui_AboutDialog.h
		build/ui_ConsoleDockWidget.h
)

SOURCE_GROUP(UI_Generated FILES ${Calibration_FORMS} ${Calibration_FORMS_MOC})

SET(Calibration_RESOURCE_FILES
		build/qrc_calibrationtool_resources.cxx
		src/ui//calibrationtool_resources.qrc
)

SOURCE_GROUP(RESOURCE FILES ${Calibration_RESOURCE_FILES} src/ui/calibrationtool_resources.rc)


#SETTING UP QT FORMS
#QT4_WRAP_CPP(Calibration_HEADERS_MOC ${Calibration_HEADERS} )
QT4_WRAP_UI(Calibration_FORMS_HEADERS_GEN ${Calibration_FORMS})

SET(Calibration_RESOURCES
		src/ui/calibrationtool_resources.qrc
)
QT4_ADD_RESOURCES(Calibration_RESOURCES_RCC ${Calibration_RESOURCES})

SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin)


#BUILD AND LINK
ADD_EXECUTABLE(${PROJECT_NAME} ${GUI_TYPE}
		${Calibration_SOURCES} 
		${Calibration_HEADERS}
		${Processing_SOURCES} 
		${Processing_HEADERS}
		${Calibration_FORMS_SOURCES} 
		${Calibration_FORMS_HEADERS}
		${Calibration_FORMS_HEADERS_GEN} 
    	${Calibration_RESOURCES_RCC}
		src/ui/calibrationtool_resources.rc
)

IF (APPLE)
  add_dependencies(${PROJECT_NAME} osx_bundle_dirs )
ENDIF (APPLE)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${OpenCV_LIBS} ${OPENGL_LIBRARIES} ${QT_LIBRARIES} ${QUAZIP_LIBRARIES})
