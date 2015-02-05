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

#include "core/Project.h"
#include "core/Camera.h"
#include "core/Trial.h"
#include "core/CalibrationImage.h"
#include "core/UndistortionObject.h"
#include "core/Settings.h"

#include "processing/BlobDetection.h"
#include "processing/LocalUndistortion.h"

#include <QSplitter>
#include <QFileDialog>
#include <QtCore>

#include <iostream>

#ifdef WIN32
#define OS_SEP "\\"
#else
#define OS_SEP "/"
#endif

using namespace xma;

MainWindow* MainWindow::instance = NULL;

MainWindow::MainWindow(QWidget *parent) :
												QMainWindow(parent),
												ui(new Ui::MainWindow){

	//To prevent endless looping when accesing MainWindow::getInstance in constructors
	worldViewDockWidget = NULL;
	if(!instance) instance = this;

	ui->setupUi(this);
	this->statusBar()->hide();

	ui->imageMainFrame->setVisible(false);
	WorkspaceNavigationFrame::getInstance()->setObjectName(QString::fromUtf8("workspaceNavigationFrame"));
    WorkspaceNavigationFrame::getInstance()->setFrameShape(QFrame::StyledPanel);
    WorkspaceNavigationFrame::getInstance()->setFrameShadow(QFrame::Raised);
	ui->gridLayout_2->removeWidget(ui->imageScrollArea);
	ui->gridLayout_2->addWidget(WorkspaceNavigationFrame::getInstance(), 0, 0, 1, 1);
	ui->gridLayout_2->addWidget(ui->imageScrollArea, 1, 0, 1, 1);

	WorkspaceNavigationFrame::getInstance()->setVisible(false);

	ui->sequenceNavigationFrame->setVisible(false);
	ui->imageScrollArea->setVisible(false);
	ui->startTrialFrame->setVisible(false);
	ui->actionDetailed_View->setChecked(Settings::getShowDetailView());
	ui->actionDetailed_View->setEnabled(false);

	project = NULL;

	resizeTimer.setSingleShot( true );
	connect( &resizeTimer, SIGNAL(timeout()), SLOT(resizeDone()) );

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
	restoreGeometry(Settings::getUIGeometry("XMALab"));
	if(Settings::getUIState("XMALab").size()<0 ||
		!restoreState(Settings::getUIState("XMALab"),UI_VERSION)){
		WizardDockWidget::getInstance()->setFloating(true);
		ProgressDialog::getInstance()->setFloating(true);
		worldViewDockWidget->setFloating(true);	
		ConsoleDockWidget::getInstance()->setFloating(true);	
		DetailViewDockWidget::getInstance()->setFloating(true);
		PlotWindow::getInstance()->setFloating(true);
	}
	
	WizardDockWidget::getInstance()->hide();
	ConsoleDockWidget::getInstance()->hide();
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
}


MainWindow::~MainWindow(){
	closeProject();
	delete GLSharedWidget::getInstance();
	delete WizardDockWidget::getInstance();
	delete worldViewDockWidget;
	instance = NULL;
}


MainWindow* MainWindow::getInstance()
{
	if(!instance) 
	{
		instance = new MainWindow();
	}
	return instance;
}

void MainWindow::resizeDone(){
	if(State::getInstance()->getDisplay() == ALL_CAMERAS_FULL_HEIGHT){
		for (int i = 0 ; i < cameraViews.size();i++){
			cameraViews[i]->setMinimumWidthGL(true);
		}
	}
}

void MainWindow::resizeEvent(QResizeEvent *event){
	resizeTimer.start( 500 );
	QMainWindow::resizeEvent(event);
} 

void MainWindow::closeEvent (QCloseEvent * event){
	Settings::setUIGeometry("XMALab",saveGeometry());
    Settings::setUIState("XMALab", saveState(UI_VERSION));
    QMainWindow::closeEvent(event);
	QApplication::quit();
}	

void MainWindow::recountFrames(){
	project->recountFrames();
	ui->sequenceNavigationFrame->setNbImages(project->getNbImagesCalibration());
}

