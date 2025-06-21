//  ----------------------------------
//  XMALab -- Copyright � 2015, Brown University, Providence, RI.
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
//  PROVIDED �AS IS�, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
//  FOR ANY PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR ANY 
//  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR FOR ANY DAMAGES WHATSOEVER RESULTING 
//  FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
//  OTHER TORTIOUS ACTION, OR ANY OTHER LEGAL THEORY, ARISING OUT OF OR IN CONNECTION 
//  WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
//  ----------------------------------
//  
///\file MainWindow.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/MainWindow.h"
#include "ui_MainWindow.h"
#include "ui/CameraViewWidget.h"
#include "ui/GLSharedWidget.h"
#include "ui/WorkspaceNavigationFrame.h"
#include "ui_WorkspaceNavigationFrame.h"
#include "ui/SequenceNavigationFrame.h"
#include "ui_SequenceNavigationFrame.h"
#include "ui/NewProjectDialog.h"
#include "ui/NewTrialDialog.h"
#include "ui/ErrorDialog.h"
#include "ui/ConfirmationDialog.h"
#include "ui/ProjectFileIO.h"
#include "ui/ProgressDialog.h"
#include "ui/WizardDockWidget.h"
#include "ui/ConsoleDockWidget.h"
#include "ui/DetailViewDockWidget.h"
#include "ui/DisplayOptionsDockWidget.h"
#include "PointsDockWidget.h"
#include "ui/UndistortSequenceDialog.h"
#include "ui/SettingsDialog.h"
#include "ui/WorldViewDockWidget.h"
#include "ui/PlotWindow.h"
#include "ui/AboutDialog.h"
#include "ui/Shortcuts.h"
#include "ui/PointImportExportDialog.h"
#include "ui/FromToDialog.h"
#include "ui/MetaDataInfo.h"
#include "ui/TrialSelectorDialog.h"
#include "ui/HelpDialog.h"
#include "ui/DetectionSettings.h"
#include "ui/EventDockWidget.h"
#include "ui/WelcomeDialog.h"
#include "ui/TrialImportDeleteDialog.h"

#include "core/Project.h"
#include "core/Camera.h"
#include "core/Trial.h"
#include "core/CalibrationImage.h"
#include "core/UndistortionObject.h"
#include "core/Settings.h"
#include "core/HelperFunctions.h"
#include "core/CalibrationSequence.h"

#include "processing/BlobDetection.h"
#include "processing/LocalUndistortion.h"
#include "processing/MultiCameraCalibration.h"
#include "processing/ThreadScheduler.h"

#include <QSplitter>
#include <QFileDialog>
#include <QtCore>
#include <QtConcurrent/QtConcurrent>
#include <QMenu>
#include <QAction>
#include <QPalette>
#include <QStyleFactory>

#include <iostream>

#ifdef WIN32
#define OS_SEP "\\"
#else
#define OS_SEP "/"
#endif
#include <QInputDialog>
#include "ProjectOverview.h"

#ifdef __APPLE__
	#include <sys/sysctl.h>
	#include <string>
	#include <QMessageBox>
#endif

using namespace xma;


MainWindow* MainWindow::instance = NULL;


MainWindow::MainWindow(QWidget* parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	if (!instance) instance = this;

	ui->setupUi(this);

	ui->actionConsole->setVisible(false);
	ui->actionXROMM_VR->setVisible(false);
	ui->actionAll_Trials_for_External->setVisible(Settings::getInstance()->getBoolSetting("ExportAllEnabled"));

#ifdef __APPLE__
	foreach(QMenu* menu, menuBar()->findChildren<QMenu*>())
	{
		if (menu->title() == "Edit")
		{
			menu->setTitle(menu->title().prepend(QString::fromUtf8("\u200C")));
		}
		else if (menu->title() == "View")
		{
			menu->setTitle(menu->title().prepend(QString::fromUtf8("\u200C")));
		}
	}
#endif

	if (QDate::currentDate() >= QDate(2016,12,23)
		&& QDate::currentDate() <= QDate(2017, 1, 2))
	{
		ui->labelXMAS->show();
	}else
	{
		ui->labelXMAS->hide();
	}

	if (!restoreGeometry(Settings::getInstance()->getUIGeometry("XMALab")))
	{
		setGeometry(50, 50, 1280, 1024);
	}

	mapper = new QSignalMapper(this);
	connect(mapper, SIGNAL(mapped(QString)), this, SLOT(loadRecentFile(QString)));
	updateRecentFiles();


	this->statusBar()->hide();

	ui->imageMainFrame->setVisible(false);
	WorkspaceNavigationFrame::getInstance()->setObjectName(QString::fromUtf8("workspaceNavigationFrame"));
	WorkspaceNavigationFrame::getInstance()->setFrameShape(QFrame::StyledPanel);
	WorkspaceNavigationFrame::getInstance()->setFrameShadow(QFrame::Raised);

	SequenceNavigationFrame::getInstance()->setObjectName(QString::fromUtf8("sequenceNavigationFrame"));
	SequenceNavigationFrame::getInstance()->setFrameShape(QFrame::StyledPanel);
	SequenceNavigationFrame::getInstance()->setFrameShadow(QFrame::Raised);

	ui->gridLayout_2->removeWidget(ui->imageScrollArea);
	ui->gridLayout_2->addWidget(WorkspaceNavigationFrame::getInstance(), 0, 0, 1, 1);
	ui->gridLayout_2->addWidget(ui->imageScrollArea, 1, 0, 1, 1);
	ui->gridLayout_2->addWidget(SequenceNavigationFrame::getInstance(), 3, 0, 1, 1);

	WorkspaceNavigationFrame::getInstance()->setVisible(false);
	SequenceNavigationFrame::getInstance()->setVisible(false);
	ui->imageScrollArea->setVisible(false);
	ui->startTrialFrame->setVisible(false);

	ui->actionDetailed_View->setChecked(Settings::getInstance()->getBoolSetting("ShowDetailView"));
	ui->actionDetailed_View->setEnabled(false);

	ui->actionDisplay_Options->setChecked(Settings::getInstance()->getBoolSetting("ShowDisplayOptions"));
	ui->actionDisplay_Options->setEnabled(false);

	ui->actionPlot->setChecked(Settings::getInstance()->getBoolSetting("ShowPlot"));
	ui->actionPlot->setEnabled(false);

	ui->actionEvents->setChecked(Settings::getInstance()->getBoolSetting("ShowEvents"));
	ui->actionEvents->setEnabled(false);

	ui->action3D_world_view->setChecked(Settings::getInstance()->getBoolSetting("Show3DView"));
	ui->action3D_world_view->setEnabled(false);

	ui->actionConsole->setChecked(Settings::getInstance()->getBoolSetting("Console"));

	ui->actionDetectionSettings->setEnabled(false);

	project = NULL;

	resizeTimer.setSingleShot(true);
	connect(&resizeTimer, SIGNAL(timeout()), SLOT(resizeDone()));

	addDockWidget(Qt::LeftDockWidgetArea, WorldViewDockWidget::getInstance());

	WizardDockWidget::getInstance();
	ProgressDialog::getInstance();
	ConsoleDockWidget::getInstance();
	PointsDockWidget::getInstance();
	DetailViewDockWidget::getInstance();
	DisplayOptionsDockWidget::getInstance();
	PlotWindow::getInstance();
	EventDockWidget::getInstance();
	GLSharedWidget::getInstance();

	if (!restoreState(Settings::getInstance()->getUIState("XMALab"), UI_VERSION))
	{
		WizardDockWidget::getInstance()->setFloating(true);
		ProgressDialog::getInstance()->setFloating(false);
		WorldViewDockWidget::getInstance()->setFloating(true);
		ConsoleDockWidget::getInstance()->setFloating(true);
		DetailViewDockWidget::getInstance()->setFloating(false);
		DisplayOptionsDockWidget::getInstance()->setFloating(false);
		PlotWindow::getInstance()->setFloating(false);
		EventDockWidget::getInstance()->setFloating(false);
	}

	WizardDockWidget::getInstance()->hide();
	if (Settings::getInstance()->getBoolSetting("Console"))
	{
	}
	else
	{
		ConsoleDockWidget::getInstance()->hide();
	}
	WorldViewDockWidget::getInstance()->hide();
	ProgressDialog::getInstance()->hide();
	PointsDockWidget::getInstance()->hide();
	DetailViewDockWidget::getInstance()->hide();
	DisplayOptionsDockWidget::getInstance()->hide();
	PlotWindow::getInstance()->hide();
	EventDockWidget::getInstance()->hide();

	connect(State::getInstance(), SIGNAL(workspaceChanged(work_state)), this, SLOT(workspaceChanged(work_state)));
	connect(State::getInstance(), SIGNAL(displayChanged(ui_state)), this, SLOT(displayChanged(ui_state)));
	connect(State::getInstance(), SIGNAL(activeCameraChanged(int)), this, SLOT(activeCameraChanged(int)));
	connect(State::getInstance(), SIGNAL(activeFrameCalibrationChanged(int)), this, SLOT(activeFrameCalibrationChanged(int)));
	connect(State::getInstance(), SIGNAL(activeFrameTrialChanged(int)), this, SLOT(activeFrameTrialChanged(int)));
	connect(State::getInstance(), SIGNAL(activeTrialChanged(int)), this, SLOT(activeTrialChanged(int)));

	setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
	setCorner(Qt::BottomLeftCorner, Qt::BottomDockWidgetArea);
	setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
	setCorner(Qt::BottomRightCorner, Qt::BottomDockWidgetArea);

	ui->actionExport2D_Points->setEnabled(false);
	ui->actionReprojection_Errors->setEnabled(false);
	ui->actionExportEvents->setEnabled(false);
	ui->actionExport3D_Points->setEnabled(false);
	ui->actionRigidBodyTransformations->setEnabled(false);
	ui->actionMarkertoMarkerDistances->setEnabled(false);
	ui->actionImport2D_Points->setEnabled(false);
	ui->actionExport_Undistorted_Trial_images_for_Maya->setEnabled(false);
	ui->actionXROMM_VR->setEnabled(false);
	ui->actionDetailed_View->setEnabled(false);	
	ui->actionDisplay_Options->setEnabled(false);
	ui->actionPlot->setEnabled(false);
	ui->actionEvents->setEnabled(false);
	ui->action3D_world_view->setEnabled(false);
	ui->actionImportTrial->setEnabled(false);
	ui->actionPrecisionInfo->setEnabled(false);

	Shortcuts::getInstance()->bindApplicationShortcuts();
#ifndef PROJECT_BETA_VERSION
	this->setWindowTitle("XMALab " + QString(PROJECT_VERSION));
#else 
	this->setWindowTitle("XMALab " + QString(PROJECT_VERSION) + " - BETA" PROJECT_BETA_VERSION);
#endif

	if (Settings::getInstance()->getQStringSetting("WelcomeDialog") < PROJECT_VERSION)
	{
		WelcomeDialog* diag = new WelcomeDialog(this);
		diag->show();
		diag->setAttribute(Qt::WA_DeleteOnClose);
	}

	// Add Theme menu to View menu
	createThemeMenu();
}

