#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/MainWindow.h"
#include "ui/WizardDockWidget.h"
#include "ui_WizardDockWidget.h"
#include "ui/WizardUndistortionFrame.h"
#include "ui/WizardCalibrationCubeFrame.h"
#include "ui/WizardDigitizationFrame.h"

#include "ui/Shortcuts.h"

#include <QLabel>

using namespace xma;

WizardDockWidget* WizardDockWidget::instance = NULL;

WizardDockWidget::WizardDockWidget(QWidget *parent) :
												QDockWidget(parent),
												dock(new Ui::WizardDockWidget){

	dock->setupUi(this);
	
	undistortionFrame = new WizardUndistortionFrame(this);
	calibrationFrame = new WizardCalibrationCubeFrame(this);
	digitizationFrame = new WizardDigitizationFrame(this);

	dock->gridLayout->addWidget(undistortionFrame, 0, 0, 1, 1);
	dock->gridLayout->addWidget(calibrationFrame, 1, 0, 1, 1);
	dock->gridLayout->addWidget(digitizationFrame, 2, 0, 1, 1);

	dock->gridLayout->setSizeConstraint(QLayout::SetFixedSize);
	connect(State::getInstance(), SIGNAL(workspaceChanged(work_state)), this, SLOT(workspaceChanged(work_state)));

	Shortcuts::getInstance()->installEventFilterToChildren(this);
}

bool WizardDockWidget::checkForPendingChanges(){
	if(State::getInstance()->getWorkspace() == UNDISTORTION){
		return undistortionFrame->checkForPendingChanges();
	}else if(State::getInstance()->getWorkspace() == CALIBRATION){
		return calibrationFrame->checkForPendingChanges();
	}
	else if (State::getInstance()->getWorkspace() == DIGITIZATION){
		return true;
	}
	return true;
}

void WizardDockWidget::update(){
	 calibrationFrame->runCalibrationCameraAllFrames();
}

WizardDockWidget::~WizardDockWidget(){
	delete dock;
	instance = NULL;
}

WizardDockWidget* WizardDockWidget::getInstance()
{
	if(!instance) 
	{
		instance = new WizardDockWidget(MainWindow::getInstance());
		MainWindow::getInstance()->addDockWidget(Qt::LeftDockWidgetArea, instance);
	}
	return instance;
}

bool WizardDockWidget::manualCalibrationRunning()
{
	return calibrationFrame->manualCalibrationRunning();
}

void WizardDockWidget::addCalibrationReference(double x, double y){
	calibrationFrame->addCalibrationReference(x,y);
}

void WizardDockWidget::addDigitizationPoint(int camera, double x, double y)
{
	digitizationFrame->addDigitizationPoint(camera, x, y);
}

void WizardDockWidget::selectDigitizationPoint(int camera, double x, double y)
{
	digitizationFrame->selectDigitizationPoint(camera, x, y);
}

void WizardDockWidget::moveDigitizationPoint(int camera, double x, double y, bool noDetection )
{
	digitizationFrame->moveDigitizationPoint(camera, x, y, noDetection);
}

void WizardDockWidget::draw(){
	if(State::getInstance()->getWorkspace() == UNDISTORTION){
	}else if(State::getInstance()->getWorkspace() == CALIBRATION){
		calibrationFrame->draw();
	}
}

void WizardDockWidget::updateDialog(){
	if (State::getInstance()->getWorkspace() == CALIBRATION){
		calibrationFrame->loadCalibrationSettings();
	}
	else if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		digitizationFrame->setDialog();
	}
}

void WizardDockWidget::stop()
{
	if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		digitizationFrame->stopTracking();
	}
}

void WizardDockWidget::workspaceChanged(work_state workspace){
	if(workspace == UNDISTORTION){
		undistortionFrame->show();
		calibrationFrame->hide();
		digitizationFrame->hide();
	}else if(workspace == CALIBRATION){
		undistortionFrame->hide();
		calibrationFrame->show();
		digitizationFrame->hide();
	}else if (workspace == DIGITIZATION){
		undistortionFrame->hide();
		calibrationFrame->hide();
		digitizationFrame->show();
	}
}

void WizardDockWidget::trackSelectedPointForward()
{
	if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		digitizationFrame->trackSelectedPointToNextFrame();
	}
}

void WizardDockWidget::trackSelectedPointBackward()
{
	if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		digitizationFrame->trackSelectedPointToPrevFrame();
	}
}

void WizardDockWidget::goToLastTrackedFrame()
{
	if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		digitizationFrame->goToLastTrackedFrame();
	}
}

void WizardDockWidget::goToFirstTrackedFrame()
{
	if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		digitizationFrame->goToFirstTrackedFrame();
	}
}