void MainWindow::setupProjectUI(){
	State::getInstance()->changeActiveCamera(0);
	State::getInstance()->changeActiveFrameCalibration(0);

	bool hasUndistorion = false;
	int count = 0;
	for(std::vector <Camera*>::const_iterator it = project->getCameras().begin(); it != project->getCameras().end(); ++it){
		CameraViewWidget* cam_widget = new CameraViewWidget((*it), this);
		cam_widget->setSharedGLContext(GLSharedWidget::getInstance()->getQGLContext());	
		cameraViews.push_back(cam_widget);	
		hasUndistorion = hasUndistorion || (*it)->hasUndistortion();
		WorkspaceNavigationFrame::getInstance()->addCamera(count,(*it)->getName());
		count++;
		QApplication::processEvents();
    }

	ui->startMainFrame->setVisible(false);
	ui->imageMainFrame->setVisible(true);

	ui->sequenceNavigationFrame->setNbImages(project->getNbImagesCalibration());

	WorkspaceNavigationFrame::getInstance()->setVisible(true);
	WorkspaceNavigationFrame::getInstance()->setUndistortion(hasUndistorion);
	
	if (hasUndistorion) State::getInstance()->changeWorkspace(UNDISTORTION);
	else  State::getInstance()->changeWorkspace(CALIBRATION);
	
	relayoutCameras();
	DetailViewDockWidget::getInstance()->setup();
	redrawGL();
}

void MainWindow::tearDownProjectUI(){
	clearSplitters();
	for (int i = 0 ; i < cameraViews.size();i++){
		delete cameraViews[i];
		WorkspaceNavigationFrame::getInstance()->removeCamera(0);
	}
	cameraViews.clear();

	DetailViewDockWidget::getInstance()->clear();

	ui->startMainFrame->setVisible(true);
	ui->imageMainFrame->setVisible(false);
	ui->startTrialFrame->setVisible(false);
	WorkspaceNavigationFrame::getInstance()->setVisible(false);
	ui->sequenceNavigationFrame->setVisible(false);
	WorkspaceNavigationFrame::getInstance()->closeProject();
}

void MainWindow::clearSplitters(){
	for (int i = 0 ; i < cameraViews.size();i++){
		cameraViews[i]->setParent(NULL);
	}

	QObjectList objs = ui->imageMainFrame->children();
	for(int x = 0; x < objs.size(); x ++){
		QObjectList objs2 = objs[x]->children();
		QSplitter * vertsplit = dynamic_cast<QSplitter*> (objs[x]);
		for(int y = objs2.size() - 1 ; y >= 0; y --){	
			QSplitter * horsplit = dynamic_cast<QSplitter*> (objs2[y]);
			if(horsplit){
				QObjectList objs3 = horsplit->children();
				delete horsplit;
			}
		}	
		
		if(vertsplit){
			ui->gridLayout_4->removeWidget(vertsplit);
			delete vertsplit;
		}
	}
}