MainWindow::~MainWindow()
{
	closeProject();
	delete GLSharedWidget::getInstance();
	delete WizardDockWidget::getInstance();
	delete WorldViewDockWidget::getInstance();
	delete mapper;
	instance = NULL;
}

MainWindow* MainWindow::getInstance()
{
	if (!instance)
	{
		instance = new MainWindow();
	}
	return instance;
}

void MainWindow::resizeDone()
{
	if (State::getInstance()->getDisplay() == ALL_CAMERAS_FULL_HEIGHT)
	{
		for (unsigned int i = 0; i < cameraViews.size(); i++)
		{
			cameraViews[i]->setMinimumWidthGL(true);
		}
	}
}

void MainWindow::centerViews()
{
	for (auto v : cameraViews)
	{
		v->centerViewToPoint();
	}
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
	resizeTimer.start(500);
	QMainWindow::resizeEvent(event);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
	if (!Settings::getInstance()->getBoolSetting("ConfirmQuitXMALab") || ConfirmationDialog::getInstance()->showConfirmationDialog("You are about to quit XMALab. Please make sure your data have been saved. \nAre you sure you want to quit?"))
	{
		Settings::getInstance()->setUIGeometry("XMALab", saveGeometry());
		Settings::getInstance()->setUIState("XMALab", saveState(UI_VERSION));
		closeProject();
		QMainWindow::closeEvent(event);
		QApplication::quit();
	}
	else
	{
		event->ignore();
	}
}

void MainWindow::recountFrames()
{
	project->recountFrames();
	SequenceNavigationFrame::getInstance()->setNbImages(project->getNbImagesCalibration());
}

void MainWindow::setupProjectUI()
{
	State::getInstance()->changeActiveCamera(0);
	State::getInstance()->changeActiveFrameCalibration(0);

	bool hasUndistorion = false;
	//int count = 0;
	for (std::vector<Camera*>::const_iterator it = project->getCameras().begin(); it != project->getCameras().end(); ++it)
	{
		CameraViewWidget* cam_widget = new CameraViewWidget((*it), this);
		cameraViews.push_back(cam_widget);
		hasUndistorion = hasUndistorion || (*it)->hasUndistortion();
		//WorkspaceNavigationFrame::getInstance()->addCamera(count, (*it)->getName());
		//count++;
		QApplication::processEvents();
	}

	ui->startMainFrame->setVisible(false);
	ui->imageMainFrame->setVisible(true);

	SequenceNavigationFrame::getInstance()->setNbImages(project->getNbImagesCalibration());

	WorkspaceNavigationFrame::getInstance()->setVisible(true);
	WorkspaceNavigationFrame::getInstance()->setUndistortionCalibration(hasUndistorion, project->getCalibration() != NO_CALIBRATION);

	if (project->getCalibration() == NO_CALIBRATION)
	{
		State::getInstance()->changeWorkspace(DIGITIZATION, true);
	}
	else if (hasUndistorion) {
		State::getInstance()->changeWorkspace(UNDISTORTION);
	}
	else
	{
		State::getInstance()->changeUndistortion(UNDISTORTED);
		State::getInstance()->changeWorkspace(CALIBRATION, true);
	}
	QApplication::processEvents();
	relayoutCameras();
	DetailViewDockWidget::getInstance()->setup();
	redrawGL();
}

void MainWindow::tearDownProjectUI()
{
	clearSplitters();
	for (unsigned int i = 0; i < cameraViews.size(); i++)
	{
		delete cameraViews[i];
	}
	cameraViews.clear();

	DetailViewDockWidget::getInstance()->clear();

	ui->startMainFrame->setVisible(true);
	ui->imageMainFrame->setVisible(false);
	ui->startTrialFrame->setVisible(false);
	WorkspaceNavigationFrame::getInstance()->setVisible(false);
	SequenceNavigationFrame::getInstance()->setVisible(false);
	WorkspaceNavigationFrame::getInstance()->closeProject();
}

void MainWindow::updateCamera(int id)
{
	DetailViewDockWidget::getInstance()->updateCamera(id);
	cameraViews[id]->updateCamera();
}

void MainWindow::clearSplitters()
{
	for (unsigned int i = 0; i < cameraViews.size(); i++)
	{
		cameraViews[i]->setParent(NULL);
	}

	QObjectList objs = ui->imageMainFrame->children();
	for (int x = 0; x < objs.size(); x++)
	{
		QObjectList objs2 = objs[x]->children();
		QSplitter* vertsplit = dynamic_cast<QSplitter*>(objs[x]);
		for (int y = objs2.size() - 1; y >= 0; y --)
		{
			QSplitter* horsplit = dynamic_cast<QSplitter*>(objs2[y]);
			if (horsplit)
			{
				QObjectList objs3 = horsplit->children();
				delete horsplit;
			}
		}

		if (vertsplit)
		{
			ui->gridLayout_4->removeWidget(vertsplit);
			delete vertsplit;
		}
	}
}

void MainWindow::relayoutCameras()
{
	clearSplitters();

	if (State::getInstance()->getDisplay() == ALL_CAMERAS_FULL_HEIGHT)
	{
		ui->imageScrollArea->setVisible(true);
		ui->imageMainFrame->setVisible(false);
		for (unsigned int i = 0; i < cameraViews.size(); i++)
		{
			if (cameraViews[i]->isVisible())
				ui->horizontalLayout->addWidget(cameraViews[i]);
		}
		resizeTimer.start(500);
	}
	else
	{
		ui->imageScrollArea->setVisible(false);
		ui->imageMainFrame->setVisible(true);
		QApplication::processEvents();

		int rows = 1;

		if (State::getInstance()->getDisplay() == ALL_CAMERAS_2ROW_SCALED)
		{
			rows = 2;
		}
		else if (State::getInstance()->getDisplay() == ALL_CAMERAS_3ROW_SCALED)
		{
			rows = 3;
		}

		int count_cams = 0;
		for (unsigned int i = 0; i < cameraViews.size(); i++)
		{
			if (cameraViews[i]->isVisible())
				count_cams++;
		}

		//Create New
		QSize cameraViewArrangement = QSize(ceil(((double) count_cams) / rows), rows);

		QSplitter* splitter = new QSplitter(this);
		splitter->setOrientation(Qt::Vertical);
		for (int i = 0; i < cameraViewArrangement.height(); i++)
		{
			QSplitter* splitterHorizontal = new QSplitter(splitter);
			splitterHorizontal->setOrientation(Qt::Horizontal);
			splitter->addWidget(splitterHorizontal);
		}

		int freeSpaces = cameraViewArrangement.height() * cameraViewArrangement.width() - count_cams;

		int count = 0;
		int count_cams2 = 0;
		for (unsigned int i = 0; i < cameraViews.size(); i++)
		{
			if (cameraViews[i]->isVisible()){
				cameraViews[i]->setMinimumWidthGL(false);
				if ((int)count_cams < count_cams2 + freeSpaces) count++;
				QObject* obj = splitter->children().at(rows - 1 - count / (cameraViewArrangement.width()));
				QSplitter* horsplit = dynamic_cast<QSplitter*>(obj);
				if (horsplit)horsplit->addWidget(cameraViews[i]);
				count_cams2++, count++;
			}
		}

		ui->gridLayout_4->addWidget(splitter, 0, 0, 1, 1);
		QApplication::processEvents();

		for (int i = 0; i < cameraViewArrangement.height(); i ++)
		{
			QObject* obj = splitter->children().at(i);
			QSplitter* horsplit = dynamic_cast<QSplitter*>(obj);
			if (horsplit)
			{
				QList<int> sizelist = horsplit->sizes();
				for (int m = 0; m < sizelist.size(); m++)
				{
					sizelist[m] = 100;
				}
				horsplit->setSizes(sizelist);
			}
		}
	}
}

//Project functions
void MainWindow::newProject()
{
	if (project)
		closeProject();

	project = Project::getInstance();

	newProjectdialog = new NewProjectDialog();

	newProjectdialog->exec();

	if (newProjectdialog->result())
	{
		m_FutureWatcher = new QFutureWatcher<int>();
		connect(m_FutureWatcher, SIGNAL( finished() ), this, SLOT( newProjectFinished() ));

		QFuture<int> future = QtConcurrent::run([this]() -> int {
			newProjectdialog->createProject();
			return 0; // Return a default value
		});
		m_FutureWatcher->setFuture(future);

		ProgressDialog::getInstance()->showProgressbar(0, 0, "Create new dataset");
	}
	else
	{
		delete newProjectdialog;
		delete project;
		project = NULL;
	}
}

void MainWindow::newProjectExternalCalibration()
{
	bool ok;
	int nbCameras = QInputDialog::getInt(this, tr("Set number of Cameras"),
		tr("Cameras:"), 2, 1, 20, 1, &ok);
	if (ok)
	{
		if (project)
			closeProject();

		project = Project::getInstance();
		project->setCalibation(EXTERNAL);

		for (int i = 0; i < nbCameras; i++)
		{
			Camera* cam = new Camera("Camera " + QString::number(i + 1), Project::getInstance()->getCameras().size());
			cam->setIsLightCamera(true);
			Project::getInstance()->addCamera(cam);
			cam->getCalibrationSequence()->loadImages(QStringList() << "");
		}

		project->loadTextures();
		setupProjectUI();
		ProgressDialog::getInstance()->closeProgressbar();

		WizardDockWidget::getInstance()->updateDialog();
		WizardDockWidget::getInstance()->show();
	}
}

void MainWindow::newProjectFinished()
{
	if (m_FutureWatcher->result() >= 0)
	{
		project->loadTextures();
		setupProjectUI();
		delete newProjectdialog;
		delete m_FutureWatcher;
		ProgressDialog::getInstance()->closeProgressbar();

		WizardDockWidget::getInstance()->updateDialog();
		WizardDockWidget::getInstance()->show();
	}
	else
	{
		delete project;
		project = NULL;

		delete newProjectdialog;
		delete m_FutureWatcher;
		ProgressDialog::getInstance()->closeProgressbar();
	}

	ProjectFileIO::getInstance()->removeTmpDir();
}

