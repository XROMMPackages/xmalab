/*
 * ProgressDialog.cpp
 *
 *  Created on: Nov 19, 2013
 *      Author: ben
 */


#include "ui/WorkspaceNavigationFrame.h"
#include "ui_WorkspaceNavigationFrame.h"
#include "ui/MainWindow.h"
#include "ui/WizardDockWidget.h"

#include "core/Project.h"
#include "core/Camera.h"

WorkspaceNavigationFrame* WorkspaceNavigationFrame::instance = NULL;

WorkspaceNavigationFrame::WorkspaceNavigationFrame(QWidget *parent) :
												QFrame(parent),
												frame(new Ui::WorkspaceNavigationFrame){

	frame->setupUi(this);

	connect(State::getInstance(), SIGNAL(workspaceChanged(work_state)), this, SLOT(workspaceChanged(work_state)));
	connect(State::getInstance(), SIGNAL(displayChanged(ui_state)), this, SLOT(displayChanged(ui_state)));
	connect(State::getInstance(), SIGNAL(activeCameraChanged(int)), this, SLOT(activeCameraChanged(int)));
	
}

WorkspaceNavigationFrame::~WorkspaceNavigationFrame(){
	delete frame;
	instance = NULL;
}

WorkspaceNavigationFrame* WorkspaceNavigationFrame::getInstance()
{
	if(!instance) 
	{
		instance = new WorkspaceNavigationFrame(MainWindow::getInstance());
	}
	return instance;
}


void WorkspaceNavigationFrame::setUndistortion(bool hasUndistortion){
	if(hasUndistortion){
		frame->comboBoxWorkspace->setCurrentIndex(frame->comboBoxWorkspace->findText("Undistortion") );
		on_comboBoxWorkspace_currentIndexChanged(frame->comboBoxWorkspace->currentText());
		frame->horizontalSpacer->changeSize(10,10,QSizePolicy::Expanding, QSizePolicy::Minimum);
		frame->comboBoxWorkspace->setVisible(true);
	}else{
		frame->comboBoxWorkspace->setCurrentIndex(frame->comboBoxWorkspace->findText("Calibration") );
		on_comboBoxWorkspace_currentIndexChanged(frame->comboBoxWorkspace->currentText());
		frame->horizontalSpacer->changeSize(10,10,QSizePolicy::Ignored, QSizePolicy::Minimum);
		frame->comboBoxWorkspace->setVisible(false);
	}
}

void WorkspaceNavigationFrame::addCamera(int idx, QString name){
	frame->comboBoxViewspace->insertItem(idx,name);
}

void WorkspaceNavigationFrame::removeCamera(int idx){
	frame->comboBoxViewspace->removeItem(idx);
}

void WorkspaceNavigationFrame::workspaceChanged(work_state workspace){
	if(workspace == UNDISTORTION){
		frame->comboBoxWorkspace->setCurrentIndex(frame->comboBoxWorkspace->findText("Undistortion") );
	}else if(workspace == CALIBRATION){
		frame->comboBoxWorkspace->setCurrentIndex(frame->comboBoxWorkspace->findText("Calibration") );
	}
}

void WorkspaceNavigationFrame::displayChanged(ui_state display){
	if(display == ALL_CAMERAS_FULL_HEIGHT){
		frame->comboBoxViewspace->setCurrentIndex(frame->comboBoxViewspace->findText("All Cameras - Full Height") );
	}else if(display == ALL_CAMERAS_1ROW_SCALED){
		frame->comboBoxViewspace->setCurrentIndex(frame->comboBoxViewspace->findText("All Cameras - 1 Row Scaled") );
	}else if(display == ALL_CAMERAS_2ROW_SCALED){
		frame->comboBoxViewspace->setCurrentIndex(frame->comboBoxViewspace->findText("All Cameras - 2 Row Scaled") );
	}else if(display == ALL_CAMERAS_3ROW_SCALED){
		frame->comboBoxViewspace->setCurrentIndex(frame->comboBoxViewspace->findText("All Cameras - 3 Row Scaled") );
	}else if(display == SINGLE_CAMERA){
		frame->comboBoxViewspace->setCurrentIndex(frame->comboBoxViewspace->findText(Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getName()) );
	}
}  

void WorkspaceNavigationFrame::activeCameraChanged(int activeCamera){
	if(State::getInstance()->getDisplay() == SINGLE_CAMERA){
		frame->comboBoxViewspace->setCurrentIndex(frame->comboBoxViewspace->findText(Project::getInstance()->getCameras()[activeCamera]->getName()) );
	}
}  

void WorkspaceNavigationFrame::on_comboBoxWorkspace_currentIndexChanged(QString value){
	if(currentComboBoxWorkspaceIndex != frame->comboBoxWorkspace->currentIndex()){
		if(WizardDockWidget::getInstance()->checkForPendingChanges()){
			if(value == "Undistortion"){
				State::getInstance()->changeWorkspace(UNDISTORTION);
				currentComboBoxWorkspaceIndex = frame->comboBoxWorkspace->currentIndex();
			}else if(value == "Calibration"){
				State::getInstance()->changeWorkspace(CALIBRATION);
				currentComboBoxWorkspaceIndex = frame->comboBoxWorkspace->currentIndex();
			}
		}else{
			frame->comboBoxWorkspace->setCurrentIndex(currentComboBoxWorkspaceIndex);
		}
	}
}

void WorkspaceNavigationFrame::on_comboBoxViewspace_currentIndexChanged(QString value){
	if(value == "All Cameras - Full Height"){
		State::getInstance()->changeDisplay(ALL_CAMERAS_FULL_HEIGHT);
	}else if(value == "All Cameras - 1 Row Scaled"){
		State::getInstance()->changeDisplay(ALL_CAMERAS_1ROW_SCALED);
	}else if(value == "All Cameras - 2 Row Scaled"){
		State::getInstance()->changeDisplay(ALL_CAMERAS_2ROW_SCALED);
	}else if(value == "All Cameras - 3 Row Scaled"){
		State::getInstance()->changeDisplay(ALL_CAMERAS_3ROW_SCALED);
	}else{
		//Single cameras
		int count = 0;
		for(std::vector <Camera*>::const_iterator it = Project::getInstance()->getCameras().begin(); it != Project::getInstance()->getCameras().end(); ++it){
			if( (*it)->getName() == value ){
				State::getInstance()->changeActiveCamera(count);
				State::getInstance()->changeDisplay(SINGLE_CAMERA);
				return;
			}
			count ++;
		}
	}
}

void WorkspaceNavigationFrame::setWorkState(work_state workspace){
	if(workspace == UNDISTORTION){
		frame->comboBoxWorkspace->setCurrentIndex(frame->comboBoxWorkspace->findText("Undistortion"));
	}else if(workspace == CALIBRATION){
		frame->comboBoxWorkspace->setCurrentIndex(frame->comboBoxWorkspace->findText("Calibration"));
	}
}