void MainWindow::relayoutCameras(){
	
	clearSplitters();
	
	if(State::getInstance()->getDisplay() == SINGLE_CAMERA){
		ui->imageScrollArea->setVisible(false);
		ui->imageMainFrame->setVisible(true);

		for (int i = 0 ; i < cameraViews.size();i++){
			cameraViews[i]->setMinimumWidthGL(false);
		}
		ui->gridLayout_4->addWidget(cameraViews[State::getInstance()->getActiveCamera()]);
	}
	else if(State::getInstance()->getDisplay() == ALL_CAMERAS_FULL_HEIGHT){
		ui->imageScrollArea->setVisible(true);
		ui->imageMainFrame->setVisible(false);
		for (int i = 0 ; i < cameraViews.size();i++){
			ui->horizontalLayout->addWidget(cameraViews[i]);
		}
		resizeTimer.start( 500 );
	}else{
		ui->imageScrollArea->setVisible(false);
		ui->imageMainFrame->setVisible(true);
		QApplication::processEvents();

		int rows = 1;

		if(State::getInstance()->getDisplay() == ALL_CAMERAS_2ROW_SCALED){
			rows = 2;
		}else if(State::getInstance()->getDisplay() == ALL_CAMERAS_3ROW_SCALED){
			rows = 3;
		}

		//Create New
		QSize cameraViewArrangement = QSize(ceil( ((double) cameraViews.size())/rows),rows);

		QSplitter * splitter = new QSplitter(this);
		splitter->setOrientation(Qt::Vertical);
		for(int i = 0; i < cameraViewArrangement.height() ; i ++){
			QSplitter * splitterHorizontal = new QSplitter(splitter);
			splitterHorizontal->setOrientation(Qt::Horizontal);
			splitter->addWidget(splitterHorizontal);
		}

		int freeSpaces = cameraViewArrangement.height() * cameraViewArrangement.width() - cameraViews.size();
	
		int count = 0;
		for (int i = 0 ; i < cameraViews.size();i++, count++){
			cameraViews[i]->setMinimumWidthGL(false);
			if(cameraViews.size() < i + freeSpaces) count++;
			QObject* obj = splitter->children().at(rows - 1 - count / (cameraViewArrangement.width()));
			QSplitter * horsplit = dynamic_cast<QSplitter*> (obj);
			if(horsplit)horsplit->addWidget(cameraViews[i]);
		}

		ui->gridLayout_4->addWidget(splitter, 0, 0, 1, 1);
		QApplication::processEvents();

		for(int i = 0; i < cameraViewArrangement.height() ; i ++){
			QObject* obj = splitter->children().at(i);
			QSplitter * horsplit = dynamic_cast<QSplitter*> (obj);
			if(horsplit){
				QList<int> sizelist = horsplit->sizes();
				for(int m = 0; m < sizelist.size(); m++){
						sizelist[m] = 100;
				}
				horsplit->setSizes(sizelist);
			}
		}		
	}
}

//Project functions
void MainWindow::newProject(){
	if(project) 
		closeProject();
	project = Project::getInstance();
	
	newProjectdialog = new NewProjectDialog();
	newProjectdialog->exec();

	if(newProjectdialog->result()){
		m_FutureWatcher = new QFutureWatcher<bool>();
		connect( m_FutureWatcher, SIGNAL( finished() ), this, SLOT( newProjectFinished() ) );

		QFuture<bool> future = QtConcurrent::run( newProjectdialog, &NewProjectDialog::createProject);
		m_FutureWatcher->setFuture( future );

		ProgressDialog::getInstance()->showProgressbar(0, 0, "Create new project");
	}else{
		delete newProjectdialog;
		delete project;
		project = NULL;
	}
}

void MainWindow::newProjectFinished(){
	if(m_FutureWatcher->result()){
		project->loadTextures();
		setupProjectUI();
	}else{
		delete project;
		project = NULL;
	}
	delete newProjectdialog;
	delete m_FutureWatcher;
	ProgressDialog::getInstance()->closeProgressbar();
	WizardDockWidget::getInstance()->updateDialog();
	WizardDockWidget::getInstance()->show();
}


void MainWindow::loadProject(){
	if(project) 
		closeProject();

	project = Project::getInstance();

	QString fileName = QFileDialog::getOpenFileName(this,
									tr("Select Project File"), Settings::getLastUsedDirectory(),tr("Project zip File (*.zip)"));
	if ( fileName.isNull() == false )
    {
		Settings::setLastUsedDirectory(fileName);

		m_FutureWatcher = new QFutureWatcher<bool>();
		connect( m_FutureWatcher, SIGNAL( finished() ), this, SLOT( loadProjectFinished() ) );

		QFuture<bool> future = QtConcurrent::run( ProjectFileIO::getInstance(), &ProjectFileIO::loadProject,fileName  );
		m_FutureWatcher->setFuture( future );
		
		ProgressDialog::getInstance()->showProgressbar(0, 0, ("Load project "  + fileName).toAscii().data());
	}
}