//Project functions
void MainWindow::newProjectFromXMALab(QString filename)
{
	project = Project::getInstance();

	newProjectdialog = new NewProjectDialog();

	ProjectFileIO::getInstance()->loadXMALabProject(filename, newProjectdialog);
	newProjectdialog->setupBasedOnMissingParameters();

	newProjectdialog->exec();

	if (newProjectdialog->result())
	{
		m_FutureWatcher = new QFutureWatcher<int>();
		connect(m_FutureWatcher, SIGNAL(finished()), this, SLOT(newProjectFinished()));

		QFuture<int> future = QtConcurrent::run([this]() -> int {
			newProjectdialog->createProject();
			return 0; // Return a default value
		});
		m_FutureWatcher->setFuture(future);

		ProgressDialog::getInstance()->showProgressbar(0, 0, "Create new dataset");
	}
	else
	{
		delete newProjectdialog;
		delete project;
		project = NULL;
	}

#ifndef PROJECT_BETA_VERSION
	this->setWindowTitle(Project::getInstance()->getProjectBasename() + " - XMALab " + PROJECT_VERSION);
#else 
	this->setWindowTitle(Project::getInstance()->getProjectBasename() + " - XMALab " + PROJECT_VERSION + " - BETA" PROJECT_BETA_VERSION);
#endif
}

void MainWindow::loadProject()
{
	QString fileName = QFileDialog::getOpenFileName(this,
	                                                tr("Select dataset"), Settings::getInstance()->getLastUsedDirectory(), tr("Dataset (*.xma  *.zip)"));
	if (fileName.isNull() == false)
	{
		loadProject(fileName);
	}
}

void MainWindow::loadProjectWithDifferentCalibration() {
	QString fileName_calibration = QFileDialog::getOpenFileName(this,
		tr("Select dataset for the calibration"), Settings::getInstance()->getLastUsedDirectory(), tr("Dataset (*.xma  *.zip)"));
	
	QString fileName_trials = QFileDialog::getOpenFileName(this,
		tr("Select dataset for the trials"), Settings::getInstance()->getLastUsedDirectory(), tr("Dataset (*.xma  *.zip)"));

	if (fileName_calibration.isNull() == false && fileName_trials.isNull() == false)
	{
		//replacementTrialsXMAfilename = fileName_trials;
		loadProject(fileName_trials, fileName_calibration);
	}
}

void MainWindow::loadProject(QString fileName, QString fileName_extraCalib)
{
	if (project)
		closeProject();

	project = Project::getInstance();

	State::getInstance()->setLoading(true);

	Settings::getInstance()->setLastUsedDirectory(fileName);
	Settings::getInstance()->addToRecentFiles(fileName);
	updateRecentFiles();

	m_FutureWatcher = new QFutureWatcher<int>();
	connect(m_FutureWatcher, SIGNAL(finished()), this, SLOT(loadProjectFinished()));

	QFuture<int> future = QtConcurrent::run([this, fileName, fileName_extraCalib]() -> int {
		ProjectFileIO::getInstance()->loadProject(fileName, fileName_extraCalib);
		return 0; // Return a default value
	});
	m_FutureWatcher->setFuture(future);

	ProgressDialog::getInstance()->showProgressbar(0, 0, ("Load dataset " + fileName).toUtf8());
}

void MainWindow::loadProjectFinished()
{
	checkTrialImagePaths();
	int i = m_FutureWatcher->result();
	if (m_FutureWatcher->result() == 0)
	{
		bool hasCalibration = false;
		for (auto c : Project::getInstance()->getCameras())
		{
			if (c->getCalibrationImages().size() > 0)
				hasCalibration = true;
		}

		if (!hasCalibration)
		{
			Project::getInstance()->setCalibation(NO_CALIBRATION);
			Project::getInstance()->getTrials()[0]->setCameraSizes();
		}
		
		project->loadTextures();
		for (std::vector<Trial*>::const_iterator trial = Project::getInstance()->getTrials().begin(); trial != Project::getInstance()->getTrials().end(); ++trial)
		{
			(*trial)->bindTextures();
		}
		QApplication::processEvents();
		setupProjectUI();
		QApplication::processEvents();
		delete m_FutureWatcher;
		ProgressDialog::getInstance()->closeProgressbar();
		WizardDockWidget::getInstance()->updateDialog();
		WizardDockWidget::getInstance()->show();

		for (unsigned int i = 0; i < Project::getInstance()->getCameras().size(); i++)
		{
			if (Project::getInstance()->getCameras()[i]->hasUndistortion() && Project::getInstance()->getCameras()[i]->getUndistortionObject()->isComputed())
			{
				LocalUndistortion* localUndistortion = new LocalUndistortion(i);
				connect(localUndistortion, SIGNAL(localUndistortion_finished()), this, SLOT(UndistortionAfterloadProjectFinished()));
				localUndistortion->computeUndistortion(true);
			}
			else if (!Project::getInstance()->getCameras()[i]->hasUndistortion())
			{
				Project::getInstance()->getCameras()[i]->undistort();
			}
		}
		if (!LocalUndistortion::isRunning())
		{
			MainWindow::getInstance()->redrawGL();
			UndistortionAfterloadProjectFinished();
		}

#ifndef PROJECT_BETA_VERSION
		this->setWindowTitle(Project::getInstance()->getProjectBasename() + " - XMALab " + PROJECT_VERSION);
#else 
		this->setWindowTitle(Project::getInstance()->getProjectBasename() + " - XMALab " + PROJECT_VERSION " - BETA" + PROJECT_BETA_VERSION);
#endif
	}
	else if (m_FutureWatcher->result() == 1)
	{
		State::getInstance()->setLoading(false);

		delete m_FutureWatcher;
		ProgressDialog::getInstance()->closeProgressbar();
		newProjectFromXMALab(project->getProjectFilename());
	}
	else
	{
		State::getInstance()->setLoading(false);

		delete project;
		project = NULL;

		delete m_FutureWatcher;
		ProgressDialog::getInstance()->closeProgressbar();
	}
}

void MainWindow::UndistortionAfterloadProjectFinished()
{
	if (Project::getInstance()->getCalibration() != NO_CALIBRATION){
		if (!LocalUndistortion::isRunning()){
			bool allCamerasUndistorted = true;

			for (std::vector<Camera*>::const_iterator it = Project::getInstance()->getCameras().begin(); it != Project::getInstance()->getCameras().end(); ++it)
			{
				if ((*it)->hasUndistortion())
				{
					if (!(*it)->getUndistortionObject()->isComputed())
					{
						allCamerasUndistorted = false;
					}
				}
			}
			if (allCamerasUndistorted) State::getInstance()->changeUndistortion(UNDISTORTED);

			bool allCamerasCalibrated = true;
			for (std::vector<Camera*>::const_iterator it = Project::getInstance()->getCameras().begin(); it != Project::getInstance()->getCameras().end(); ++it)
			{
				bool calibrated = false;
				for (std::vector<CalibrationImage*>::const_iterator it2 = (*it)->getCalibrationImages().begin(); it2 != (*it)->getCalibrationImages().end(); ++it2)
				{
					if ((*it2)->isCalibrated() > 0) calibrated = true;
				}
				if (calibrated)
				{
					(*it)->setCalibrated(true);
					(*it)->setRecalibrationRequired(0);
					MultiCameraCalibration::reproject((*it)->getID());
					Project::getInstance()->getCameras()[(*it)->getID()]->setUpdateInfoRequired(true);
				}
				else
				{
					allCamerasCalibrated = false;
				}
			}


			ConsoleDockWidget::getInstance()->afterLoad();

			WizardDockWidget::getInstance()->updateWizard();

			State::getInstance()->setLoading(false);

			if (allCamerasCalibrated)
			{
				ui->actionImportTrial->setEnabled(true);
				State::getInstance()->changeWorkspace(DIGITIZATION, true);
			}
			else if (State::getInstance()->getUndistortion() == UNDISTORTED){
				State::getInstance()->changeWorkspace(CALIBRATION, true);
			}

			MainWindow::getInstance()->redrawGL();
		}
	}
}

void MainWindow::closeProject()
{
	if (project)
	{
		//prompt for save
		bool saveProject = false;

		if (saveProject)
		{
			saveProjectAs();
		}

		//unlink UI and remove project
		tearDownProjectUI();
		delete project;
		project = NULL;
	}
	State::getInstance()->changeUndistortion(NOTUNDISTORTED);
	State::getInstance()->changeActiveTrial(-1);
	WizardDockWidget::getInstance()->hide();
	PointsDockWidget::getInstance()->hide();
	DetailViewDockWidget::getInstance()->hide();
	DisplayOptionsDockWidget::getInstance()->hide();
	PlotWindow::getInstance()->hide();
	EventDockWidget::getInstance()->hide();

#ifndef PROJECT_BETA_VERSION
	this->setWindowTitle("XMALab " + QString(PROJECT_VERSION));
#else 
	this->setWindowTitle("XMALab " + QString(PROJECT_VERSION) + " - BETA" + PROJECT_BETA_VERSION);
#endif

	ConsoleDockWidget::getInstance()->clear();

	ui->actionPlot->setEnabled(false);
	ui->actionEvents->setEnabled(false);
	ui->actionDetectionSettings->setEnabled(false);
	ui->action3D_world_view->setEnabled(false);
	ui->actionDetailed_View->setEnabled(false);
	ui->actionDisplay_Options->setEnabled(false);
}

void MainWindow::updateBeforeSaveProject(std::vector <Trial*> trials) {
	if (Settings::getInstance()->getBoolSetting("RecomputeWhenSaving")) {
		for (auto &trial : trials) {
			if (trial->getRequiresRecomputation()) {
				ThreadScheduler::getInstance()->updateTrialData(trial);
			}
		}
		while (ThreadScheduler::getInstance()->isRunning()) {
			QApplication::processEvents();
		}
	}
}

void MainWindow::saveProject()
{
	//if (!ConfirmationDialog::getInstance()->showConfirmationDialog("Are you sure you want to overwrite the existing File " + project->getProjectFilename() + "?")) return;

	if (WizardDockWidget::getInstance()->checkForPendingChanges())
	{
		if (Project::getInstance()->get_date_created().isEmpty())
		{
			Project::getInstance()->set_date_created();
		}

		updateBeforeSaveProject(project->getTrials());

		m_FutureWatcher = new QFutureWatcher<int>();
		connect(m_FutureWatcher, SIGNAL( finished() ), this, SLOT( saveProjectFinished() ));

		QFuture<int> future = QtConcurrent::run([this]() -> int {
			ProjectFileIO::getInstance()->saveProject(this->project->getProjectFilename(), this->project->getTrials(), false);
			return 0; // Return a default value
		});
		m_FutureWatcher->setFuture(future);

		ProgressDialog::getInstance()->showProgressbar(0, 0, ("Save dataset as " + project->getProjectFilename()).toUtf8());
	}
}

