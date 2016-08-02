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

if(MSVC OR MSVC_IDE) 
  message( "VC11: use Microsoft Visual Studio 2013 " 
      "with Microsoft Visual C++ Compiler Nov 2012 CTP (v120p_xp)" ) 
    set(CMAKE_GENERATOR_TOOLSET   "v120_xp" CACHE STRING "Platform Toolset" FORCE) 
    set(CMAKE_VS_PLATFORM_TOOLSET  "v120_xp" CACHE STRING "Platform Toolset" FORCE) 
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
FIND_PACKAGE( GLEW REQUIRED )
FIND_PACKAGE( Levmar REQUIRED)
FIND_PACKAGE( QCustomPlot REQUIRED)

#Shared Libs off
#set(OpenCV_SHARED OFF)
#set(BUILD_SHARED_LIBS OFF)

#ADD QT4 SUPPORT
FIND_PACKAGE(Qt4 REQUIRED COMPONENTS QtCore QtGui QtOpenGL )
set(QT_USE_QTOPENGL TRUE)
INCLUDE(${QT_USE_FILE})
SET(CMAKE_AUTOMOC ON)

set(PROJECT_BUILD_TIME "7/21/2016")
set(PROJECT_VERSION_MAJOR "1")
set(PROJECT_VERSION_MINOR "3")
set(PROJECT_VERSION_PATCH "6")
set(PROJECT_VERSION_COUNT 34)
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
set(PROJECT_VENDOR "Brown University")
set(PROJECT_COPYRIGHT_YEAR "2014")
set(PROJECT_DOMAIN_FIRST "xromm")
set(PROJECT_DOMAIN_SECOND "org")
set(PROJECT_DOMAIN "${PROJECT_DOMAIN_FIRST}.${PROJECT_DOMAIN_SECOND}")

# Set some Win32 Specific Settings
OPTION(WITH_CONSOLE "Build with a command line for debugging" OFF)
IF(WIN32)
	IF(WITH_CONSOLE)
		ADD_DEFINITIONS(-DWITH_CONSOLE)
	ELSE()
		SET(GUI_TYPE WIN32)
		SET(QT_USE_QMAIN true)
	ENDIF()
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
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${QUAZIP_INCLUDE_DIRS} ${GLEW_INCLUDE_DIR} ${OPENCV_INCLUDE_DIRS} ${LEVMAR_INCLUDE_DIR} ${QCUSTOMPLOT_INCLUDE_DIR})

#SET GENERIC HEADER AND SOURCE FILES
SET(XMALab_HEADERS 
	src/core/Project.h
	src/core/Settings.h
	src/core/Camera.h
	src/core/Image.h
	src/core/CalibrationImage.h
	src/core/UndistortionObject.h
	src/core/CalibrationObject.h
	src/core/Trial.h
	src/core/Marker.h
	src/core/RigidBody.h
	src/core/CineVideo.h
	src/core/AviVideo.h
	src/core/VideoStream.h
	src/core/ImageSequence.h
	src/core/HelperFunctions.h
	src/core/RigidBodyObj.h
)

SET(XMALab_SOURCES
	src/core/Project.cpp
	src/core/Settings.cpp
	src/core/Camera.cpp
	src/core/Image.cpp
	src/core/CalibrationImage.cpp
	src/core/UndistortionObject.cpp
	src/core/CalibrationObject.cpp
	src/core/Trial.cpp
	src/core/Marker.cpp
	src/core/RigidBody.cpp
	src/core/CineVideo.cpp
	src/core/AviVideo.cpp
	src/core/VideoStream.cpp
	src/core/ImageSequence.cpp
	src/core/HelperFunctions.cpp
	src/core/RigidBodyObj.cpp
)

SOURCE_GROUP(_CORE FILES ${XMALab_HEADERS} ${XMALab_SOURCES})

SET(Processing_HEADERS 
	src/processing/ThreadScheduler.h
	src/processing/ThreadedProcessing.h
	src/processing/BlobDetection.h
	src/processing/LocalUndistortion.h
	src/processing/CubeCalibration.h
	src/processing/Calibration.h
	src/processing/MarkerDetection.h
	src/processing/MarkerTracking.h
	src/processing/ButterworthLowPassFilter.h
	src/processing/CheckerboardDetection.h
	src/processing/MultiCameraCalibration.h
	src/processing/UpdateTrialFrame.h
	src/processing/cubic.h
)

SET(Processing_SOURCES
	src/processing/ThreadScheduler.cpp
	src/processing/ThreadedProcessing.cpp
	src/processing/BlobDetection.cpp
	src/processing/LocalUndistortion.cpp
	src/processing/CubeCalibration.cpp
	src/processing/Calibration.cpp
	src/processing/MarkerDetection.cpp
	src/processing/MarkerTracking.cpp
	src/processing/ButterworthLowPassFilter.cpp
	src/processing/CheckerboardDetection.cpp
	src/processing/MultiCameraCalibration.cpp
	src/processing/UpdateTrialFrame.cpp
)