void MainWindow::loadProjectFinished(){
	checkTrialImagePaths();

	if(m_FutureWatcher->result()){
		project->loadTextures();
		for (std::vector<Trial*>::const_iterator trial = Project::getInstance()->getTrials().begin(); trial != Project::getInstance()->getTrials().end(); ++trial)
		{
			(*trial)->bindTextures();
		}

		setupProjectUI();
	}else{
		delete project;
		project = NULL;
	}
	delete m_FutureWatcher;
	ProgressDialog::getInstance()->closeProgressbar();
	WizardDockWidget::getInstance()->updateDialog();
	WizardDockWidget::getInstance()->show();

	for(int i = 0; i < Project::getInstance()->getCameras().size(); i++){
		if(Project::getInstance()->getCameras()[i]->hasUndistortion() && Project::getInstance()->getCameras()[i]->getUndistortionObject()->isComputed()){
			LocalUndistortion * localUndistortion = new LocalUndistortion(i);
			connect(localUndistortion, SIGNAL(localUndistortion_finished()), this, SLOT(UndistortionAfterloadProjectFinished()));
			localUndistortion->computeUndistortion(true);
		}
	}
	if(!LocalUndistortion::isRunning()){
		MainWindow::getInstance()->redrawGL();
		UndistortionAfterloadProjectFinished();
	}

	this->setWindowTitle(Project::getInstance()->getProjectBasename() + " - XMALab");
}

void MainWindow::UndistortionAfterloadProjectFinished(){
	bool allCamerasUndistorted = true;

	for(std::vector <Camera*>::const_iterator it = Project::getInstance()->getCameras().begin(); it != Project::getInstance()->getCameras().end(); ++it){
		if((*it)->hasUndistortion()){
			if(!(*it)->getUndistortionObject()->isComputed()){
				allCamerasUndistorted = false;
			}
		}
	}
	if(allCamerasUndistorted) State::getInstance()->changeUndistortion(UNDISTORTED);

	for(std::vector <Camera*>::const_iterator it = Project::getInstance()->getCameras().begin(); it != Project::getInstance()->getCameras().end(); ++it){
		bool calibrated = false;
		for(std::vector <CalibrationImage*>::const_iterator it2 = (*it)->getCalibrationImages().begin(); it2 != (*it)->getCalibrationImages().end(); ++it2){
			if((*it2)->isCalibrated()) calibrated = true;
		}
		if(calibrated){
			(*it)->setCalibrated(true);
			(*it)->setRecalibrationRequired(1);
		}
	}

	ConsoleDockWidget::getInstance()->afterLoad();

	WizardDockWidget::getInstance()->update();

	MainWindow::getInstance()->redrawGL();
}

void MainWindow::closeProject(){
	
	if(project){
		//prompt for save
		bool saveProject = false;
		
		if(saveProject){
			saveProjectAs();
		}
	
		//unlink UI and remove project
		tearDownProjectUI();
		delete project;
		project = NULL;
	}
	State::getInstance()->changeUndistortion(NOTUNDISTORTED);
	WizardDockWidget::getInstance()->hide();
	PointsDockWidget::getInstance()->hide();
    DetailViewDockWidget::getInstance()->hide();
	this->setWindowTitle("XMALab");
	ConsoleDockWidget::getInstance()->clear();
	ui->actionPlot->setChecked(false);
	PlotWindow::getInstance()->hide();
}


void MainWindow::saveProject(){
	if(WizardDockWidget::getInstance()->checkForPendingChanges()){
		m_FutureWatcher = new QFutureWatcher<bool>();
		connect( m_FutureWatcher, SIGNAL( finished() ), this, SLOT( saveProjectFinished() ) );

		QFuture<bool> future = QtConcurrent::run( ProjectFileIO::getInstance(), &ProjectFileIO::saveProject,project->getProjectFilename()  );
		m_FutureWatcher->setFuture( future );
		
		ProgressDialog::getInstance()->showProgressbar(0, 0, ("Save project as " + project->getProjectFilename()).toAscii().data());
	}
}

