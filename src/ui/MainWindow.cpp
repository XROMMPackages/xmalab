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

#include "core/Project.h"
#include "core/Camera.h"
#include "core/Trial.h"
#include "core/CalibrationImage.h"
#include "core/UndistortionObject.h"
#include "core/Settings.h"
#include "core/HelperFunctions.h"

#include "processing/BlobDetection.h"
#include "processing/LocalUndistortion.h"
#include "processing/MultiCameraCalibration.h"

#include <QSplitter>
#include <QFileDialog>
#include <QtCore>


#include <iostream>

#ifdef WIN32
#define OS_SEP "\\"
#else
#define OS_SEP "/"
#endif
#include <QtGui/QInputDialog>

#ifdef __APPLE__
	#include <sys/sysctl.h>
	#include <string>
	#include <QMessageBox>
#endif


//#define BETA 1

using namespace xma;

MainWindow* MainWindow::instance = NULL;

MainWindow::MainWindow(QWidget* parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	//To prevent endless looping when accesing MainWindow::getInstance in constructors
	worldViewDockWidget = NULL;
	if (!instance) instance = this;

	ui->setupUi(this);
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

	ui->actionPlot->setChecked(Settings::getInstance()->getBoolSetting("ShowPlot"));
	ui->actionPlot->setEnabled(false);

	ui->action3D_world_view->setChecked(Settings::getInstance()->getBoolSetting("Show3DView"));
	ui->action3D_world_view->setEnabled(false);

	ui->actionConsole->setChecked(Settings::getInstance()->getBoolSetting("Console"));

	project = NULL;

	resizeTimer.setSingleShot(true);
	connect(&resizeTimer, SIGNAL(timeout()), SLOT(resizeDone()));

	GLSharedWidget::getInstance()->makeGLCurrent();
	worldViewDockWidget = new WorldViewDockWidget(this);
	addDockWidget(Qt::LeftDockWidgetArea, worldViewDockWidget);
	worldViewDockWidget->setSharedGLContext(GLSharedWidget::getInstance()->getQGLContext());

	WizardDockWidget::getInstance();
	ProgressDialog::getInstance();
	ConsoleDockWidget::getInstance();
	PointsDockWidget::getInstance();
	DetailViewDockWidget::getInstance();
	PlotWindow::getInstance();

	restoreGeometry(Settings::getInstance()->getUIGeometry("XMALab"));
	if (Settings::getInstance()->getUIState("XMALab").size() < 0 ||
		!restoreState(Settings::getInstance()->getUIState("XMALab"), UI_VERSION))
	{
		WizardDockWidget::getInstance()->setFloating(true);
		ProgressDialog::getInstance()->setFloating(true);
		worldViewDockWidget->setFloating(true);
		ConsoleDockWidget::getInstance()->setFloating(true);
		DetailViewDockWidget::getInstance()->setFloating(true);
		PlotWindow::getInstance()->setFloating(true);
	}

	WizardDockWidget::getInstance()->hide();
	if (Settings::getInstance()->getBoolSetting("Console"))
	{
	}
	else
	{
		ConsoleDockWidget::getInstance()->hide();
	}
	worldViewDockWidget->hide();
	ProgressDialog::getInstance()->hide();
	PointsDockWidget::getInstance()->hide();
	DetailViewDockWidget::getInstance()->hide();
	PlotWindow::getInstance()->hide();

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
	ui->actionExport3D_Points->setEnabled(false);
	ui->actionRigidBodyTransformations->setEnabled(false);
	ui->actionMarkertoMarkerDistances->setEnabled(false);
	ui->actionImport2D_Points->setEnabled(false);
	ui->actionExport_Undistorted_Trial_images_for_Maya->setEnabled(false);
	ui->actionDetailed_View->setEnabled(false);
	ui->actionPlot->setEnabled(false);
	ui->action3D_world_view->setEnabled(false);
	ui->actionImportTrial->setEnabled(false);
	Shortcuts::getInstance()->bindApplicationShortcuts();
#ifndef BETA
	this->setWindowTitle("XMALab " + QString(PROJECT_VERSION));
#else 
	this->setWindowTitle("XMALab " + QString(PROJECT_VERSION) + " - BETA");
#endif
}