SOURCE_GROUP(_PROCESSING FILES ${Processing_HEADERS} ${Processing_SOURCES})

SET(GL_HEADERS 
	src/gl/Framebuffer.h
	src/gl/VertexBuffer.h
	src/gl/GLMLoader.h
	src/gl/Shader.h
	src/gl/DistortionShader.h
	src/gl/MultisampleFramebuffer.h
	src/gl/BlendShader.h
)

SET(GL_SOURCES
	src/gl/Framebuffer.cpp
	src/gl/VertexBuffer.cpp
	src/gl/GLMLoader.cpp
	src/gl/Shader.cpp
	src/gl/DistortionShader.cpp
	src/gl/MultisampleFramebuffer.cpp
	src/gl/BlendShader.cpp
)

SOURCE_GROUP(_GL FILES ${GL_HEADERS} ${GL_SOURCES})

#HEADER AND SOURCE FILES FOR UI
SET(XMALab_FORMS_HEADERS 
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
	src/ui/WizardDigitizationFrame.h	
	src/ui/CameraBoxTrial.h
	src/ui/NewTrialDialog.h
	src/ui/MarkerTreeWidget.h
	src/ui/PointsDockWidget.h
	src/ui/CameraViewDetailWidget.h
	src/ui/DetailViewDockWidget.h
	src/ui/ImportExportPointsDialog.h
	src/ui/PlotWindow.h
	src/ui/Shortcuts.h
	src/ui/MarkerTreeWidgetButton.h
	src/ui/RigidBodyDialog.h
	src/ui/MarkerDialog.h
	src/ui/TrialDialog.h
	src/ui/PointImportExportDialog.h
	src/ui/OptimizationDialog.h
	src/ui/FromToDialog.h
	src/ui/DigitizationInfoFrame.h
	src/ui/MetaDataInfo.h
	src/ui/TrialSelectorDialog.h
	src/ui/StatusColorDialog.h
)

SET(XMALab_FORMS_SOURCES
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
		src/ui/WizardDigitizationFrame.cpp	
		src/ui/CameraBoxTrial.cpp
		src/ui/NewTrialDialog.cpp
		src/ui/MarkerTreeWidget.cpp
		src/ui/PointsDockWidget.cpp
		src/ui/CameraViewDetailWidget.cpp
		src/ui/DetailViewDockWidget.cpp
		src/ui/ImportExportPointsDialog.cpp
		src/ui/PlotWindow.cpp
		src/ui/Shortcuts.cpp
		src/ui/MarkerTreeWidgetButton.cpp
		src/ui/RigidBodyDialog.cpp
		src/ui/MarkerDialog.cpp
		src/ui/TrialDialog.cpp
		src/ui/PointImportExportDialog.cpp
		src/ui/OptimizationDialog.cpp
		src/ui/FromToDialog.cpp
		src/ui/DigitizationInfoFrame.cpp
		src/ui/MetaDataInfo.cpp
		src/ui/TrialSelectorDialog.cpp
		src/ui/StatusColorDialog.cpp
)

SOURCE_GROUP(_UI FILES ${XMALab_FORMS_HEADERS} ${XMALab_FORMS_SOURCES})

#QT DESIGNER UI FILES
SET(XMALab_FORMS 
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
		src/ui/ui-files/WizardDigitizationFrame.ui
		src/ui/ui-files/ConsoleDockWidget.ui
		src/ui/ui-files/CameraBoxTrial.ui
		src/ui/ui-files/NewTrialDialog.ui
		src/ui/ui-files/PointsDockWidget.ui
		src/ui/ui-files/CameraViewDetailWidget.ui
		src/ui/ui-files/DetailViewDockWidget.ui
		src/ui/ui-files/ImportExportPointsDialog.ui
		src/ui/ui-files/RigidBodyDialog.ui
		src/ui/ui-files/MarkerDialog.ui
		src/ui/ui-files/TrialDialog.ui
		src/ui/ui-files/PlotWindow.ui
		src/ui/ui-files/PointImportExportDialog.ui
		src/ui/ui-files/OptimizationDialog.ui
		src/ui/ui-files/FromToDialog.ui
		src/ui/ui-files/DigitizationInfoFrame.ui
		src/ui/ui-files/MetaDataInfo.ui
		src/ui/ui-files/TrialSelectorDialog.ui
		src/ui/ui-files/StatusColorDialog.ui
	)

SOURCE_GROUP(UI_Files FILES ${XMALab_FORMS})