void MainWindow::saveProjectAs(){
	if(WizardDockWidget::getInstance()->checkForPendingChanges()){
		QString fileName = QFileDialog::getSaveFileName(this,
			tr("Save Project File as"), project->getProjectFilename().isEmpty() ? Settings::getLastUsedDirectory() : project->getProjectFilename(),tr("Project zip File (*.zip)"));

		ConsoleDockWidget::getInstance()->prepareSave();

		if ( fileName.isNull() == false )
		{
			Settings::setLastUsedDirectory(fileName);
    
			m_FutureWatcher = new QFutureWatcher<bool>();
			connect( m_FutureWatcher, SIGNAL( finished() ), this, SLOT( saveProjectFinished() ) );

			QFuture<bool> future = QtConcurrent::run( ProjectFileIO::getInstance(), &ProjectFileIO::saveProject,fileName  );
			m_FutureWatcher->setFuture( future );
		
			ProgressDialog::getInstance()->showProgressbar(0, 0, ("Save project as " + fileName).toAscii().data());
		}
	}
}

void MainWindow::newTrial()
{
	NewTrialDialog * newTriaLdialog = new NewTrialDialog();
	newTriaLdialog->exec();

	if (newTriaLdialog->result()){
		newTriaLdialog->createTrial();
		State::getInstance()->changeActiveTrial(project->getTrials().size() - 1);
		State::getInstance()->changeActiveFrameTrial(project->getTrials()[State::getInstance()->getActiveTrial()]->getActiveFrame());
	}
}

void MainWindow::checkTrialImagePaths()
{
	for (int t = 0; t < Project::getInstance()->getTrials().size(); t++)
	{
		for (int c = 0; c < Project::getInstance()->getCameras().size(); c++)
		{
			QString filename = Project::getInstance()->getTrials()[t]->getFilenames()[c].at(0);
			QFileInfo fileinfo(filename);
			if (!QFile::exists(filename))
			{
				QString newfolder = QFileDialog::getExistingDirectory(this, fileinfo.fileName() + "not found. Enter a new directory for " + fileinfo.fileName(), Settings::getLastUsedDirectory());

				if (!newfolder.isEmpty())
				{
					QString oldfolder = filename.replace(fileinfo.fileName(), "");
					Project::getInstance()->getTrials()[t]->changeImagePath(c, newfolder + OS_SEP, oldfolder);
				}
			}
		}
	}
}

void MainWindow::saveProjectFinished(){
	delete m_FutureWatcher;
	ProgressDialog::getInstance()->closeProgressbar();
	this->setWindowTitle(Project::getInstance()->getProjectBasename() + " - XMALab");
}

void MainWindow::redrawGL(){
	for (unsigned int i = 0; i < cameraViews.size(); i++) {
		cameraViews[i]->draw();
		cameraViews[i]->updateInfo();
    }
	DetailViewDockWidget::getInstance()->draw();
	if(worldViewDockWidget)worldViewDockWidget->draw();
}

void MainWindow::setCameraViewWidgetTitles(){
	if(State::getInstance()->getWorkspace() == CALIBRATION){
		for (unsigned int i = 0; i < cameraViews.size(); i++) {
			cameraViews[i]->setImageName(Project::getInstance()->getCameras()[i]->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->getFilename());
		}
	}
	else if (State::getInstance()->getWorkspace() == UNDISTORTION){
		for (unsigned int i = 0; i < cameraViews.size(); i++) {
			if (Project::getInstance()->getCameras()[i]->hasUndistortion()){
				cameraViews[i]->setImageName(Project::getInstance()->getCameras()[i]->getUndistortionObject()->getFilename());
			}
			else{
				cameraViews[i]->setImageName("No undistortion Grid loaded");
			}
		}
	}
	else if (State::getInstance()->getWorkspace() == DIGITIZATION){
		if (project->getTrials().size() > 0)
		{
			for (unsigned int i = 0; i < cameraViews.size(); i++) {
				cameraViews[i]->setImageName(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveFilename(i));
			}
		}
	}
}

/***************************************************************
UI - SLOTS
***************************************************************/