void MainWindow::saveProjectAs(bool subset)
{
	if (WizardDockWidget::getInstance()->checkForPendingChanges())
	{
		QString fileName = QFileDialog::getSaveFileName(this,
		                                                tr("Save dataset as"), project->getProjectFilename().isEmpty() ? Settings::getInstance()->getLastUsedDirectory() : project->getProjectFilename(), tr("Dataset (*.xma *.zip)"));

		ConsoleDockWidget::getInstance()->prepareSave();

		if (fileName.isNull() == false)
		{
			Project::getInstance()->set_date_created();
			Settings::getInstance()->setLastUsedDirectory(fileName);
			Settings::getInstance()->addToRecentFiles(fileName);
			updateRecentFiles();

			if (!subset){
				updateBeforeSaveProject(project->getTrials());

				m_FutureWatcher = new QFutureWatcher<int>();
				connect(m_FutureWatcher, SIGNAL(finished()), this, SLOT(saveProjectFinished()));

				QFuture<int> future = QtConcurrent::run([this, fileName]() -> int {
					ProjectFileIO::getInstance()->saveProject(fileName, this->project->getTrials(), false);
					return 0; // Return a default value
				});
				m_FutureWatcher->setFuture(future);

				ProgressDialog::getInstance()->showProgressbar(0, 0, ("Save dataset as " + fileName).toUtf8());
			}
			else
			{
				TrialSelectorDialog *diag = new TrialSelectorDialog(project->getTrials(),this);
				bool ok = diag->exec();
				if (ok)
				{
					updateBeforeSaveProject(diag->getTrials());

					m_FutureWatcher = new QFutureWatcher<int>();
					connect(m_FutureWatcher, SIGNAL(finished()), this, SLOT(saveProjectFinished()));

					QFuture<int> future = QtConcurrent::run([fileName, diag]() -> int {
						ProjectFileIO::getInstance()->saveProject(fileName, diag->getTrials(), true);
						return 0; // Return a default value
					});
					m_FutureWatcher->setFuture(future);

					ProgressDialog::getInstance()->showProgressbar(0, 0, ("Save dataset as " + fileName).toUtf8());
				}
				delete diag;
			}
		}
	}
}

void MainWindow::updateRecentFiles()
{
	ui->menuRecent_Files->clear();

	Settings::getInstance()->checkRecenFiles();
	QStringList recentFiles = Settings::getInstance()->getQStringListSetting("RecentFiles");

	for (QStringList::const_iterator iter = recentFiles.constBegin(); iter != recentFiles.constEnd(); ++iter)
	{
		QAction * action = ui->menuRecent_Files->addAction(*iter);
		QString filename = *iter; // Capture the filename for the lambda
		connect(action, &QAction::triggered, [this, filename]() {
			loadRecentFile(filename);
		});
	}
	
	QLayoutItem* item;
	while ((item = ui->gridLayout->takeAt(0)) != NULL)
	{
		delete item->widget();
		delete item;
	}

	int count = 0;
	for (QStringList::const_iterator iter = recentFiles.constBegin(); iter != recentFiles.constEnd(); ++iter)
	{
		QPushButton * button = new QPushButton(*iter, this);
		QString filename = *iter; // Capture the filename for the lambda
		connect(button, &QPushButton::clicked, [this, filename]() {
			loadRecentFile(filename);
		});
		ui->gridLayout->addWidget(button, count, 0, 1, 1);
		count++;
	}
}

void MainWindow::newTrial()
{
	NewTrialDialog* newTriaLdialog = new NewTrialDialog();
	newTriaLdialog->exec();

	if (newTriaLdialog->result())
	{
		if (!newTriaLdialog->createTrial()){
			delete newTriaLdialog;
			return;
		}
		State::getInstance()->changeActiveTrial(project->getTrials().size() - 1);
		State::getInstance()->changeActiveFrameTrial(project->getTrials()[State::getInstance()->getActiveTrial()]->getActiveFrame());
	}
	delete newTriaLdialog;
}

QString listFiles(QDir directory, QString name)
{
	QDir dir(directory);
	QFileInfoList list = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
	foreach(QFileInfo finfo, list)
		{
			if (finfo.fileName() == name)
			{
				return finfo.absolutePath();
			}
			else if (finfo.isDir())
			{
				QString path = listFiles(QDir(finfo.absoluteFilePath()), name);
				if (!path.isEmpty())
				{
					return path;
				}
			}
		}
	return "";
}

void MainWindow::checkTrialImagePaths()
{
	if (Settings::getInstance()->getBoolSetting("DisableImageSearch"))
		return;

	for (unsigned int t = 0; t < Project::getInstance()->getTrials().size(); t++)
	{
		bool requiresReload = false;

		if (Project::getInstance()->getTrials()[t]->getIsDefault())
			continue;

		for (unsigned int c = 0; c < Project::getInstance()->getCameras().size(); c++)
		{
			QString filename = littleHelper::adjustPathToOS(Project::getInstance()->getTrials()[t]->getVideoStreams()[c]->getFilenames()[0]);
			QFileInfo fileinfo(filename);
			if (!QFile::exists(filename))
			{
				//if the file does not exist we first set the nbImages by using the data in the markers if possible.
				Project::getInstance()->getTrials()[t]->setNbImagesByMarkers();


				//try to find it recursively from projectfile directory
				QFileInfo projectinfo(Project::getInstance()->getProjectFilename());
				QDir projectDir = projectinfo.absoluteDir();
				QString path = listFiles(projectDir, fileinfo.fileName());
				if (!path.isEmpty())
				{
					QString oldfolder = filename.replace(fileinfo.fileName(), "");
					Project::getInstance()->getTrials()[t]->changeImagePath(c, path + OS_SEP, oldfolder);
					Project::getInstance()->getTrials()[t]->setActiveFrame(Project::getInstance()->getTrials()[t]->getActiveFrame());
					requiresReload = true;
				}
				else
				{
					if (Project::getInstance()->getTrials()[t]->getVideoStreams()[c]->getFilenames().size() > 1)
					{
#ifdef __APPLE__
						char str[256];
						size_t size = sizeof(str);
						int ret = sysctlbyname("kern.osrelease", str, &size, NULL, 0);
						std::string s = std::string(str);
						int version = std::stoi(s.substr(0, s.find(".")));
						if (version >= 15)
						{
							QMessageBox msgBox;
							msgBox.setText(fileinfo.fileName() + " not found. Enter a new directory for " + fileinfo.fileName());
							msgBox.exec();
						}
#endif

						QString newfolder = QFileDialog::getExistingDirectory(this, fileinfo.fileName() + " not found. Enter a new directory for " + fileinfo.fileName(), Settings::getInstance()->getLastUsedDirectory());

						if (!newfolder.isEmpty())
						{
							QString oldfolder = filename.replace(fileinfo.fileName(), "");
							Project::getInstance()->getTrials()[t]->changeImagePath(c, newfolder + OS_SEP, oldfolder);
							Project::getInstance()->getTrials()[t]->setActiveFrame(Project::getInstance()->getTrials()[t]->getActiveFrame());
						} 
					}
					else
					{
#ifdef __APPLE__
						char str[256];
						size_t size = sizeof(str);
						int ret = sysctlbyname("kern.osrelease", str, &size, NULL, 0);
						std::string s = std::string(str);
						int version = std::stoi(s.substr(0, s.find(".")));
						if (version >= 15)
						{
							QMessageBox msgBox;
							msgBox.setText(fileinfo.fileName() + " not found. Enter a file for " + fileinfo.fileName());
							msgBox.exec();
						}
#endif

						QString newfolder = QFileDialog::getOpenFileName(this, fileinfo.fileName() + " not found. Enter a file for " + fileinfo.fileName(), Settings::getInstance()->getLastUsedDirectory());

						if (!newfolder.isEmpty())
						{
							Project::getInstance()->getTrials()[t]->changeImagePath(c, newfolder, filename);
							Project::getInstance()->getTrials()[t]->setActiveFrame(Project::getInstance()->getTrials()[t]->getActiveFrame());
							requiresReload = true;
						}
					}
				}
			}
		}
		if (requiresReload) Project::getInstance()->getTrials()[t]->updateAfterChangeImagePath();
	}
}

void MainWindow::saveProjectFinished()
{
	delete m_FutureWatcher;
	ProgressDialog::getInstance()->closeProgressbar();


#ifndef PROJECT_BETA_VERSION
	this->setWindowTitle(Project::getInstance()->getProjectBasename() + " - XMALab " + PROJECT_VERSION);
#else 
	this->setWindowTitle(Project::getInstance()->getProjectBasename() + " - XMALab " + PROJECT_VERSION + " - BETA" + PROJECT_BETA_VERSION);
#endif
}

void MainWindow::redrawGL()
{
	if (!State::getInstance()->getDisableDraw())
	{
		for (unsigned int i = 0; i < cameraViews.size(); i++)
		{
			cameraViews[i]->draw();
			cameraViews[i]->updateInfo();
		}
		DetailViewDockWidget::getInstance()->draw();
		PlotWindow::getInstance()->draw();
		PointsDockWidget::getInstance()->updateColor();
		if (WorldViewDockWidget::getInstance())WorldViewDockWidget::getInstance()->draw();
	}
}

void MainWindow::setUndo(bool value)
{
	ui->actionUndo->setEnabled(value);
}

void MainWindow::showAndLoad(QString file)
{
	this->show();
	loadProject(file);
}

void MainWindow::loadProjectFromEvent(QString filename)
{
	if (project && !ConfirmationDialog::getInstance()->showConfirmationDialog("You are about to close your dataset. Please make sure your data have been saved. \nAre you sure you want to close current dataset and load an existing dataset?"))
		return;
	loadProject(filename);
}

void MainWindow::setCameraVisible(int idx, bool visible)
{
	cameraViews[idx]->setIsVisible(visible);
	DetailViewDockWidget::getInstance()->setCameraVisible(idx, visible);
}

void MainWindow::setCameraViewWidgetTitles()
{
	if ((State::getInstance()->getWorkspace() == CALIBRATION) && (Project::getInstance()->getCalibration() == INTERNAL))
	{
		for (unsigned int i = 0; i < cameraViews.size(); i++)
		{
			cameraViews[i]->setImageName(Project::getInstance()->getCameras()[i]->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->getFilename());
		}
	}
	else if (State::getInstance()->getWorkspace() == UNDISTORTION)
	{
		for (unsigned int i = 0; i < cameraViews.size(); i++)
		{
			if (Project::getInstance()->getCameras()[i]->hasUndistortion())
			{
				cameraViews[i]->setImageName(Project::getInstance()->getCameras()[i]->getUndistortionObject()->getFilename());
			}
			else
			{
				cameraViews[i]->setImageName("No undistortion Grid loaded");
			}
		}
	}
	else if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		if (((int) Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0))
		{
			for (unsigned int i = 0; i < cameraViews.size(); i++)
			{
				cameraViews[i]->setImageName(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveFilename(i));
			}
		}
	}
}

/***************************************************************
UI - SLOTS
***************************************************************/

