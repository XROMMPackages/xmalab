#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/WizardUndistortionFrame.h"
#include "ui_WizardUndistortionFrame.h"
#include "ui/MainWindow.h"
#include "ui/ConfirmationDialog.h"
#include "ui/WizardDockWidget.h"

#include "core/Project.h"
#include "core/Camera.h"
#include "core/Settings.h"
#include "core/UndistortionObject.h"

#include "processing/BlobDetection.h"
#include "processing/LocalUndistortion.h"
#include <QInputDialog>

using namespace xma;

WizardUndistortionFrame::WizardUndistortionFrame(QWidget *parent) :
												QFrame(parent),
												frame(new Ui::WizardUndistortionFrame){

	frame->setupUi(this);
#ifdef __APPLE__
	frame->pushButton->setMinimumHeight(26);
#endif

	undistortionChanged(NOTUNDISTORTED);
	connect(State::getInstance(), SIGNAL(undistortionChanged(undistortion_state)), this, SLOT(undistortionChanged(undistortion_state)));
}

WizardUndistortionFrame::~WizardUndistortionFrame(){
	delete frame;
}

void WizardUndistortionFrame::undistortionChanged(undistortion_state undistortion){
	if(undistortion == NOTUNDISTORTED){
		frame->label->setText("You first need to perform an Undistortion");
		frame->radioButtonMouseClickOutlier->hide();
		frame->groupBoxVisualization->hide();
		frame->pushButtonRemoveOutlierAutomatically->hide();
		frame->pushButton->setText("compute undistortion");
	}else{
		frame->label->setText("Modify Undistortion");
		frame->radioButtonMouseClickOutlier->show();
		frame->groupBoxVisualization->show();
		frame->pushButtonRemoveOutlierAutomatically->show();
		frame->comboBoxImage->setCurrentIndex(1);
		frame->comboBoxPoints->setCurrentIndex(3);
		frame->radioButtonMouseClickNone->setChecked(true);
		frame->pushButton->setText("recompute undistortion");
	}
}

void WizardUndistortionFrame::on_comboBoxImage_currentIndexChanged(int idx){
	if(idx == 0){
		if(frame->comboBoxPoints->currentIndex() == 3){
			frame->comboBoxPoints->setCurrentIndex(2);
		}
	}
	else if (idx == 1){
		if(frame->comboBoxPoints->currentIndex() == 2) {
			frame->comboBoxPoints->setCurrentIndex(3);
		}
	}

	State::getInstance()->changeUndistortionVisImage(undistortionVisImage_state(idx));
	MainWindow::getInstance()->redrawGL();
}

bool WizardUndistortionFrame::checkForPendingChanges(){
	bool hasPendingChanges = false;
	for(int i = 0; i < Project::getInstance()->getCameras().size(); i++){
		if(Project::getInstance()->getCameras()[i]->hasUndistortion()){
			if(Project::getInstance()->getCameras()[i]->getUndistortionObject()->isRecalibrationRequired() > 0){
				hasPendingChanges = true;
			}
		}
	}

	if(hasPendingChanges){
		if (Settings::getInstance()->getBoolSetting("AutoConfirmPendingChanges") || ConfirmationDialog::getInstance()->showConfirmationDialog(
			"You have modified the undistortion input and before continuing you first have to recompute the undistortion. If you want to recompute click \'yes\', if you want to continue changing undistortion parameters click \'cancel\' "
			)){
			QEventLoop loop;
			for(int i = 0; i < Project::getInstance()->getCameras().size(); i++){
				if(Project::getInstance()->getCameras()[i]->hasUndistortion()){
					if(Project::getInstance()->getCameras()[i]->getUndistortionObject()->isRecalibrationRequired() > 0){
						LocalUndistortion * localUndistortion = new LocalUndistortion(i);
						connect(localUndistortion, SIGNAL(localUndistortion_finished()), &loop, SLOT(quit()));
						if(Project::getInstance()->getCameras()[i]->getUndistortionObject()->isRecalibrationRequired() > 1)
						{
							localUndistortion->computeUndistortion(false);
						}
						else{
							localUndistortion->computeUndistortion(true);
						}
					}
				}
			}
			loop.exec();

			return true;
		}else{
			return false;
		}
	}else{
		return true;
	}
}

void WizardUndistortionFrame::on_comboBoxPoints_currentIndexChanged(int idx){
	//distorted
	if(idx == 1 || idx == 2){
		if(frame->comboBoxImage->currentIndex() == 1) frame->comboBoxImage->setCurrentIndex(0);
	}
	//undistorted
	else if(idx == 3 || idx == 4){
		if(frame->comboBoxImage->currentIndex() == 0) frame->comboBoxImage->setCurrentIndex(1);
	}

	State::getInstance()->changeUndistortionVisPoints(undistortionVisPoints_state(idx));
	MainWindow::getInstance()->redrawGL();
}