MainWindow::~MainWindow()
{
	closeProject();
	delete GLSharedWidget::getInstance();
	delete WizardDockWidget::getInstance();
	delete worldViewDockWidget;
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
	int count = 0;
	for (std::vector<Camera*>::const_iterator it = project->getCameras().begin(); it != project->getCameras().end(); ++it)
	{
		CameraViewWidget* cam_widget = new CameraViewWidget((*it), this);
		cam_widget->setSharedGLContext(GLSharedWidget::getInstance()->getQGLContext());
		cameraViews.push_back(cam_widget);
		hasUndistorion = hasUndistorion || (*it)->hasUndistortion();
		WorkspaceNavigationFrame::getInstance()->addCamera(count, (*it)->getName());
		count++;
		QApplication::processEvents();
	}

	ui->startMainFrame->setVisible(false);
	ui->imageMainFrame->setVisible(true);

	SequenceNavigationFrame::getInstance()->setNbImages(project->getNbImagesCalibration());

	WorkspaceNavigationFrame::getInstance()->setVisible(true);
	WorkspaceNavigationFrame::getInstance()->setUndistortion(hasUndistorion);

	if (hasUndistorion) State::getInstance()->changeWorkspace(UNDISTORTION);
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
		WorkspaceNavigationFrame::getInstance()->removeCamera(0);
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

	if (State::getInstance()->getDisplay() == SINGLE_CAMERA)
	{
		ui->imageScrollArea->setVisible(false);
		ui->imageMainFrame->setVisible(true);

		for (unsigned int i = 0; i < cameraViews.size(); i++)
		{
			cameraViews[i]->setMinimumWidthGL(false);
		}
		ui->gridLayout_4->addWidget(cameraViews[State::getInstance()->getActiveCamera()]);
	}
	else if (State::getInstance()->getDisplay() == ALL_CAMERAS_FULL_HEIGHT)
	{
		ui->imageScrollArea->setVisible(true);
		ui->imageMainFrame->setVisible(false);
		for (unsigned int i = 0; i < cameraViews.size(); i++)
		{
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

		//Create New
		QSize cameraViewArrangement = QSize(ceil(((double) cameraViews.size()) / rows), rows);

		QSplitter* splitter = new QSplitter(this);
		splitter->setOrientation(Qt::Vertical);
		for (int i = 0; i < cameraViewArrangement.height(); i ++)
		{
			QSplitter* splitterHorizontal = new QSplitter(splitter);
			splitterHorizontal->setOrientation(Qt::Horizontal);
			splitter->addWidget(splitterHorizontal);
		}

		int freeSpaces = cameraViewArrangement.height() * cameraViewArrangement.width() - cameraViews.size();

		int count = 0;
		for (unsigned int i = 0; i < cameraViews.size(); i++, count++)
		{
			cameraViews[i]->setMinimumWidthGL(false);
			if ((int)cameraViews.size() < i + freeSpaces) count++;
			QObject* obj = splitter->children().at(rows - 1 - count / (cameraViewArrangement.width()));
			QSplitter* horsplit = dynamic_cast<QSplitter*>(obj);
			if (horsplit)horsplit->addWidget(cameraViews[i]);
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

		QFuture<int> future = QtConcurrent::run(newProjectdialog, &NewProjectDialog::createProject);
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

	newProjectdialog->exec();

	if (newProjectdialog->result())
	{
		m_FutureWatcher = new QFutureWatcher<int>();
		connect(m_FutureWatcher, SIGNAL(finished()), this, SLOT(newProjectFinished()));

		QFuture<int> future = QtConcurrent::run(newProjectdialog, &NewProjectDialog::createProject);
		m_FutureWatcher->setFuture(future);

		ProgressDialog::getInstance()->showProgressbar(0, 0, "Create new dataset");
	}
	else
	{
		delete newProjectdialog;
		delete project;
		project = NULL;
	}

#ifndef BETA
	this->setWindowTitle(Project::getInstance()->getProjectBasename() + " - XMALab " + PROJECT_VERSION);
#else 
	this->setWindowTitle(Project::getInstance()->getProjectBasename() + " - XMALab " + PROJECT_VERSION + " - BETA");
#endif
}

void MainWindow::loadProject()
{
	if (project)
		closeProject();

	project = Project::getInstance();

	QString fileName = QFileDialog::getOpenFileName(this,
	                                                tr("Select dataset"), Settings::getInstance()->getLastUsedDirectory(), tr("Dataset (*.xma  *.zip)"));
	if (fileName.isNull() == false)
	{
		State::getInstance()->setLoading(true);

		Settings::getInstance()->setLastUsedDirectory(fileName);

		m_FutureWatcher = new QFutureWatcher<int>();
		connect(m_FutureWatcher, SIGNAL( finished() ), this, SLOT( loadProjectFinished() ));

		QFuture<int> future = QtConcurrent::run(ProjectFileIO::getInstance(), &ProjectFileIO::loadProject, fileName);
		m_FutureWatcher->setFuture(future);

		ProgressDialog::getInstance()->showProgressbar(0, 0, ("Load dataset " + fileName).toAscii().data());
	}
}

void MainWindow::loadProjectFinished()
{
	checkTrialImagePaths();

	if (m_FutureWatcher->result() == 0)
	{
		project->loadTextures();
		for (std::vector<Trial*>::const_iterator trial = Project::getInstance()->getTrials().begin(); trial != Project::getInstance()->getTrials().end(); ++trial)
		{
			(*trial)->bindTextures();
		}

		setupProjectUI();

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

#ifndef BETA
		this->setWindowTitle(Project::getInstance()->getProjectBasename() + " - XMALab " + PROJECT_VERSION);
#else 
		this->setWindowTitle(Project::getInstance()->getProjectBasename() + " - XMALab " + PROJECT_VERSION " - BETA");
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
				if ((*it2)->isCalibrated()) calibrated = true;
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
		if (allCamerasCalibrated)
		{
			ui->actionImportTrial->setEnabled(true);
			State::getInstance()->changeWorkspace(DIGITIZATION, true);
		}
		else if (State::getInstance()->getUndistortion() == UNDISTORTED){
			State::getInstance()->changeWorkspace(CALIBRATION, true);
		}

		ConsoleDockWidget::getInstance()->afterLoad();

		WizardDockWidget::getInstance()->updateWizard();

		State::getInstance()->setLoading(false);

		MainWindow::getInstance()->redrawGL();
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

#ifndef BETA
	this->setWindowTitle("XMALab " + QString(PROJECT_VERSION));
#else 
	this->setWindowTitle("XMALab " + QString(PROJECT_VERSION) + " - BETA");
#endif

	ConsoleDockWidget::getInstance()->clear();
	ui->actionPlot->setChecked(false);
	PlotWindow::getInstance()->hide();
}

void MainWindow::saveProject()
{
	if (WizardDockWidget::getInstance()->checkForPendingChanges())
	{
		m_FutureWatcher = new QFutureWatcher<int>();
		connect(m_FutureWatcher, SIGNAL( finished() ), this, SLOT( saveProjectFinished() ));

		QFuture<int> future = QtConcurrent::run(ProjectFileIO::getInstance(), &ProjectFileIO::saveProject, project->getProjectFilename(), project->getTrials(), false);
		m_FutureWatcher->setFuture(future);

		ProgressDialog::getInstance()->showProgressbar(0, 0, ("Save dataset as " + project->getProjectFilename()).toAscii().data());
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
			Settings::getInstance()->setLastUsedDirectory(fileName);

			if (!subset){
				m_FutureWatcher = new QFutureWatcher<int>();
				connect(m_FutureWatcher, SIGNAL(finished()), this, SLOT(saveProjectFinished()));

				QFuture<int> future = QtConcurrent::run(ProjectFileIO::getInstance(), &ProjectFileIO::saveProject, fileName, project->getTrials(), false);
				m_FutureWatcher->setFuture(future);

				ProgressDialog::getInstance()->showProgressbar(0, 0, ("Save dataset as " + fileName).toAscii().data());
			}
			else
			{
				TrialSelectorDialog *diag = new TrialSelectorDialog(project->getTrials(),this);
				bool ok = diag->exec();
				if (ok)
				{
					m_FutureWatcher = new QFutureWatcher<int>();
					connect(m_FutureWatcher, SIGNAL(finished()), this, SLOT(saveProjectFinished()));

					QFuture<int> future = QtConcurrent::run(ProjectFileIO::getInstance(), &ProjectFileIO::saveProject, fileName, diag->getTrials(), true);
					m_FutureWatcher->setFuture(future);

					ProgressDialog::getInstance()->showProgressbar(0, 0, ("Save dataset as " + fileName).toAscii().data());
				}
				delete diag;
			}
		}
	}
}

void MainWindow::newTrial()
{
	NewTrialDialog* newTriaLdialog = new NewTrialDialog();
	newTriaLdialog->exec();

	if (newTriaLdialog->result())
	{
		newTriaLdialog->createTrial();
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
	for (unsigned int t = 0; t < Project::getInstance()->getTrials().size(); t++)
	{
		bool requiresReload = false;
		for (unsigned int c = 0; c < Project::getInstance()->getCameras().size(); c++)
		{
			QString filename = littleHelper::adjustPathToOS(Project::getInstance()->getTrials()[t]->getVideoStreams()[c]->getFilenames()[0]);
			QFileInfo fileinfo(filename);
			if (!QFile::exists(filename))
			{
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


#ifndef BETA
	this->setWindowTitle(Project::getInstance()->getProjectBasename() + " - XMALab " + PROJECT_VERSION);
#else 
	this->setWindowTitle(Project::getInstance()->getProjectBasename() + " - XMALab " + PROJECT_VERSION + " - BETA");
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
		if (worldViewDockWidget)worldViewDockWidget->draw();
	}
}

void MainWindow::setCameraViewWidgetTitles()
{
	if (State::getInstance()->getWorkspace() == CALIBRATION)
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
	if (project->isCalibrated())
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
		ui->actionExport3D_Points->setEnabled(false);
		ui->actionRigidBodyTransformations->setEnabled(false);
		ui->actionMarkertoMarkerDistances->setEnabled(false);
		ui->actionImport2D_Points->setEnabled(false);
		ui->actionExport_Undistorted_Trial_images_for_Maya->setEnabled(false);
		ui->actionDetailed_View->setEnabled(false);
		ui->actionPlot->setEnabled(false);
		ui->action3D_world_view->setEnabled(false);

		PointsDockWidget::getInstance()->hide();
		DetailViewDockWidget::getInstance()->hide();
		PlotWindow::getInstance()->hide();
		worldViewDockWidget->hide();
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
		ui->actionExport3D_Points->setEnabled(false);
		ui->actionRigidBodyTransformations->setEnabled(false);
		ui->actionMarkertoMarkerDistances->setEnabled(false);
		ui->actionImport2D_Points->setEnabled(false);
		ui->actionExport_Undistorted_Trial_images_for_Maya->setEnabled(false);
		ui->actionPlot->setChecked(false);
		ui->actionPlot->setEnabled(false);

		ui->actionDetailed_View->setEnabled(false);
		ui->actionPlot->setEnabled(false);
		ui->action3D_world_view->setEnabled(true);

		PointsDockWidget::getInstance()->hide();
		DetailViewDockWidget::getInstance()->hide();
		PlotWindow::getInstance()->hide();
		if (Settings::getInstance()->getBoolSetting("Show3DView"))
		{
			worldViewDockWidget->show();
		}
		else
		{
			worldViewDockWidget->hide();
		}
	}
	else if (workspace == DIGITIZATION)
	{
		if (project->getTrials().size() > 0)
		{
			ui->imageMainFrame->setVisible(true);
			ui->startTrialFrame->setVisible(false);
			SequenceNavigationFrame::getInstance()->setVisible(true);
			ui->actionPlot->setEnabled(true);
			if (project->isCalibrated())
			{
				ui->labelCalibrateFirst->setVisible(false);
				ui->pushButtonNewTrial->setVisible(true);

				ui->actionDetailed_View->setEnabled(true);
				ui->actionPlot->setEnabled(true);
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
					worldViewDockWidget->show();
				}
				else
				{
					worldViewDockWidget->hide();
				}

				if (Settings::getInstance()->getBoolSetting("ShowDetailView"))
				{
					DetailViewDockWidget::getInstance()->show();
				}
				else
				{
					DetailViewDockWidget::getInstance()->hide();
				}
			}
			else
			{
				ui->labelCalibrateFirst->setVisible(true);
				ui->pushButtonNewTrial->setVisible(false);

				ui->actionDetailed_View->setEnabled(false);
				ui->actionPlot->setEnabled(false);
				ui->action3D_world_view->setEnabled(false);

				PointsDockWidget::getInstance()->hide();
				DetailViewDockWidget::getInstance()->hide();
				PlotWindow::getInstance()->hide();
				worldViewDockWidget->hide();
			}

			ui->actionExport2D_Points->setEnabled(true);
			ui->actionExport3D_Points->setEnabled(true);
			ui->actionRigidBodyTransformations->setEnabled(true);
			ui->actionMarkertoMarkerDistances->setEnabled(true);
			ui->actionImport2D_Points->setEnabled(true);
			ui->actionExport_Undistorted_Trial_images_for_Maya->setEnabled(true);
		}
		else
		{
			ui->imageMainFrame->setVisible(false);
			ui->startTrialFrame->setVisible(true);
			SequenceNavigationFrame::getInstance()->setVisible(false);
			ui->actionPlot->setEnabled(false);
			if (project->isCalibrated())
			{
				ui->labelCalibrateFirst->setVisible(false);
				ui->pushButtonNewTrial->setVisible(true);
			}
			else
			{
				ui->labelCalibrateFirst->setVisible(true);
				ui->pushButtonNewTrial->setVisible(false);
			}
			ui->actionExport2D_Points->setEnabled(false);
			ui->actionExport3D_Points->setEnabled(false);
			ui->actionRigidBodyTransformations->setEnabled(false);
			ui->actionMarkertoMarkerDistances->setEnabled(false);
			ui->actionImport2D_Points->setEnabled(false);
			ui->actionExport_Undistorted_Trial_images_for_Maya->setEnabled(false);

			ui->actionDetailed_View->setEnabled(false);
			ui->actionPlot->setEnabled(false);
			ui->actionShow_3D_View->setEnabled(false);

			PointsDockWidget::getInstance()->hide();
			DetailViewDockWidget::getInstance()->hide();
			PlotWindow::getInstance()->hide();
			worldViewDockWidget->hide();
		}
	}


	setCameraViewWidgetTitles();
	redrawGL();
}

void MainWindow::displayChanged(ui_state display)
{
	relayoutCameras();
	redrawGL();
}

void MainWindow::activeCameraChanged(int activeCamera)
{
	if (State::getInstance()->getDisplay() == SINGLE_CAMERA)
	{
		relayoutCameras();
	}
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
		setCameraViewWidgetTitles();
		redrawGL();
	}
}

//File Menu Slots
void MainWindow::on_actionNew_Project_triggered(bool checked)
{
	if (project && !ConfirmationDialog::getInstance()->showConfirmationDialog("You are about to close your dataset. Please make sure your data have been saved. \nAre you sure you want to close current dataset and create a new dataset?"))
		return;
	newProject();
}

void MainWindow::on_actionLoad_Project_triggered(bool checked)
{
	if (project && !ConfirmationDialog::getInstance()->showConfirmationDialog("You are about to close your dataset. Please make sure your data have been saved. \nAre you sure you want to close current dataset and load an existing dataset?"))
		return;
	loadProject();
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

void MainWindow::on_actionExport3D_Points_triggered(bool checked)
{
	PointImportExportDialog* diag = new PointImportExportDialog(EXPORT3D, this);

	diag->exec();
	if (diag->result())
	{
		bool saved;
		bool filterData = ConfirmationDialog::getInstance()->showConfirmationDialog("Do you want to filter 3D Points?", true);
		double filter_frequency = -1;
		if (filterData)
		{
			filter_frequency = QInputDialog::getDouble(this, "Enter cutoff frequency (Hz)", "", project->getTrials()[State::getInstance()->getActiveTrial()]->getCutoffFrequency(), 0.0, project->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed()*0.5);
		}


		if (Settings::getInstance()->getBoolSetting("Export3DSingle"))
		{
			QString outputPath = QFileDialog::getExistingDirectory(this,
			                                                       tr("Save to Directory "), Settings::getInstance()->getLastUsedDirectory());

			if (outputPath.isNull() == false)
			{
				saved = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->save3dPoints(outputPath + OS_SEP
				                                                                                          , Settings::getInstance()->getBoolSetting("Export3DMulti")
				                                                                                          , Settings::getInstance()->getBoolSetting("Export3DHeader")
																										  ,filter_frequency);
				Settings::getInstance()->setLastUsedDirectory(outputPath, true);
			}
		}
		else if (Settings::getInstance()->getBoolSetting("Export3DMulti"))
		{
			QString fileName = QFileDialog::getSaveFileName(this,
			                                                tr("Save 3D points as"), Settings::getInstance()->getLastUsedDirectory(), tr("Comma seperated data (*.csv)"));

			if (fileName.isNull() == false)
			{
				saved = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->save3dPoints(fileName
				                                                                                          , Settings::getInstance()->getBoolSetting("Export3DMulti")
				                                                                                          , Settings::getInstance()->getBoolSetting("Export3DHeader"),
																										  filter_frequency);
				Settings::getInstance()->setLastUsedDirectory(fileName);
			}
		}
		if (!saved) ErrorDialog::getInstance()->showErrorDialog("There has been a problem saving. Please ensure that the cutoff and framerate are valid in case you exported filtered data.");
	}

	delete diag;
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
				                                                                                          , Settings::getInstance()->getBoolSetting("Export2DMulti")
				                                                                                          , Settings::getInstance()->getBoolSetting("Export2DDistorted")
				                                                                                          , Settings::getInstance()->getBoolSetting("Export2DCount1")
				                                                                                          , Settings::getInstance()->getBoolSetting("Export2DYUp")
				                                                                                          , Settings::getInstance()->getBoolSetting("Export2DHeader")
				                                                                                          , Settings::getInstance()->getBoolSetting("Export2DOffsetCols"));
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
				                                                                                          , Settings::getInstance()->getBoolSetting("Export2DMulti")
				                                                                                          , Settings::getInstance()->getBoolSetting("Export2DDistorted")
				                                                                                          , Settings::getInstance()->getBoolSetting("Export2DCount1")
				                                                                                          , Settings::getInstance()->getBoolSetting("Export2DYUp")
				                                                                                          , Settings::getInstance()->getBoolSetting("Export2DHeader")
				                                                                                          , Settings::getInstance()->getBoolSetting("Export2DOffsetCols"));
				Settings::getInstance()->setLastUsedDirectory(fileName);
			}
		}
	}

	delete diag;
}