//custom slots for state
void MainWindow::workspaceChanged(work_state workspace)
{
	ui->actionUndo->setEnabled(false);
	if (project->isCalibrated() || (project->getCalibration() == NO_CALIBRATION))
	{
		ui->actionImportTrial->setEnabled(true);
	}
	else
	{
		ui->actionImportTrial->setEnabled(false);
	}

	if (workspace == UNDISTORTION)
	{
		SequenceNavigationFrame::getInstance()->setVisible(false);
		ui->imageMainFrame->setVisible(true);
		ui->startTrialFrame->setVisible(false);
		ui->actionExport2D_Points->setEnabled(false);
		ui->actionReprojection_Errors->setEnabled(false);
		ui->actionPrecisionInfo->setEnabled(false);
		ui->actionExportEvents->setEnabled(false);
		ui->actionExport3D_Points->setEnabled(false);
		ui->actionRigidBodyTransformations->setEnabled(false);
		ui->actionMarkertoMarkerDistances->setEnabled(false);
		ui->actionImport2D_Points->setEnabled(false);
		ui->actionExport_Undistorted_Trial_images_for_Maya->setEnabled(false);
		ui->actionXROMM_VR->setEnabled(false);
		ui->actionDetailed_View->setEnabled(false);
		ui->actionDisplay_Options->setEnabled(false);
		ui->actionPlot->setEnabled(false);
		ui->actionEvents->setEnabled(false);
		ui->actionDetectionSettings->setEnabled(false);
		ui->action3D_world_view->setEnabled(false);

		PointsDockWidget::getInstance()->hide();
		DetectionSettings::getInstance()->hide();
		DetailViewDockWidget::getInstance()->hide();
		DisplayOptionsDockWidget::getInstance()->hide();
		PlotWindow::getInstance()->hide();
		EventDockWidget::getInstance()->hide();
		WorldViewDockWidget::getInstance()->hide();
	}
	else if (workspace == CALIBRATION)
	{
		if (project->getNbImagesCalibration() > 1)
		{
			SequenceNavigationFrame::getInstance()->setVisible(true);
		}
		else
		{
			SequenceNavigationFrame::getInstance()->setVisible(false);
		}
		ui->imageMainFrame->setVisible(true);
		ui->startTrialFrame->setVisible(false);
		ui->actionExport2D_Points->setEnabled(false);
		ui->actionPrecisionInfo->setEnabled(false);
		ui->actionReprojection_Errors->setEnabled(false);
		ui->actionExportEvents->setEnabled(false);
		ui->actionExport3D_Points->setEnabled(false);
		ui->actionRigidBodyTransformations->setEnabled(false);
		ui->actionMarkertoMarkerDistances->setEnabled(false);
		ui->actionImport2D_Points->setEnabled(false);
		ui->actionExport_Undistorted_Trial_images_for_Maya->setEnabled(false);
		ui->actionXROMM_VR->setEnabled(false); 
		ui->actionPlot->setChecked(false);
		ui->actionEvents->setChecked(false);
		ui->actionDetectionSettings->setChecked(false);

		ui->actionPlot->setEnabled(false);
		ui->actionEvents->setEnabled(false);
		ui->actionDetectionSettings->setEnabled(false);
		ui->actionDetailed_View->setEnabled(false);
		ui->actionDisplay_Options->setEnabled(false);
		ui->action3D_world_view->setEnabled(true);

		PointsDockWidget::getInstance()->hide();
		DetailViewDockWidget::getInstance()->hide();
		DisplayOptionsDockWidget::getInstance()->hide();
		DetectionSettings::getInstance()->hide();
		PlotWindow::getInstance()->hide();
		EventDockWidget::getInstance()->hide();
		if (Settings::getInstance()->getBoolSetting("Show3DView"))
		{
			WorldViewDockWidget::getInstance()->show();
		}
		else
		{
			WorldViewDockWidget::getInstance()->hide();
		}
	}
	else if (workspace == DIGITIZATION)
	{
		if (project->getTrials().size() > 0)
		{
			ui->imageMainFrame->setVisible(true);
			ui->startTrialFrame->setVisible(false);
			SequenceNavigationFrame::getInstance()->setVisible(true);
			if (project->isCalibrated() || (project->getCalibration() == NO_CALIBRATION))
			{
				ui->labelCalibrateFirst->setVisible(false);
				ui->pushButtonNewTrial->setVisible(true);
				ui->pushButtonDefaultTrial->setVisible(true);

				ui->actionDetailed_View->setEnabled(true);
				ui->actionDisplay_Options->setEnabled(true);
				ui->actionPlot->setEnabled(true);
				ui->actionEvents->setEnabled(true);
				ui->actionDetectionSettings->setEnabled(true);
				ui->action3D_world_view->setEnabled(true);

				PointsDockWidget::getInstance()->show();

				if (Settings::getInstance()->getBoolSetting("ShowPlot"))
				{
					PlotWindow::getInstance()->show();
				}
				else
				{
					PlotWindow::getInstance()->hide();
				}

				if (Settings::getInstance()->getBoolSetting("Show3DView"))
				{
					WorldViewDockWidget::getInstance()->show();
				}
				else
				{
					WorldViewDockWidget::getInstance()->hide();
				}

				if (Settings::getInstance()->getBoolSetting("ShowDetailView"))
				{
					DetailViewDockWidget::getInstance()->show();
				}
				else
				{
					DetailViewDockWidget::getInstance()->hide();
				}

				if (Settings::getInstance()->getBoolSetting("ShowDisplayOptions"))
				{
					DisplayOptionsDockWidget::getInstance()->show();
				}
				else
				{
					DisplayOptionsDockWidget::getInstance()->hide();
				}
				if (Settings::getInstance()->getBoolSetting("ShowEvents"))
				{
					EventDockWidget::getInstance()->show();
				}
				else
				{
					EventDockWidget::getInstance()->hide();
				}
			}
			else 
			{
				ui->labelCalibrateFirst->setVisible(true);
				ui->pushButtonNewTrial->setVisible(false);
				ui->pushButtonDefaultTrial->setVisible(false);

				ui->actionDetailed_View->setEnabled(false);
				ui->actionDisplay_Options->setEnabled(false);
				ui->actionPlot->setEnabled(false);
				ui->actionEvents->setEnabled(false);
				ui->actionDetectionSettings->setEnabled(false);
				ui->action3D_world_view->setEnabled(false);

				
				PointsDockWidget::getInstance()->hide();
				DetailViewDockWidget::getInstance()->hide();
				DisplayOptionsDockWidget::getInstance()->hide();
				PlotWindow::getInstance()->hide();
				EventDockWidget::getInstance()->hide();
				DetectionSettings::getInstance()->hide();
				WorldViewDockWidget::getInstance()->hide();
			}
			ui->actionPrecisionInfo->setEnabled(true);
			ui->actionExport2D_Points->setEnabled(true);
			ui->actionReprojection_Errors->setEnabled(true);
			ui->actionExportEvents->setEnabled(true);
			ui->actionExport3D_Points->setEnabled(true);
			ui->actionRigidBodyTransformations->setEnabled(true);
			ui->actionMarkertoMarkerDistances->setEnabled(true);
			ui->actionImport2D_Points->setEnabled(true);
			ui->actionExport_Undistorted_Trial_images_for_Maya->setEnabled(true);
			ui->actionXROMM_VR->setEnabled(true);
		}
		else
		{
			ui->imageMainFrame->setVisible(false);
			ui->startTrialFrame->setVisible(true);
			SequenceNavigationFrame::getInstance()->setVisible(false);
			ui->actionPlot->setEnabled(false);
			ui->actionEvents->setEnabled(false);
			ui->actionDetectionSettings->setEnabled(false);
			if (project->isCalibrated() || (project->getCalibration() == NO_CALIBRATION))
			{
				ui->labelCalibrateFirst->setVisible(false);
				ui->pushButtonNewTrial->setVisible(true);
				ui->pushButtonDefaultTrial->setVisible(true);
			}
			else
			{
				ui->labelCalibrateFirst->setVisible(true);
				ui->pushButtonNewTrial->setVisible(false);
				ui->pushButtonDefaultTrial->setVisible(false);
			}
			ui->actionPrecisionInfo->setEnabled(false);
			ui->actionExport2D_Points->setEnabled(false);
			ui->actionReprojection_Errors->setEnabled(false);
			ui->actionExportEvents->setEnabled(false);
			ui->actionExport3D_Points->setEnabled(false);
			ui->actionRigidBodyTransformations->setEnabled(false);
			ui->actionMarkertoMarkerDistances->setEnabled(false);
			ui->actionImport2D_Points->setEnabled(false);
			ui->actionExport_Undistorted_Trial_images_for_Maya->setEnabled(false);
			ui->actionXROMM_VR->setEnabled(false);

			ui->actionDetailed_View->setEnabled(false);
			ui->actionDisplay_Options->setEnabled(false);
			ui->actionPlot->setEnabled(false);
			ui->actionEvents->setEnabled(false);
			ui->actionDetectionSettings->setEnabled(false);
			ui->actionShow_3D_View->setEnabled(false);

			PointsDockWidget::getInstance()->hide();
			DetailViewDockWidget::getInstance()->hide();
			DisplayOptionsDockWidget::getInstance()->hide();
			PlotWindow::getInstance()->hide();
			EventDockWidget::getInstance()->hide();
			DetectionSettings::getInstance()->hide();
			WorldViewDockWidget::getInstance()->hide();
		}
	}


	setCameraViewWidgetTitles();
	project->reloadTextures();
	redrawGL();
}

void MainWindow::displayChanged(ui_state display)
{
	relayoutCameras();
	redrawGL();
}

void MainWindow::activeCameraChanged(int activeCamera)
{
	redrawGL();
}

void MainWindow::activeFrameCalibrationChanged(int activeFrame)
{
	setCameraViewWidgetTitles();
	redrawGL();
}

void MainWindow::activeFrameTrialChanged(int)
{
	setCameraViewWidgetTitles();
	redrawGL();
}

void MainWindow::activeTrialChanged(int activeTrial)
{
	if (activeTrial >= 0)
	{
		ui->actionUndo->setEnabled(false);
		setCameraViewWidgetTitles();
		redrawGL();
	}

	if (DetectionSettings::getInstance()->isVisible())
		DetectionSettings::getInstance()->close();
}

//File Menu Slots
void MainWindow::on_actionNew_Project_triggered(bool checked)
{
	if (project && !ConfirmationDialog::getInstance()->showConfirmationDialog("You are about to close your dataset. Please make sure your data have been saved. \nAre you sure you want to close current dataset and create a new dataset?"))
		return;
	newProject();
}

void MainWindow::on_actionNew_dataset_external_calibration_triggered(bool checked)
{
	newProjectExternalCalibration();
}

