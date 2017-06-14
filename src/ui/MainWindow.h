//  ----------------------------------
//  XMALab -- Copyright © 2015, Brown University, Providence, RI.
//  
//  All Rights Reserved
//   
//  Use of the XMALab software is provided under the terms of the GNU General Public License version 3 
//  as published by the Free Software Foundation at http://www.gnu.org/licenses/gpl-3.0.html, provided 
//  that this copyright notice appear in all copies and that the name of Brown University not be used in 
//  advertising or publicity pertaining to the use or distribution of the software without specific written 
//  prior permission from Brown University.
//  
//  See license.txt for further information.
//  
//  BROWN UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE WHICH IS 
//  PROVIDED “AS IS”, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
//  FOR ANY PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR ANY 
//  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR FOR ANY DAMAGES WHATSOEVER RESULTING 
//  FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
//  OTHER TORTIOUS ACTION, OR ANY OTHER LEGAL THEORY, ARISING OUT OF OR IN CONNECTION 
//  WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
//  ----------------------------------
//  
///\file MainWindow.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QFuture>
#include <QSignalMapper>

#include "ui/State.h"

namespace Ui
{
	class MainWindow;
}

namespace xma
{
	class Project;
	class CameraViewWidget;
	class NewProjectDialog;
	class WorldViewDockWidget;

	class MainWindow : public QMainWindow
	{
		Q_OBJECT
	public:
		static MainWindow* getInstance();
		virtual ~MainWindow();
		void recountFrames();

		void save3DPoints(std::vector<int> markers);
		void saveRigidBodies(std::vector<int> bodies);
		void setUndo(bool value);
		void showAndLoad(QString file);
		void loadProjectFromEvent(QString filename);
        
		void setCameraVisible(int idx, bool visible);
		void relayoutCameras();

	private:
		Ui::MainWindow* ui;
		static MainWindow* instance;

		Project* project;

		std::vector<CameraViewWidget *> cameraViews;
		
		void clearSplitters();

		//Functions to setup and tearDown the UI
		void setupProjectUI();
		void tearDownProjectUI();

		//Project functions
		void newProject();
		void newProjectFromXMALab(QString filename);
		void loadProject();
		void loadProject(QString filename);
		void closeProject();
		void saveProject();
		void saveProjectAs(bool subset = false);

		void updateRecentFiles();

		//Trial functions
		void newTrial();
		void checkTrialImagePaths();
		void setCameraViewWidgetTitles();

		QFutureWatcher<int>* m_FutureWatcher;
		NewProjectDialog* newProjectdialog;
		WorldViewDockWidget* worldViewDockWidget;

		QSignalMapper * mapper;

	protected:
		MainWindow(QWidget* parent = 0);

		void resizeEvent(QResizeEvent* event);
		void closeEvent(QCloseEvent* event);
		QTimer resizeTimer;

	public slots:
		void redrawGL();

		//Futures1
		void loadProjectFinished();
		void UndistortionAfterloadProjectFinished();
		void saveProjectFinished();
		void newProjectFinished();

		//custom slots for state
		void workspaceChanged(work_state workspace);
		void displayChanged(ui_state display);
		void activeCameraChanged(int activeCamera);
		void activeFrameCalibrationChanged(int activeFrame);
		void activeFrameTrialChanged(int);
		void activeTrialChanged(int);

		//File Menu Slots
		void on_actionNew_Project_triggered(bool checked);
		void on_actionNew_trial_without_calibration_triggered(bool checked);
		void on_actionLoad_Project_triggered(bool checked);
		void on_actionClose_Project_triggered(bool checked);
		void on_actionSave_Project_triggered(bool checked);
		void on_actionSave_Project_as_triggered(bool checked);
		void on_actionSave_subdataset_as_triggered(bool checked);
		void loadRecentFile(QString filename);

		void on_actionUndo_triggered(bool checked);

		//File->Export Menu Slots
		void on_actionExportAll_triggered(bool checked);

		void on_actionExport3D_Points_triggered(bool checked);
		void on_actionExport2D_Points_triggered(bool checked);
		void on_actionExportEvents_triggered(bool checked);
		void on_actionRigidBodyTransformations_triggered(bool checked);
		void on_actionMarkertoMarkerDistances_triggered(bool checked);
		void on_actionExport_Undistorted_Trial_images_for_Maya_triggered(bool checked);
		void on_actionMayaCams_triggered(bool checked);
		void on_actionMayaCams_2_0_triggered(bool checked);
		void on_actionUndistort_sequence_triggered(bool checked);

		void on_actionImport2D_Points_triggered(bool checked);
		void on_actionImportTrial_triggered(bool checked);

		void on_actionSettings_triggered(bool checked);
		void on_actionAbout_triggered(bool checked);
		void on_actionHelp_triggered(bool checked);
		void on_action3D_world_view_triggered(bool checked);
		void on_actionConsole_triggered(bool checked);
		void on_actionDetailed_View_triggered(bool checked);
		void on_actionDisplay_Options_triggered(bool checked);
		void on_actionPlot_triggered(bool checked);
		void on_actionProject_Metadata_triggered(bool checked);
		void on_actionDetectionSettings_triggered(bool checked);
		void on_actionEvents_triggered(bool checked);

		//startMainFrameButtons
		void on_pushButtonNew_Project_clicked();
		void on_pushButtonLoad_Project_clicked();
		void on_pushButtonNewTrial_clicked();
		void on_pushButtonDefaultTrial_clicked();
		void on_pushButtonTrailWOCalibration_clicked();

		//resizing
		void resizeDone();

		void centerViews();
	};
}

#endif // MAINWINDOW_H


