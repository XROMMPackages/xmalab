#include "ui/MainWindow.h"
#include "ui/WizardDockWidget.h"
#include "ui_WizardDockWidget.h"
#include "ui/WizardUndistortionFrame.h"
#include "ui/WizardCalibrationCubeFrame.h"

WizardDockWidget* WizardDockWidget::instance = NULL;

WizardDockWidget::WizardDockWidget(QWidget *parent) :
												QDockWidget(parent),
												dock(new Ui::WizardDockWidget){

	dock->setupUi(this);
	
	undistortionFrame = new WizardUndistortionFrame(this);
	calibrationFrame = new WizardCalibrationCubeFrame(this);
	dock->gridLayout->addWidget(undistortionFrame, 0, 0, 1, 1);
	dock->gridLayout->addWidget(calibrationFrame, 1, 0, 1, 1);
	dock->gridLayout->setSizeConstraint(QLayout::SetFixedSize);
	connect(State::getInstance(), SIGNAL(workspaceChanged(work_state)), this, SLOT(workspaceChanged(work_state)));
}

bool WizardDockWidget::checkForPendingChanges(){
	if(State::getInstance()->getWorkspace() == work_state::UNDISTORTION){
		return undistortionFrame->checkForPendingChanges();
	}else if(State::getInstance()->getWorkspace() == work_state::CALIBRATION){
		return calibrationFrame->checkForPendingChanges();
	}										
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
		MainWindow::getInstance()->addDockWidget(Qt::BottomDockWidgetArea, instance);
		instance->setFloating(true);
		instance->hide();
	}
	return instance;
}

void WizardDockWidget::addCalibrationReference(double x, double y){
	calibrationFrame->addCalibrationReference(x,y);
}

void WizardDockWidget::draw(){
	if(State::getInstance()->getWorkspace() == work_state::UNDISTORTION){
	}else if(State::getInstance()->getWorkspace() == work_state::CALIBRATION){
		calibrationFrame->draw();
	}
}

void WizardDockWidget::updateFrames(){
	calibrationFrame->loadCalibrationSettings();
}

void WizardDockWidget::workspaceChanged(work_state workspace){
	if(workspace == work_state::UNDISTORTION){
		undistortionFrame->show();
		calibrationFrame->hide();
	}else if(workspace == work_state::CALIBRATION){
		undistortionFrame->hide();
		calibrationFrame->show();
	}
}