void MainWindow::on_actionNew_trial_without_calibration_triggered(bool checked)
{
	if (project && !ConfirmationDialog::getInstance()->showConfirmationDialog("You are about to close your dataset. Please make sure your data have been saved. \nAre you sure you want to close current dataset and create a new dataset?"))
		return;
	if (!ConfirmationDialog::getInstance()->showConfirmationDialog("Are you sure you want to create a trial without calibration? There will be no possibility to add a calibration afterwards."))
		return;

	if (project)
		closeProject();

	project = Project::getInstance();

	NewTrialDialog* newTriaLdialog = new NewTrialDialog();
	newTriaLdialog->setNBCamerasVisible(); 
	newTriaLdialog->exec();
	
	if (newTriaLdialog->result())
	{
		project->setCalibation(NO_CALIBRATION);
		newTriaLdialog->createTrial();
		State::getInstance()->changeActiveTrial(project->getTrials().size() - 1);
		State::getInstance()->changeActiveFrameTrial(project->getTrials()[State::getInstance()->getActiveTrial()]->getActiveFrame());

		project->loadTextures();
		setupProjectUI();

		WizardDockWidget::getInstance()->updateDialog();
		WizardDockWidget::getInstance()->show();	
	}

	delete newTriaLdialog;
}

void MainWindow::on_actionLoad_Project_triggered(bool checked)
{
	if (project && !ConfirmationDialog::getInstance()->showConfirmationDialog("You are about to close your dataset. Please make sure your data have been saved. \nAre you sure you want to close current dataset and load an existing dataset?"))
		return;
	loadProject();
}

void MainWindow::on_actionLoad_Project_with_different_calibration_triggered(bool checked) {
	if (project && !ConfirmationDialog::getInstance()->showConfirmationDialog("You are about to close your dataset. Please make sure your data have been saved. \nAre you sure you want to close current dataset and load an existing dataset?"))
		return;
	loadProjectWithDifferentCalibration();
}

void MainWindow::on_actionClose_Project_triggered(bool checked)
{
	if (project && !ConfirmationDialog::getInstance()->showConfirmationDialog("You are about to close your dataset. Please make sure your data have been saved. \nAre you sure you want to close current dataset?"))
		return;
	closeProject();
}

void MainWindow::on_actionSave_Project_triggered(bool checked)
{
	if (project->getProjectFilename().isEmpty())
	{
		saveProjectAs(false);
	}
	else
	{
		saveProject();
	}
}

void MainWindow::on_actionSave_Project_as_triggered(bool checked)
{
	saveProjectAs(false);
}

void MainWindow::on_actionSave_subdataset_as_triggered(bool checked)
{
	saveProjectAs(true);
}

void MainWindow::loadRecentFile(QString filename)
{
	if (project && !ConfirmationDialog::getInstance()->showConfirmationDialog("You are about to close your dataset. Please make sure your data have been saved. \nAre you sure you want to close current dataset and load an existing dataset?"))
		return;
	loadProject(filename);
}

void MainWindow::on_actionUndo_triggered(bool checked)
{
	WizardDockWidget::getInstance()->undoLastPoint();
}

//File->Export Menu Slots
void MainWindow::on_actionExportAll_triggered(bool checked)
{
	if (Project::getInstance()->isCalibrated())
	{
		QString outputPath = QFileDialog::getExistingDirectory(this,
		                                                       tr("Save to Directory "), Settings::getInstance()->getLastUsedDirectory());

		if (outputPath.isNull() == false)
		{
			Project::getInstance()->exportDLT(outputPath);
			Project::getInstance()->exportMayaCam(outputPath);
			Project::getInstance()->exportLUT(outputPath);
			Settings::getInstance()->setLastUsedDirectory(outputPath, true);
		}
	}
	else
	{
		ErrorDialog::getInstance()->showErrorDialog("You have to first calibrate the cameras to be able to export");
	}
}

void MainWindow::save3DPoints(std::vector<int> markers)
{
	PointImportExportDialog* diag = new PointImportExportDialog(EXPORT3D, this);

	diag->exec();
	if (diag->result())
	{
		bool saved;
		bool filterData = ConfirmationDialog::getInstance()->showConfirmationDialog("Do you want to filter 3D Points?", true);

		if (filterData && Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRecordingSpeed() <= 0)
		{
			ErrorDialog::getInstance()->showErrorDialog("Recording speed is not set for the trial. First set the recording speed and repeat the export");
		}
		else{
			double filter_frequency = -1;
			if (filterData)
			{
				filter_frequency = QInputDialog::getDouble(this, "Enter cutoff frequency (Hz)", "", project->getTrials()[State::getInstance()->getActiveTrial()]->getCutoffFrequency(), 0.0, project->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed()*0.5);
			}
			int start = 0;
			int stop = Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getNbImages() - 1;
			if (Settings::getInstance()->getBoolSetting("Export3DOffsetCols")){
				FromToDialog* fromTo = new FromToDialog(Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getStartFrame()
					, Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getEndFrame()
					, Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getNbImages()
					, false,false, this);

				bool ok2 = fromTo->exec();
				if (ok2)
				{
					start = fromTo->getFrom() - 1;
					stop = fromTo->getTo() - 1;
				}
			}
		

			if (Settings::getInstance()->getBoolSetting("Export3DSingle"))
			{
				QString outputPath = QFileDialog::getExistingDirectory(this,
					tr("Save to Directory "), Settings::getInstance()->getLastUsedDirectory());

				if (outputPath.isNull() == false)
				{
					saved = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->save3dPoints(markers, outputPath + OS_SEP
						, Settings::getInstance()->getBoolSetting("Export3DMulti")
						, Settings::getInstance()->getBoolSetting("Export3DHeader")
						, filter_frequency
						, Settings::getInstance()->getBoolSetting("Export3DOffsetCols"), start, stop);
					Settings::getInstance()->setLastUsedDirectory(outputPath, true);
				}
			}
			else if (Settings::getInstance()->getBoolSetting("Export3DMulti"))
			{
				QString fileName = QFileDialog::getSaveFileName(this,
					tr("Save 3D points as"), Settings::getInstance()->getLastUsedDirectory(), tr("Comma seperated data (*.csv)"));

				if (fileName.isNull() == false)
				{
					saved = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->save3dPoints(markers, fileName
						, Settings::getInstance()->getBoolSetting("Export3DMulti")
						, Settings::getInstance()->getBoolSetting("Export3DHeader"),
						filter_frequency
						, Settings::getInstance()->getBoolSetting("Export3DOffsetCols"), start, stop);
					Settings::getInstance()->setLastUsedDirectory(fileName);
				}
			}
			if (!saved) ErrorDialog::getInstance()->showErrorDialog("There has been a problem saving. Please ensure that the cutoff and framerate are valid in case you exported filtered data.");
		}
	}

	delete diag;
}



void MainWindow::on_actionExport3D_Points_triggered(bool checked)
{
	std::vector<int> markers;
	for (int i = 0; i < Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers().size(); i++)
		markers.push_back(i);
	save3DPoints(markers);
}

void MainWindow::on_actionExport2D_Points_triggered(bool checked)
{
	PointImportExportDialog* diag = new PointImportExportDialog(EXPORT2D, this);

	diag->exec();

	if (diag->result())
	{
		if (Settings::getInstance()->getBoolSetting("Export2DSingle"))
		{
			QString outputPath = QFileDialog::getExistingDirectory(this, tr("Save to Directory "), Settings::getInstance()->getLastUsedDirectory());

			if (outputPath.isNull() == false)
			{
				Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->save2dPoints(outputPath + OS_SEP
				                                                                                         ,Settings::getInstance()->getBoolSetting("Export2DMulti")
				                                                                                         ,Settings::getInstance()->getBoolSetting("Export2DDistorted")
				                                                                                         ,Settings::getInstance()->getBoolSetting("Export2DCount1")
				                                                                                         ,Settings::getInstance()->getBoolSetting("Export2DYUp")
				                                                                                         ,Settings::getInstance()->getBoolSetting("Export2DHeader")
				                                                                                         ,Settings::getInstance()->getBoolSetting("Export2DOffsetCols"));
				Settings::getInstance()->setLastUsedDirectory(outputPath, true);
			}
		}
		else if (Settings::getInstance()->getBoolSetting("Export2DMulti"))
		{
			QString fileName = QFileDialog::getSaveFileName(this,
			                                                tr("Save 2D points as"), Settings::getInstance()->getLastUsedDirectory(), tr("Comma seperated data (*.csv)"));


			if (fileName.isNull() == false)
			{
				Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->save2dPoints(fileName
				                                                                                         ,Settings::getInstance()->getBoolSetting("Export2DMulti")
				                                                                                         ,Settings::getInstance()->getBoolSetting("Export2DDistorted")
				                                                                                         ,Settings::getInstance()->getBoolSetting("Export2DCount1")
				                                                                                         ,Settings::getInstance()->getBoolSetting("Export2DYUp")
				                                                                                         ,Settings::getInstance()->getBoolSetting("Export2DHeader")
				                                                                                         ,Settings::getInstance()->getBoolSetting("Export2DOffsetCols"));
				Settings::getInstance()->setLastUsedDirectory(fileName);
			}
		}
	}

	delete diag;
}

void MainWindow::on_actionExportEvents_triggered(bool checked)
{
	if (State::getInstance()->getActiveTrial() < 0 && State::getInstance()->getActiveTrial() >= Project::getInstance()->getTrials().size())
		return;
	
	QString outputPath = QFileDialog::getExistingDirectory(this, tr("Save to Directory "), Settings::getInstance()->getLastUsedDirectory());
	if (outputPath.isNull() == false)
	{
		for (auto e : Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEvents())
			e->saveData(outputPath + OS_SEP + e->getName() + ".csv");

		Settings::getInstance()->setLastUsedDirectory(outputPath, true);
	}
}

void MainWindow::saveRigidBodies(std::vector<int> bodies)
{
	PointImportExportDialog* diag = new PointImportExportDialog(EXPORTTRANS, this);

	diag->exec();
	if (diag->result())
	{
		if (Settings::getInstance()->getBoolSetting("ExportTransFiltered") && (Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getRecordingSpeed() <= 0 || Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getCutoffFrequency() <= 0))
		{
			ErrorDialog::getInstance()->showErrorDialog("Recording speed or Cutoff frequency are not set for the trial. First set the recording speed and Cutoff frequency and repeat the export");
		}
		else{
			int start = 0;
			int stop = Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getNbImages() - 1;
			if (Settings::getInstance()->getBoolSetting("ExportTransOffsetCols")){
				FromToDialog* fromTo = new FromToDialog(Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getStartFrame()
					, Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getEndFrame()
					, Project::getInstance()->getTrials()[xma::State::getInstance()->getActiveTrial()]->getNbImages()
					, false,false, this);

				bool ok2 = fromTo->exec();
				if (ok2)
				{
					start = fromTo->getFrom() - 1;
					stop = fromTo->getTo() - 1;
				}
			}

			if (Settings::getInstance()->getBoolSetting("ExportTransSingle"))
			{
				QString outputPath = QFileDialog::getExistingDirectory(this, tr("Save to Directory "), Settings::getInstance()->getLastUsedDirectory());

				if (outputPath.isNull() == false)
				{
					Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->saveRigidBodyTransformations(bodies, outputPath + OS_SEP
						, Settings::getInstance()->getBoolSetting("ExportTransMulti")
						, Settings::getInstance()->getBoolSetting("ExportTransHeader")
						, Settings::getInstance()->getBoolSetting("ExportTransFiltered")
						, Settings::getInstance()->getBoolSetting("ExportTransOffsetCols"), start, stop);
					Settings::getInstance()->setLastUsedDirectory(outputPath, true);
				}
			}
			else if (Settings::getInstance()->getBoolSetting("ExportTransMulti"))
			{
				QString fileName = QFileDialog::getSaveFileName(this,
					tr("Save Rigid Bodies as"), Settings::getInstance()->getLastUsedDirectory(), tr("Comma seperated data (*.csv)"));


				if (fileName.isNull() == false)
				{
					Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->saveRigidBodyTransformations(bodies, fileName
						, Settings::getInstance()->getBoolSetting("ExportTransMulti")
						, Settings::getInstance()->getBoolSetting("ExportTransHeader")
						, Settings::getInstance()->getBoolSetting("ExportTransFiltered")
						, Settings::getInstance()->getBoolSetting("ExportTransOffsetCols"), start, stop);
					Settings::getInstance()->setLastUsedDirectory(fileName);
				}
			}
		}
	}
	delete diag;
}