void MainWindow::on_actionRigidBodyTransformations_triggered(bool checked)
{
	PointImportExportDialog* diag = new PointImportExportDialog(EXPORTTRANS, this);

	diag->exec();
	if (diag->result())
	{
		if (Settings::getInstance()->getBoolSetting("ExportTransSingle"))
		{
			QString outputPath = QFileDialog::getExistingDirectory(this, tr("Save to Directory "), Settings::getInstance()->getLastUsedDirectory());

			if (outputPath.isNull() == false)
			{
				Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->saveRigidBodyTransformations(outputPath + OS_SEP
				                                                                                                          , Settings::getInstance()->getBoolSetting("ExportTransMulti")
				                                                                                                          , Settings::getInstance()->getBoolSetting("ExportTransHeader")
				                                                                                                          , Settings::getInstance()->getBoolSetting("ExportTransFiltered"));
				Settings::getInstance()->setLastUsedDirectory(outputPath, true);
			}
		}
		else if (Settings::getInstance()->getBoolSetting("ExportTransMulti"))
		{
			QString fileName = QFileDialog::getSaveFileName(this,
			                                                tr("Save 2D points as"), Settings::getInstance()->getLastUsedDirectory(), tr("Comma seperated data (*.csv)"));


			if (fileName.isNull() == false)
			{
				Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->saveRigidBodyTransformations(fileName
				                                                                                                          , Settings::getInstance()->getBoolSetting("ExportTransMulti")
				                                                                                                          , Settings::getInstance()->getBoolSetting("ExportTransHeader")
				                                                                                                          , Settings::getInstance()->getBoolSetting("ExportTransFiltered"));
				Settings::getInstance()->setLastUsedDirectory(fileName);
			}
		}
	}
	delete diag;
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
			Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->saveMarkerToMarkerDistances(fileName,fromTo->getFrom(), fromTo->getTo());
		}
	}
	delete fromTo;
}