#Only for groupinh
SET(XMALab_FORMS_MOC
		${CMAKE_CURRENT_BINARY_DIR}/XMALab_automoc.cpp
		${CMAKE_CURRENT_BINARY_DIR}/ui_MainWindow.h
		${CMAKE_CURRENT_BINARY_DIR}/ui_ErrorDialog.h
		${CMAKE_CURRENT_BINARY_DIR}/ui_ConfirmationDialog.h
		${CMAKE_CURRENT_BINARY_DIR}/ui_ProgressDockWidget.h
		${CMAKE_CURRENT_BINARY_DIR}/ui_NewProjectDialog.h
		${CMAKE_CURRENT_BINARY_DIR}/ui_CameraBox.h
		${CMAKE_CURRENT_BINARY_DIR}/ui_CameraViewWidget.h
		${CMAKE_CURRENT_BINARY_DIR}/ui_SequenceNavigationFrame.h
		${CMAKE_CURRENT_BINARY_DIR}/ui_WorkspaceNavigationFrame.h
		${CMAKE_CURRENT_BINARY_DIR}/ui_WizardDockWidget.h
		${CMAKE_CURRENT_BINARY_DIR}/ui_WizardUndistortionFrame.h
		${CMAKE_CURRENT_BINARY_DIR}/ui_WizardCalibrationCubeFrame.h
		${CMAKE_CURRENT_BINARY_DIR}/ui_UndistortSequenceDialog.h
		${CMAKE_CURRENT_BINARY_DIR}/ui_SettingsDialog.h
		${CMAKE_CURRENT_BINARY_DIR}/ui_CalibrationInfoFrame.h
		${CMAKE_CURRENT_BINARY_DIR}/ui_UndistortionInfoFrame.h
		${CMAKE_CURRENT_BINARY_DIR}/ui_AboutDialog.h
		${CMAKE_CURRENT_BINARY_DIR}/ui_WizardDigitizationFrame.h
		${CMAKE_CURRENT_BINARY_DIR}/ui_ConsoleDockWidget.h
		${CMAKE_CURRENT_BINARY_DIR}/ui_CameraBoxTrial.h
		${CMAKE_CURRENT_BINARY_DIR}/ui_NewTrialDialog.h
		${CMAKE_CURRENT_BINARY_DIR}/ui_PointsDockWidget.h
		${CMAKE_CURRENT_BINARY_DIR}/ui_CameraViewDetailWidget.h
		${CMAKE_CURRENT_BINARY_DIR}/ui_DetailViewDockWidget.h
		${CMAKE_CURRENT_BINARY_DIR}/ui_ImportExportPointsDialog.h
		${CMAKE_CURRENT_BINARY_DIR}/ui_RigidBodyDialog.h
		${CMAKE_CURRENT_BINARY_DIR}/ui_MarkerDialog.h
		${CMAKE_CURRENT_BINARY_DIR}/ui_TrialDialog.h
		${CMAKE_CURRENT_BINARY_DIR}/ui_PlotWindow.h
		${CMAKE_CURRENT_BINARY_DIR}/ui_PointImportExportDialog.h
		${CMAKE_CURRENT_BINARY_DIR}/ui_OptimizationDialog.h
		${CMAKE_CURRENT_BINARY_DIR}/ui_FromToDialog.h
		${CMAKE_CURRENT_BINARY_DIR}/ui_DigitizationInfoFrame.h
		${CMAKE_CURRENT_BINARY_DIR}/ui_MetaDataInfo.h
		${CMAKE_CURRENT_BINARY_DIR}/ui_TrialSelectorDialog.h
		${CMAKE_CURRENT_BINARY_DIR}/ui_StatusColorDialog.h
)

SOURCE_GROUP(UI_Generated FILES ${XMALab_FORMS_MOC})

SET(XMALab_RESOURCE_FILES
		${CMAKE_CURRENT_BINARY_DIR}/qrc_calibrationtool_resources.cxx
		src/ui//calibrationtool_resources.qrc
)

SOURCE_GROUP(RESOURCE FILES ${XMALab_RESOURCE_FILES} src/ui/calibrationtool_resources.rc )


#SETTING UP QT FORMS
#QT4_WRAP_CPP(XMALab_HEADERS_MOC ${XMALab_HEADERS} )
QT4_WRAP_UI(XMALab_FORMS_HEADERS_GEN ${XMALab_FORMS})

SET(XMALab_RESOURCES
		src/ui/calibrationtool_resources.qrc
)
QT4_ADD_RESOURCES(XMALab_RESOURCES_RCC ${XMALab_RESOURCES})

SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin)


#BUILD AND LINK
ADD_EXECUTABLE(${PROJECT_NAME} ${GUI_TYPE}
		${XMALab_SOURCES} 
		${XMALab_HEADERS}
		${Processing_SOURCES} 
		${Processing_HEADERS}
		${External_SOURCES} 
		${External_HEADERS}
		${GL_HEADERS} 
		${GL_SOURCES}
		${XMALab_FORMS_SOURCES} 
		${XMALab_FORMS_HEADERS}
		${XMALab_FORMS_HEADERS_GEN} 
    	${XMALab_RESOURCES_RCC}
		src/ui/calibrationtool_resources.rc
)

IF (APPLE)
  add_dependencies(${PROJECT_NAME} osx_bundle_dirs )
ENDIF (APPLE)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${OpenCV_LIBS} ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES} ${QT_LIBRARIES} ${QUAZIP_LIBRARIES} ${LEVMAR_LIBRARY} ${QCUSTOMPLOT_LIBRARY})