//custom slots for state
void MainWindow::workspaceChanged(work_state workspace){
	if (workspace == UNDISTORTION)
	{
		PointsDockWidget::getInstance()->hide();
		ui->actionDetailed_View->setEnabled(false);
		DetailViewDockWidget::getInstance()->hide();
		ui->sequenceNavigationFrame->setVisible(false);
		ui->imageMainFrame->setVisible(true);
		ui->startTrialFrame->setVisible(false);
		ui->actionExport2D_Points->setEnabled(false);
		ui->actionExport3D_Points->setEnabled(false);
		ui->actionPlot->setChecked(false);
		ui->actionPlot->setEnabled(false);
		PlotWindow::getInstance()->hide();
		
	}
	else if (workspace == CALIBRATION)
	{
		
		PointsDockWidget::getInstance()->hide();
		ui->actionDetailed_View->setEnabled(false);
		DetailViewDockWidget::getInstance()->hide();
		if (project->getNbImagesCalibration() > 1) {
			ui->sequenceNavigationFrame->setVisible(true);
		}
		else 
		{
			ui->sequenceNavigationFrame->setVisible(false);
		}
		ui->imageMainFrame->setVisible(true);
		ui->startTrialFrame->setVisible(false);
		ui->actionExport2D_Points->setEnabled(false);
		ui->actionExport3D_Points->setEnabled(false);
		ui->actionPlot->setChecked(false);
		ui->actionPlot->setEnabled(false);
		PlotWindow::getInstance()->hide();
	}
	else if (workspace == DIGITIZATION)
	{
		if (project->getTrials().size() > 0)
		{
			ui->imageMainFrame->setVisible(true);
			ui->startTrialFrame->setVisible(false);
			ui->sequenceNavigationFrame->setVisible(true);
			ui->actionPlot->setEnabled(true);
			if (project->isCalibrated())
			{
				ui->labelCalibrateFirst->setVisible(false);
				ui->pushButtonNewTrial->setVisible(true);
				PointsDockWidget::getInstance()->show();
				ui->actionDetailed_View->setEnabled(true);
				if (Settings::getShowDetailView()){
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
				PointsDockWidget::getInstance()->hide();
				ui->actionDetailed_View->setEnabled(false);
				DetailViewDockWidget::getInstance()->hide();
			}

			ui->actionExport2D_Points->setEnabled(true);
			ui->actionExport3D_Points->setEnabled(true);
		}
		else
		{
			ui->imageMainFrame->setVisible(false);
			ui->startTrialFrame->setVisible(true);
			ui->sequenceNavigationFrame->setVisible(false);
			PointsDockWidget::getInstance()->hide();
			ui->actionDetailed_View->setEnabled(false);
			DetailViewDockWidget::getInstance()->hide();
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
		}
	}


	setCameraViewWidgetTitles();
	redrawGL();
}

void MainWindow::displayChanged(ui_state display){
	relayoutCameras();
	redrawGL();
}

void MainWindow::activeCameraChanged(int activeCamera){
	if(State::getInstance()->getDisplay() == SINGLE_CAMERA){
		relayoutCameras();
	}
	redrawGL();
}

void MainWindow::activeFrameCalibrationChanged(int activeFrame){
	setCameraViewWidgetTitles();
	redrawGL();
}

void MainWindow::activeFrameTrialChanged(int)
{
	setCameraViewWidgetTitles();
	redrawGL();
}

void MainWindow::activeTrialChanged(int activeCamera)
{
	setCameraViewWidgetTitles();
	redrawGL();
}

//File Menu Slots
void MainWindow::on_actionNew_Project_triggered(bool checked){
	newProject();
}
void MainWindow::on_actionLoad_Project_triggered(bool checked){
	loadProject();
}
void MainWindow::on_actionClose_Project_triggered(bool checked){
	closeProject();
}
void MainWindow::on_actionSave_Project_triggered(bool checked){
	if(project->getProjectFilename().isEmpty()){
		saveProjectAs();
	}else{
		saveProject();
	}
}

void MainWindow::on_actionSave_Project_as_triggered(bool checked){
	saveProjectAs();
}

//File->Export Menu Slots
void MainWindow::on_actionExportDLT_triggered(bool checked){
	QString outputPath = QFileDialog::getExistingDirectory(this,
		tr("Save to Directory "), Settings::getLastUsedDirectory());
	if ( outputPath.isNull() == false )
    {
        Project::getInstance()->exportDLT(outputPath);
		Settings::setLastUsedDirectory(outputPath,true);
    }
}

void MainWindow::on_actionExportLUT_triggered(bool checked){
	QString outputPath = QFileDialog::getExistingDirectory(this,
		tr("Save to Directory "), Settings::getLastUsedDirectory());
	if ( outputPath.isNull() == false )
    {
        Project::getInstance()->exportLUT(outputPath);
		Settings::setLastUsedDirectory(outputPath,true);
    }
}

void MainWindow::on_actionExportMayacams_triggered(bool checked){
	QString outputPath = QFileDialog::getExistingDirectory(this,
		tr("Save to Directory "), Settings::getLastUsedDirectory());
	if ( outputPath.isNull() == false )
    {
        Project::getInstance()->exportMayaCam(outputPath);
		Settings::setLastUsedDirectory(outputPath,true);
    }
}

void MainWindow::on_actionExportAll_triggered(bool checked){
	QString outputPath = QFileDialog::getExistingDirectory(this,
		tr("Save to Directory "), Settings::getLastUsedDirectory());

	if ( outputPath.isNull() == false )
    {
        Project::getInstance()->exportDLT(outputPath);
		Project::getInstance()->exportMayaCam(outputPath);
		Project::getInstance()->exportLUT(outputPath);
		Settings::setLastUsedDirectory(outputPath,true);
    }
}

void MainWindow::on_actionExport3D_Points_triggered(bool checked)
{
	QString outputPath = QFileDialog::getExistingDirectory(this,
		tr("Save to Directory "), Settings::getLastUsedDirectory());

	if (outputPath.isNull() == false)
	{
		Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->save3dPoints(outputPath + OS_SEP);
		Settings::setLastUsedDirectory(outputPath, true);
	}
}

void MainWindow::on_actionExport2D_Points_triggered(bool checked)
{
	QString outputPath = QFileDialog::getExistingDirectory(this,
		tr("Save to Directory "), Settings::getLastUsedDirectory());

	if (outputPath.isNull() == false)
	{
		Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->save2dPoints(outputPath + OS_SEP);
		Settings::setLastUsedDirectory(outputPath, true);
	}
}

void MainWindow::on_actionUndistort_sequence_triggered(bool checked){
	UndistortSequenceDialog * diag = new UndistortSequenceDialog(this);
	diag->exec();
	delete diag;
}

void MainWindow::on_actionSettings_triggered(bool checked){
	SettingsDialog * diag = new SettingsDialog(this);
	diag->exec();
	delete diag;
}


void MainWindow::on_actionAbout_triggered(bool checked){
	AboutDialog * diag = new AboutDialog(this);
	diag->exec();
	delete diag;
}

//startMainFrameButtons
void MainWindow::on_pushButtonNew_Project_clicked(){
	newProject();
}

void MainWindow::on_pushButtonLoad_Project_clicked(){
	loadProject();
}

void MainWindow::on_pushButtonNewTrial_clicked()
{
	newTrial(); 
}

void MainWindow::on_action3D_world_view_triggered(bool checked){
	if(checked)worldViewDockWidget->show();
	else worldViewDockWidget->hide();
}

void MainWindow::on_actionConsole_triggered(bool checked){
	if(checked)ConsoleDockWidget::getInstance()->show();
	else ConsoleDockWidget::getInstance()->hide();
}

void MainWindow::on_actionDetailed_View_triggered(bool checked){
	if (checked){
		DetailViewDockWidget::getInstance()->show();
		Settings::setShowDetailView(true);
	}
	else {
		DetailViewDockWidget::getInstance()->hide();
		Settings::setShowDetailView(false);
	}
}

void MainWindow::on_actionPlot_triggered(bool checked)
{
	if (checked){
		PlotWindow::getInstance()->show();
		PlotWindow::getInstance()->updateMarkers(false);
	}
	else {
		PlotWindow::getInstance()->hide();
	}
}