void MainWindow::on_actionExport_Undistorted_Trial_images_for_Maya_triggered(bool checked)
{
	FromToDialog* fromTo = new FromToDialog(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame()
	                                        , Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame()
	                                        , Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getNbImages()
	                                        , true, this);

	bool ok = fromTo->exec();
	if (ok)
	{
		QString outputPath = QFileDialog::getExistingDirectory(this,
		                                                       tr("Save to Directory "), Settings::getInstance()->getLastUsedDirectory());

		if (outputPath.isNull() == false)
		{
			Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->saveTrialImages(outputPath + OS_SEP, fromTo->getFrom(), fromTo->getTo(), fromTo->getFormat());
			Settings::getInstance()->setLastUsedDirectory(outputPath, true);
		}
	}

	delete fromTo;
}

void MainWindow::on_actionMayaCams_triggered(bool checked)
{
	if (Project::getInstance()->isCalibrated())
	{
		QString outputPath = QFileDialog::getExistingDirectory(this,
		                                                       tr("Save to Directory "), Settings::getInstance()->getLastUsedDirectory());

		if (outputPath.isNull() == false)
		{
			Project::getInstance()->exportMayaCam(outputPath);
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
		QString outputPath = QFileDialog::getExistingDirectory(this,
		                                                       tr("Save to Directory "), Settings::getInstance()->getLastUsedDirectory());

		if (outputPath.isNull() == false)
		{
			Project::getInstance()->exportMayaCamVersion2(outputPath);
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
				                                                                                                        , Settings::getInstance()->getBoolSetting("Import2DDistorted")
				                                                                                                        , Settings::getInstance()->getBoolSetting("Import2DCount1")
				                                                                                                        , Settings::getInstance()->getBoolSetting("Import2DYUp")
				                                                                                                        , Settings::getInstance()->getBoolSetting("Import2DHeader")
				                                                                                                        , Settings::getInstance()->getBoolSetting("Import2DOffsetCols"));
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

		bool ok;
		QString item = QInputDialog::getItem(this, tr("Choose trial to import"),
		                                     tr("Trial:"), trialnames, 0, false, &ok);

		if (ok && !item.isEmpty())
		{
			Trial* trial = ProjectFileIO::getInstance()->loadTrials(fileName, item);
			Project::getInstance()->addTrial(trial);
			WorkspaceNavigationFrame::getInstance()->addTrial(item);
			checkTrialImagePaths();
			trial->bindTextures();

			if (State::getInstance()->getWorkspace() == DIGITIZATION)
			{
				State::getInstance()->changeWorkspace(DIGITIZATION, true);
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

//startMainFrameButtons
void MainWindow::on_pushButtonNew_Project_clicked()
{
	newProject();
}

void MainWindow::on_pushButtonLoad_Project_clicked()
{
	loadProject();
}

void MainWindow::on_pushButtonNewTrial_clicked()
{
	newTrial();
}

void MainWindow::on_action3D_world_view_triggered(bool checked)
{
	if (checked)
	{
		worldViewDockWidget->show();
		Settings::getInstance()->set("Show3DView", true);
	}
	else
	{
		worldViewDockWidget->hide();
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
}