void MainWindow::on_actionRigidBodyTransformations_triggered(bool checked)
{
	std::vector<int> bodies;
	for (int i = 0; i < Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRigidBodies().size(); i++)
		bodies.push_back(i);
	saveRigidBodies(bodies);
}

void MainWindow::on_actionMarkertoMarkerDistances_triggered(bool checked)
{
	FromToDialog* fromTo = new FromToDialog(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame()
		, Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame()
		, Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getNbImages()
		, false, this);

	bool ok = fromTo->exec();
	if (ok)
	{
		QString fileName = QFileDialog::getSaveFileName(this,
			tr("Save distances as"), Settings::getInstance()->getLastUsedDirectory(), tr("Comma seperated data (*.csv)"));


		if (fileName.isNull() == false)
		{
			Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->saveMarkerToMarkerDistances(fileName,fromTo->getFrom() - 1, fromTo->getTo());
		}
	}
	delete fromTo;
}

void MainWindow::on_actionPrecisionInfo_triggered(bool checked)
{
	FromToDialog* fromTo = new FromToDialog(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame()
		, Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame()
		, Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getNbImages()
		, false,false, this);
	bool ok = fromTo->exec();
	if (ok)
	{
		QString fileName = QFileDialog::getSaveFileName(this,
			tr("Save precision info as"), Settings::getInstance()->getLastUsedDirectory(), tr("Comma seperated data (*.csv)"));


		if (fileName.isNull() == false)
		{
			if (State::getInstance()->getActiveTrial() >=0)
				Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->savePrecisionInfo(fileName, fromTo->getFrom() - 1, fromTo->getTo());
		}
	}
	delete fromTo;
}

void MainWindow::on_actionReprojection_Errors_triggered(bool checked) {
	QString outputPath = QFileDialog::getExistingDirectory(this,
		tr("Save to Directory "), Settings::getInstance()->getLastUsedDirectory());

	if (outputPath.isNull() == false)
	{
		Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->saveReprojectionErrors(outputPath + OS_SEP);
		Settings::getInstance()->setLastUsedDirectory(outputPath, true);
	}
}

void MainWindow::on_actionExport_Undistorted_Trial_images_for_Maya_triggered(bool checked)
{
	FromToDialog* fromTo = new FromToDialog(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame()
	                                       ,Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame()
	                                       ,Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getNbImages()
	                                       ,true, true, this);

	bool ok = fromTo->exec();
	if (ok)
	{
		QString outputPath = QFileDialog::getExistingDirectory(this,
		                                                       tr("Save to Directory "), Settings::getInstance()->getLastUsedDirectory());

		if (outputPath.isNull() == false)
		{
			Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->saveTrialImages(outputPath + OS_SEP, fromTo->getFrom(), fromTo->getTo(), fromTo->getFormat(), fromTo->getFiltered());
			Settings::getInstance()->setLastUsedDirectory(outputPath, true);
		}
	}

	delete fromTo;
}

void MainWindow::on_actionMayaCams_triggered(bool checked)
{
	if (Project::getInstance()->isCalibrated())
	{
		int frame = ConfirmationDialog::getInstance()->showConfirmationDialog("Do you want to save the mayacams for all calibration images? Click no if you only need the mayacams for the currently selected reference calibration", true)
			? -1 : Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getReferenceCalibrationImage();

		QString outputPath = QFileDialog::getExistingDirectory(this,
		                                                       tr("Save to Directory "), Settings::getInstance()->getLastUsedDirectory());

		if (outputPath.isNull() == false)
		{
			Project::getInstance()->exportMayaCam(outputPath, frame);
			Settings::getInstance()->setLastUsedDirectory(outputPath, true);
		}
	}
	else
	{
		ErrorDialog::getInstance()->showErrorDialog("You have to first calibrate the cameras to be able to export");
	}
}

void MainWindow::on_actionMayaCams_2_0_triggered(bool checked)
{
	if (Project::getInstance()->isCalibrated())
	{
		int frame = ConfirmationDialog::getInstance()->showConfirmationDialog("Do you want to save the mayacams for all calibration images? Click no if you only need the mayacams for the currently selected reference calibration", true)
			? -1 : Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getReferenceCalibrationImage();

		QString outputPath = QFileDialog::getExistingDirectory(this,
		                                                       tr("Save to Directory "), Settings::getInstance()->getLastUsedDirectory());

		if (outputPath.isNull() == false)
		{
			Project::getInstance()->exportMayaCamVersion2(outputPath, frame);
			Settings::getInstance()->setLastUsedDirectory(outputPath, true);
		}
	}
	else
	{
		ErrorDialog::getInstance()->showErrorDialog("You have to first calibrate the cameras to be able to export");
	}
}

void MainWindow::on_actionUndistort_sequence_triggered(bool checked)
{
	UndistortSequenceDialog* diag = new UndistortSequenceDialog(this);
	diag->exec();
	delete diag;
}

void MainWindow::on_actionAll_Trials_for_External_triggered(bool checked)
{
	if (project == NULL)
		return;

	QString outputPath = QFileDialog::getExistingDirectory(this,
		tr("Save to Directory "), Settings::getInstance()->getLastUsedDirectory());

	if (outputPath.isNull() == false)
	{
		ProgressDialog::getInstance()->showProgressbar(0, 0, "Exporting all for external processing");
		for (auto &tr : Project::getInstance()->getTrials())
		{
			QString trial_path = outputPath + OS_SEP + tr->getName() + OS_SEP;
			QDir().mkpath(trial_path);
			
			for (int i = 0; i < project->getCameras().size(); i++){
				QString cam_path = trial_path + OS_SEP + "Camera " + QString::number(i) + OS_SEP;
				QDir().mkpath(cam_path);

				QDir().mkpath(cam_path + OS_SEP + "calib" + OS_SEP);
				Project::getInstance()->exportMayaCamVersion2(cam_path + OS_SEP + "calib" + OS_SEP, -1, i);
				QDir().mkpath(cam_path + OS_SEP + "images" + OS_SEP);
				tr->saveTrialImages(cam_path + OS_SEP + "images" + OS_SEP, 1, tr->getVideoStreams()[i]->getNbImages(), "tif", i);
				QDir().mkpath(cam_path + OS_SEP + "markers" + OS_SEP);
				tr->save2dPoints(cam_path + OS_SEP + "markers" + OS_SEP, false, false, false, false, false, false, i);
				QDir().mkpath(cam_path + OS_SEP + "reprojection_errors" + OS_SEP);
				tr->saveReprojectionErrors(cam_path + OS_SEP + "reprojection_errors" + OS_SEP);
			}
		}
		ProgressDialog::getInstance()->closeProgressbar();
	}
}

void MainWindow::on_actionImport2D_Points_triggered(bool checked)
{
	PointImportExportDialog* diag = new PointImportExportDialog(IMPORT2D, this);

	int loadPoints = 0;

	diag->exec();
	if (diag->result())
	{
		QStringList fileNames = QFileDialog::getOpenFileNames(this,
		                                                      tr("Open csv files for 2D points"), Settings::getInstance()->getLastUsedDirectory(), tr("Comma seperated data (*.csv)"));

		if (!fileNames.isEmpty())
		{
			ProgressDialog::getInstance()->showProgressbar(0, 0, "Importing points");
			for (int i = 0; i < fileNames.size(); i++)
			{
				loadPoints += Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->load2dPoints(fileNames.at(i)
				                                                                                                       ,Settings::getInstance()->getBoolSetting("Import2DDistorted")
				                                                                                                       ,Settings::getInstance()->getBoolSetting("Import2DCount1")
				                                                                                                       ,Settings::getInstance()->getBoolSetting("Import2DYUp")
				                                                                                                       ,Settings::getInstance()->getBoolSetting("Import2DHeader")
				                                                                                                       ,Settings::getInstance()->getBoolSetting("Import2DOffsetCols")
																													   , Settings::getInstance()->getBoolSetting("ImportStatusSet"));
			}
			ProgressDialog::getInstance()->closeProgressbar();
			Settings::getInstance()->setLastUsedDirectory(fileNames.at(0));
		}

		redrawGL();
		PointsDockWidget::getInstance()->reloadListFromObject();
		PlotWindow::getInstance()->updateMarkers(true);
	}
	delete diag;
}

void MainWindow::on_actionImportTrial_triggered(bool checked)
{
	QString fileName = QFileDialog::getOpenFileName(this,
	                                                tr("Select dataset"), Settings::getInstance()->getLastUsedDirectory(), tr("Dataset (*.xma  *.zip)"));

	if (fileName.isNull() == false)
	{
		Settings::getInstance()->setLastUsedDirectory(fileName);
		QStringList trialnames = ProjectFileIO::getInstance()->readTrials(fileName);
		
		TrialImportDeleteDialog* diag = new TrialImportDeleteDialog(trialnames, false, this);
		diag->exec();
		QStringList selected = diag->getSelected();

		if (diag->result() == QDialog::Accepted && !selected.empty())
		{
			for (auto &item : selected){
				Trial* trial = ProjectFileIO::getInstance()->loadTrials(fileName, item);
				trial->setRequiresRecomputation(true);
				if (trial->getIsDefault() && project->hasDefaultTrial())
				{
					if (ConfirmationDialog::getInstance()->showConfirmationDialog("You are about to replace your current Default trial. Are you sure you want to update it?")){
						Project::getInstance()->replaceTrial(Project::getInstance()->getDefaultTrail(), trial);
					}
					else
					{
						delete trial;
						return;
					}
				}
				else
				{
					Project::getInstance()->addTrial(trial);
					WorkspaceNavigationFrame::getInstance()->addTrial(item);
				}
			
			
				checkTrialImagePaths();
				trial->bindTextures();

				if (trial->getIsDefault())
				{
					PointsDockWidget::getInstance()->on_pushButtonApply_clicked();
					PointsDockWidget::getInstance()->reloadListFromObject();
				}

				if (State::getInstance()->getWorkspace() == DIGITIZATION)
				{
					State::getInstance()->changeWorkspace(DIGITIZATION, true);
				}
			}
		}
		delete diag;
	}
}