void WizardUndistortionFrame::on_radioButtonMouseClickCenter_clicked(bool checked){
	if(State::getInstance()->getUndistortion() == UNDISTORTED){
		if(ConfirmationDialog::getInstance()->showConfirmationDialog("Warning: Setting a new Center will reset all inliers. Are you sure you want to proceed?")){
			State::getInstance()->changeUndistortionMouseMode(UNDISTSETCENTER);
		}else{
			if(State::getInstance()->getUndistortionMouseMode() == UNDISTTOGGLEOUTLIER){
				frame->radioButtonMouseClickOutlier->click();
			}else if(State::getInstance()->getUndistortionMouseMode() == UNDISTNOMOUSEMODE){
				frame->radioButtonMouseClickNone->click();
			} 
		}
	}else{
		State::getInstance()->changeUndistortionMouseMode(UNDISTSETCENTER);
	}
	MainWindow::getInstance()->redrawGL();
} 

void WizardUndistortionFrame::on_radioButtonMouseClickOutlier_clicked(bool checked){
	State::getInstance()->changeUndistortionMouseMode(UNDISTTOGGLEOUTLIER);
	MainWindow::getInstance()->redrawGL();
}
void WizardUndistortionFrame::on_radioButtonMouseClickNone_clicked(bool checked){
	State::getInstance()->changeUndistortionMouseMode(UNDISTNOMOUSEMODE);
	MainWindow::getInstance()->redrawGL();
}

void WizardUndistortionFrame::on_pushButtonRemoveOutlierAutomatically_clicked()
{
	bool ok;
	int thres_border = QInputDialog::getInt(this, tr("Threshold: Distance to border"),
		tr("Threshold (Border)"),10,0,100, 1, &ok);
	if (ok){
		int thres_circle = QInputDialog::getInt(this, tr("Threshold: Distance to fitted circle"),
			tr("Threshold (Circle)"), 25, 0, 100, 1, &ok);
		if (ok){
			for (int i = 0; i < Project::getInstance()->getCameras().size(); i++){
				if (Project::getInstance()->getCameras()[i]->hasUndistortion()){
					Project::getInstance()->getCameras()[i]->getUndistortionObject()->removeOutlier(thres_circle, thres_border);
				}
			}
		}
	}
	MainWindow::getInstance()->redrawGL();
}

void WizardUndistortionFrame::on_pushButton_clicked(){
	if(State::getInstance()->getUndistortion() == NOTUNDISTORTED){	
		for(int i = 0; i < Project::getInstance()->getCameras().size(); i++){
			if(Project::getInstance()->getCameras()[i]->hasUndistortion()){
				BlobDetection * blobdetection = new BlobDetection(i,-1);
				connect(blobdetection, SIGNAL(detectBlobs_finished()), this, SLOT(computeUndistortion()));
				blobdetection->detectBlobs();
			}
		}
	}else if(State::getInstance()->getUndistortion() == UNDISTORTED){
		recomputeUndistortion();
	}
}

void WizardUndistortionFrame::computeUndistortion(){
	for(int i = 0; i < Project::getInstance()->getCameras().size(); i++){
		if(Project::getInstance()->getCameras()[i]->hasUndistortion()){
			LocalUndistortion * localUndistortion = new LocalUndistortion(i);
			connect(localUndistortion, SIGNAL(localUndistortion_finished()), this, SLOT(undistortionFinished()));
			localUndistortion->computeUndistortion();
		}
	}
}

void WizardUndistortionFrame::undistortionFinished(){
	State::getInstance()->changeUndistortion(UNDISTORTED);
	MainWindow::getInstance()->redrawGL();

	WizardDockWidget::getInstance()->update();
}

void WizardUndistortionFrame::recomputeUndistortion(){
	for(int i = 0; i < Project::getInstance()->getCameras().size(); i++){
		if(Project::getInstance()->getCameras()[i]->hasUndistortion()){
			if(Project::getInstance()->getCameras()[i]->getUndistortionObject()->isRecalibrationRequired() > 0){
				LocalUndistortion * localUndistortion = new LocalUndistortion(i);
				connect(localUndistortion, SIGNAL(localUndistortion_finished()), this, SLOT(undistortionFinished()));
				if(Project::getInstance()->getCameras()[i]->getUndistortionObject()->isRecalibrationRequired() > 1)
				{
					localUndistortion->computeUndistortion(false);
				}
				else{
					localUndistortion->computeUndistortion(true);
				}
			}
		}
	}
}