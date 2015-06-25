#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QFuture>

#include "ui/State.h"

namespace Ui {
	class MainWindow;
}

namespace xma{
	class Project;
	class CameraViewWidget;
	class NewProjectDialog;
	class WorldViewDockWidget;

	class MainWindow : public QMainWindow{

		Q_OBJECT
	public:
		static MainWindow* getInstance();
		virtual ~MainWindow();
		void redrawGL();
		void recountFrames();

	private:
		Ui::MainWindow *ui;
		static MainWindow* instance;

		Project * project;

		std::vector <CameraViewWidget * > cameraViews;
		void relayoutCameras();
		void clearSplitters();

		//Functions to setup and tearDown the UI
		void setupProjectUI();
		void tearDownProjectUI();

		//Project functions
		void newProject();
		void newProjectFromXMALab(QString filename);
		void loadProject();
		void closeProject();
		void saveProject();
		void saveProjectAs();

		//Trial functions
		void newTrial();
		void checkTrialImagePaths();
		void setCameraViewWidgetTitles();

		QFutureWatcher<int>* m_FutureWatcher;
		NewProjectDialog * newProjectdialog;
		WorldViewDockWidget* worldViewDockWidget;

	protected:
		MainWindow(QWidget *parent = 0);

		void resizeEvent(QResizeEvent *event);
		void closeEvent(QCloseEvent * event);
		QTimer resizeTimer;

		public slots:
		//Futures
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
		void on_actionLoad_Project_triggered(bool checked);
		void on_actionClose_Project_triggered(bool checked);
		void on_actionSave_Project_triggered(bool checked);
		void on_actionSave_Project_as_triggered(bool checked);

		//File->Export Menu Slots
		void on_actionExportAll_triggered(bool checked);

		void on_actionExport3D_Points_triggered(bool checked);
		void on_actionExport2D_Points_triggered(bool checked);
		void on_actionRigidBodyTransformations_triggered(bool checked);
		void on_actionExport_Undistorted_Trial_images_for_Maya_triggered(bool checked);
		void on_actionMayaCams_triggered(bool checked);
		void on_actionUndistort_sequence_triggered(bool checked);

		void on_actionImport2D_Points_triggered(bool checked);
		void on_actionImportTrial_triggered(bool checked);
		
		void on_actionSettings_triggered(bool checked);
		void on_actionAbout_triggered(bool checked);
		void on_action3D_world_view_triggered(bool checked);
		void on_actionConsole_triggered(bool checked);
		void on_actionDetailed_View_triggered(bool checked);
		void on_actionPlot_triggered(bool checked);

		//startMainFrameButtons
		void on_pushButtonNew_Project_clicked();
		void on_pushButtonLoad_Project_clicked();
		void on_pushButtonNewTrial_clicked();

		//resizing
		void resizeDone();
	};
}

#endif  // MAINWINDOW_H