void MainWindow::on_actionDelete_multiple_trials_triggered(bool checked)
{
	QStringList trialnames;

	for (auto &tr : project->getTrials())
		trialnames << tr->getName();
	TrialImportDeleteDialog* diag = new TrialImportDeleteDialog(trialnames, true, this);
	diag->exec();
	QStringList selected = diag->getSelected();
	if (diag->result() == QDialog::Accepted && !selected.empty())
	{
		QString text_message = "Are you sure you want to delete the following trials?\n";
		for (auto &str : selected)
			text_message += (str + QString("\n"));
		if (ConfirmationDialog::getInstance()->showConfirmationDialog(text_message))
		{
			for (auto &item : selected){
				Trial* trial = Project::getInstance()->getTrialByName(item);
				Project::getInstance()->deleteTrial(trial);
				WorkspaceNavigationFrame::getInstance()->removeTrial(trial->getName());
			}

			if (State::getInstance()->getActiveTrial() >= (int)Project::getInstance()->getTrials().size())
				State::getInstance()->changeActiveTrial(Project::getInstance()->getTrials().size() - 1, true);

			if (State::getInstance()->getActiveTrial() == -1)
			{
				State::getInstance()->changeWorkspace(DIGITIZATION, true);
				
				if (Project::getInstance()->isCalibrated() || (Project::getInstance()->getCalibration() == NO_CALIBRATION))
				{
					WorkspaceNavigationFrame::getInstance()->setTrialVisible(true);
				}
				else
				{
					WorkspaceNavigationFrame::getInstance()->setTrialVisible(false);
				}
			}

		}
	}
}

void MainWindow::on_actionSettings_triggered(bool checked)
{
	SettingsDialog* diag = new SettingsDialog(this);
	diag->exec();
	delete diag;
}


void MainWindow::on_actionAbout_triggered(bool checked)
{
	AboutDialog* diag = new AboutDialog(this);
	diag->exec();
	delete diag;
}

void MainWindow::on_actionHelp_triggered(bool checked)
{
	HelpDialog* diag = new HelpDialog(this);
	diag->show();
	diag->setAttribute(Qt::WA_DeleteOnClose);
	//delete diag;
}

//startMainFrameButtons
void MainWindow::on_pushButtonNew_Project_clicked()
{
	newProject();
}

void MainWindow::on_pushButtonLoad_Project_clicked()
{
	loadProject();
}

void  MainWindow::on_pushButtonLoad_Project_with_different_Calbration_clicked() {
	loadProjectWithDifferentCalibration();
}

void MainWindow::on_pushButtonNewTrial_clicked()
{
	newTrial();
}

void MainWindow::on_pushButtonDefaultTrial_clicked()
{
	NewTrialDialog* newTriaLdialog = new NewTrialDialog();
	newTriaLdialog->on_pushButton_Default_clicked();
	State::getInstance()->changeActiveTrial(project->getTrials().size() - 1);
	State::getInstance()->changeActiveFrameTrial(project->getTrials()[State::getInstance()->getActiveTrial()]->getActiveFrame());
	delete newTriaLdialog;
}

void MainWindow::on_pushButtonTrailWOCalibration_clicked()
{
	on_actionNew_trial_without_calibration_triggered(false);
}

void MainWindow::on_pushButtonExternalCalibration_clicked()
{
	newProjectExternalCalibration();
}

void MainWindow::on_action3D_world_view_triggered(bool checked)
{
	if (checked)
	{
		WorldViewDockWidget::getInstance()->show();
		Settings::getInstance()->set("Show3DView", true);
	}
	else
	{
		WorldViewDockWidget::getInstance()->hide();
		Settings::getInstance()->set("Show3DView", false);
		ui->action3D_world_view->setChecked(false);
	}
}

void MainWindow::on_actionConsole_triggered(bool checked)
{
	if (checked)
	{
		ConsoleDockWidget::getInstance()->show();
		Settings::getInstance()->set("Console", true);
	}
	else
	{
		ConsoleDockWidget::getInstance()->hide();
		ui->actionConsole->setChecked(false);
		Settings::getInstance()->set("Console", false);
	}
}

void MainWindow::on_actionDetailed_View_triggered(bool checked)
{
	if (checked)
	{
		DetailViewDockWidget::getInstance()->show();
		Settings::getInstance()->set("ShowDetailView", true);
	}
	else
	{
		ui->actionDetailed_View->setChecked(false);
		DetailViewDockWidget::getInstance()->hide();
		Settings::getInstance()->set("ShowDetailView", false);
	}
}

void MainWindow::on_actionDisplay_Options_triggered(bool checked)
{
	if (checked)
	{
		DisplayOptionsDockWidget::getInstance()->show();
		Settings::getInstance()->set("ShowDisplayOptions", true);
	}
	else
	{
		ui->actionDisplay_Options->setChecked(false);
		DisplayOptionsDockWidget::getInstance()->hide();
		Settings::getInstance()->set("ShowDisplayOptions", false);
	}
}

void MainWindow::on_actionPlot_triggered(bool checked)
{
	if (checked)
	{
		PlotWindow::getInstance()->show();
		PlotWindow::getInstance()->updateMarkers(false);
		PlotWindow::getInstance()->draw();
		Settings::getInstance()->set("ShowPlot", true);
	}
	else
	{
		ui->actionPlot->setChecked(false);
		PlotWindow::getInstance()->hide();
		Settings::getInstance()->set("ShowPlot", false);
	}
}

void MainWindow::on_actionProject_Metadata_triggered(bool checked)
{
	MetaDataInfo* info = new MetaDataInfo(this);
	info->show();
	info->setAttribute(Qt::WA_DeleteOnClose);
}

void MainWindow::on_actionProject_Overview_triggered(bool checked)
{
	ProjectOverview* overview = new ProjectOverview(this);
	overview->show();
	overview->setAttribute(Qt::WA_DeleteOnClose);
}

void MainWindow::on_actionDetectionSettings_triggered(bool checked)
{
	if (checked)
	{
		DetectionSettings::getInstance()->show();
	} 
	else
	{
		ui->actionDetectionSettings->setChecked(false);
		DetectionSettings::getInstance()->hide();
	}
}

void MainWindow::on_actionEvents_triggered(bool checked)
{
	if (checked)
	{
		EventDockWidget::getInstance()->show();
		Settings::getInstance()->set("ShowEvents", true);
	}
	else
	{
		ui->actionEvents->setChecked(false);
		EventDockWidget::getInstance()->hide();
		Settings::getInstance()->set("ShowEvents", false);
	}
}

void MainWindow::on_actionXROMM_VR_triggered(bool checked)
{
	QString outputPath = QFileDialog::getExistingDirectory(this,
		tr("Save to Directory "), Settings::getInstance()->getLastUsedDirectory());

	if (outputPath.isNull() == false)
	{
		Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->saveVR(outputPath + OS_SEP);
	}
}

void MainWindow::createThemeMenu() {
    themeMenu = new QMenu(tr("Theme"), this);
    actionThemeLight = new QAction(tr("Light"), this);
    actionThemeDark = new QAction(tr("Dark"), this);
    actionThemeSystem = new QAction(tr("Follow System"), this);
    actionThemeLight->setCheckable(true);
    actionThemeDark->setCheckable(true);
    actionThemeSystem->setCheckable(true);
    QActionGroup* group = new QActionGroup(this);
    group->addAction(actionThemeLight);
    group->addAction(actionThemeDark);
    group->addAction(actionThemeSystem);
    themeMenu->addAction(actionThemeLight);
    themeMenu->addAction(actionThemeDark);
    themeMenu->addAction(actionThemeSystem);
    ui->menuView->addMenu(themeMenu);    connect(actionThemeLight, &QAction::triggered, this, &MainWindow::onThemeLight);
    connect(actionThemeDark, &QAction::triggered, this, &MainWindow::onThemeDark);
    connect(actionThemeSystem, &QAction::triggered, this, &MainWindow::onThemeSystem);
    
    // Load and apply saved theme preference
    QString savedTheme = Settings::getInstance()->getQStringSetting("Theme");    if (savedTheme.isEmpty()) {
        savedTheme = "light"; // Default to light theme
    }
      if (savedTheme == "light") {
        actionThemeLight->setChecked(true);
        applyTheme("light");
    } else if (savedTheme == "dark") {
        actionThemeDark->setChecked(true);
        applyTheme("dark");
    } else {
        actionThemeSystem->setChecked(true);
        applyTheme("system");
    }
    
    // Ensure the theme setting is saved after applying it
    Settings::getInstance()->set("Theme", savedTheme);
}

void MainWindow::applyTheme(const QString& themeName) {
    QPalette palette;
    if (themeName == "dark") {
        palette = QStyleFactory::create("Fusion")->standardPalette();
        palette.setColor(QPalette::Window, QColor(53,53,53));
        palette.setColor(QPalette::WindowText, Qt::white);
        palette.setColor(QPalette::Base, QColor(42,42,42));
        palette.setColor(QPalette::AlternateBase, QColor(66,66,66));
        palette.setColor(QPalette::ToolTipBase, Qt::white);
        palette.setColor(QPalette::ToolTipText, Qt::white);
        palette.setColor(QPalette::Text, Qt::white);
        palette.setColor(QPalette::Button, QColor(53,53,53));
        palette.setColor(QPalette::ButtonText, Qt::white);
        palette.setColor(QPalette::BrightText, Qt::red);
        palette.setColor(QPalette::Link, QColor(42, 130, 218));
        palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
        palette.setColor(QPalette::HighlightedText, Qt::black);
        qApp->setPalette(palette);
    } else if (themeName == "light") {
        palette = QStyleFactory::create("Fusion")->standardPalette();
        qApp->setPalette(palette);
    } else { // system
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
        qApp->setPalette(QPalette()); // Let Qt use system palette
#else
        palette = QStyleFactory::create("Fusion")->standardPalette();
        qApp->setPalette(palette);
#endif
    }
}

void MainWindow::onThemeLight() {
    applyTheme("light");
    Settings::getInstance()->set("Theme", "light");
}
void MainWindow::onThemeDark() {
    applyTheme("dark");
    Settings::getInstance()->set("Theme", "dark");
}
void MainWindow::onThemeSystem() {
    applyTheme("system");
    Settings::getInstance()->set("Theme", "system");
}
