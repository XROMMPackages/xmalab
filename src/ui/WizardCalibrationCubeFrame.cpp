#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/WizardCalibrationCubeFrame.h"
#include "ui_WizardCalibrationCubeFrame.h"
#include "ui/MainWindow.h"
#include "ui/ErrorDialog.h"
#include "ui/ConfirmationDialog.h"
#include "ui/WorkspaceNavigationFrame.h"

#include "core/Project.h"
#include "core/Camera.h"
#include "core/Trial.h"
#include "core/Settings.h"
#include "core/CalibrationImage.h"
#include "core/CalibrationObject.h"

#include "processing/BlobDetection.h"
#include "processing/CubeCalibration.h"
#include "processing/Calibration.h"

#include <QInputDialog>

#ifdef __APPLE__
	#include <OpenGL/gl.h>
	#include <OpenGL/glu.h>
#else
	#ifdef _WIN32
	  #include <windows.h>
	#endif
	#include <GL/gl.h>
	#include <GL/glu.h>
#endif

using namespace xma;

WizardCalibrationCubeFrame::WizardCalibrationCubeFrame(QWidget *parent) :
												QFrame(parent),
												frame(new Ui::WizardCalibrationCubeFrame){

	frame->setupUi(this);
#ifdef __APPLE__
	frame->pushButton->setMinimumHeight(26);
	frame->pushButtonDeleteFrame->setMinimumHeight(26);
	frame->pushButtonResetCamera->setMinimumHeight(26);
	frame->pushButtonResetFrame->setMinimumHeight(26);
#endif

	setDialog();
	frame->comboBoxPoints->setCurrentIndex(2);
	connect(State::getInstance(), SIGNAL(activeCameraChanged(int)), this, SLOT(activeCameraChanged(int)));
	connect(State::getInstance(), SIGNAL(activeFrameCalibrationChanged(int)), this, SLOT(activeFrameCalibrationChanged(int)));
	connect(State::getInstance(), SIGNAL(workspaceChanged(work_state)), this, SLOT(workspaceChanged(work_state)));

#ifndef WIN32
	frame->label_5->setText("Add a correspondance by using COMMAND+click.");
#endif
}

void WizardCalibrationCubeFrame::loadCalibrationSettings(){
	planarCalibrationObject = CalibrationObject::getInstance()->isPlanar();	
	if(!planarCalibrationObject){
		for (int i = 0; i < 4; i++){
			selectedReferencePointsIdx[i] = CalibrationObject::getInstance()->getReferenceIDs()[i];
		}
		frame->toolButtonReference1->setText( QString::number(selectedReferencePointsIdx[0] + 1) + "  " + CalibrationObject::getInstance()->getReferenceNames()[0]);
		frame->toolButtonReference2->setText( QString::number(selectedReferencePointsIdx[1] + 1) + "  " + CalibrationObject::getInstance()->getReferenceNames()[1]);
		frame->toolButtonReference3->setText( QString::number(selectedReferencePointsIdx[2] + 1) + "  " + CalibrationObject::getInstance()->getReferenceNames()[2]);
		frame->toolButtonReference4->setText( QString::number(selectedReferencePointsIdx[3] + 1) + "  " + CalibrationObject::getInstance()->getReferenceNames()[3]);
		setupManualPoints();
	}else{
		
	
	}
}

void WizardCalibrationCubeFrame::on_toolButtonReference1_clicked(){
	bool ok;
    int idx = QInputDialog::getInt(this, tr("Set Custom Idx for Reference 1"),
                                          tr("Index"),  selectedReferencePointsIdx[0] + 1,1,CalibrationObject::getInstance()->getFrameSpecifications().size(), 1, &ok);
    if (ok){
		selectedReferencePointsIdx[0] = idx -1;
        frame->toolButtonReference1->setText( QString::number(selectedReferencePointsIdx[0] + 1) + "  CUSTOM INDEX");
	}
}

void WizardCalibrationCubeFrame::on_toolButtonReference2_clicked(){
	bool ok;
    int idx = QInputDialog::getInt(this, tr("Set Custom Idx for Reference 2"),
                                          tr("Index"),  selectedReferencePointsIdx[1] + 1,1,CalibrationObject::getInstance()->getFrameSpecifications().size(), 1, &ok);
    if (ok){
		selectedReferencePointsIdx[1] = idx -1;
        frame->toolButtonReference2->setText( QString::number(selectedReferencePointsIdx[1] + 1) + "  CUSTOM INDEX");
	}
}

void WizardCalibrationCubeFrame::on_toolButtonReference3_clicked(){
	bool ok;
    int idx = QInputDialog::getInt(this, tr("Set Custom Idx for Reference 3"),
                                          tr("Index"),  selectedReferencePointsIdx[2] + 1,1,CalibrationObject::getInstance()->getFrameSpecifications().size(), 1, &ok);
    if (ok){
		selectedReferencePointsIdx[2] = idx -1;
        frame->toolButtonReference3->setText( QString::number(selectedReferencePointsIdx[2] + 1) + "  CUSTOM INDEX");
	}
}

void WizardCalibrationCubeFrame::on_toolButtonReference4_clicked(){
	bool ok;
    int idx = QInputDialog::getInt(this, tr("Set Custom Idx for Reference 4"),
                                          tr("Index"),  selectedReferencePointsIdx[3] + 1,1,CalibrationObject::getInstance()->getFrameSpecifications().size(), 1, &ok);
    if (ok){
		selectedReferencePointsIdx[3] = idx -1;
        frame->toolButtonReference4->setText( QString::number(selectedReferencePointsIdx[3] + 1) + "  CUSTOM INDEX");
	}
}

WizardCalibrationCubeFrame::~WizardCalibrationCubeFrame(){
	delete frame;
}

void WizardCalibrationCubeFrame::activeCameraChanged(int activeCamera){
	setDialog();
}

void WizardCalibrationCubeFrame::activeFrameCalibrationChanged(int activeFrame){
	setDialog();
}

void WizardCalibrationCubeFrame::workspaceChanged(work_state workspace){
	if (workspace == CALIBRATION){
		setDialog();
	}
}

bool WizardCalibrationCubeFrame::checkForPendingChanges(){
	bool hasPendingChanges = false;
	for(unsigned int j = 0; j < Project::getInstance()->getCameras().size() ; j ++){
		if(Project::getInstance()->getCameras()[j]->isRecalibrationRequired()){
			hasPendingChanges = true;
		}
	}
	
	if(hasPendingChanges){
		if (Settings::getInstance()->getBoolSetting("AutoConfirmPendingChanges") || ConfirmationDialog::getInstance()->showConfirmationDialog(
			"You have modified the calibration input and before continuing you first have to recompute the calibration. If you want to recompute click \'yes\', if you want to continue changing calibration parameters click \'cancel\' "
			)){
			QEventLoop loop;
			for(unsigned int j = 0; j < Project::getInstance()->getCameras().size() ; j ++){
				if(Project::getInstance()->getCameras()[j]->isRecalibrationRequired()){
					Calibration * calibration = new Calibration(j);
					connect(calibration, SIGNAL(computeCameraPosesAndCam_finished()), &loop, SLOT(quit()));
					calibration->computeCameraPosesAndCam();	
				}
			}
			if(Calibration::isRunning())loop.exec();

			return true;
		}else{
			return false;
		}
	}else{
		return true;
	}
}

void WizardCalibrationCubeFrame::draw(){
	for (int i = 0; i < 4 ; i++){
		glColor3f(1.0,0.0,0.0);
		glBegin(GL_LINES);
		if(selectedReferencePoints[i].x > -1 &&
			selectedReferencePoints[i].y > -1){
			glVertex2f(selectedReferencePoints[i].x-5,selectedReferencePoints[i].y-5);
			glVertex2f(selectedReferencePoints[i].x+5,selectedReferencePoints[i].y+5);
			glVertex2f(selectedReferencePoints[i].x+5,selectedReferencePoints[i].y-5);
			glVertex2f(selectedReferencePoints[i].x-5,selectedReferencePoints[i].y+5);
		}
	glEnd();
	}	
}

void WizardCalibrationCubeFrame::resetReferences(){
	    loadCalibrationSettings();

		frame->labelReference1->setText("");
		frame->labelReference2->setText("");
		frame->labelReference3->setText("");
		frame->labelReference4->setText("");

		frame->checkBoxReference1->setChecked(false);
		frame->checkBoxReference2->setChecked(false);
		frame->checkBoxReference3->setChecked(false);
		frame->checkBoxReference4->setChecked(false);

		frame->radioButtonReference1->setChecked(true);

		for (int i = 0;i < 4 ; i++){
			selectedReferencePoints[i].x = -1;
			selectedReferencePoints[i].y = -1;
		}	
}

void WizardCalibrationCubeFrame::on_comboBoxImage_currentIndexChanged(int idx){
	if(idx == 0){
		if(frame->comboBoxPoints->currentIndex() == 4){
			frame->comboBoxPoints->setCurrentIndex(2);
		}else if(frame->comboBoxPoints->currentIndex() == 5){
			frame->comboBoxPoints->setCurrentIndex(3);
		}
	}
	else if (idx == 1){
		if(frame->comboBoxPoints->currentIndex() == 2 ){
			frame->comboBoxPoints->setCurrentIndex(4);
		}else if(frame->comboBoxPoints->currentIndex() == 3){
			frame->comboBoxPoints->setCurrentIndex(5);
		}
	}

	State::getInstance()->changeCalibrationVisImage(calibrationVisImage_state(idx));
	MainWindow::getInstance()->redrawGL();
}

void WizardCalibrationCubeFrame::on_comboBoxPoints_currentIndexChanged(int idx){
	//distorted
	if(idx == 1 || idx == 2 || idx == 3){
		if(frame->comboBoxImage->currentIndex() == 1) frame->comboBoxImage->setCurrentIndex(0);
	}
	//undistorted
	else if(idx ==  4 || idx == 5){
		if(frame->comboBoxImage->currentIndex() == 0) frame->comboBoxImage->setCurrentIndex(1);
	}

	State::getInstance()->changeCalibrationVisPoints(calibrationVisPoints_state(idx));
	MainWindow::getInstance()->redrawGL();
}

void WizardCalibrationCubeFrame::on_comboBoxText_currentIndexChanged(int idx){
	State::getInstance()->changeCalibrationVisText(calibrationVisText_state(idx));
	MainWindow::getInstance()->redrawGL();
}

bool WizardCalibrationCubeFrame::manualCalibrationRunning()
{
	return frame->checkBoxManual->isChecked();
}


void WizardCalibrationCubeFrame::addCalibrationReference(double x, double y){
	if (frame->checkBoxManual->isChecked())
	{
		for (int i = 0; i < manualReferencesRadioButton.size(); i++)
		{
			if (manualReferencesRadioButton[i]->isChecked())
			{
				if (State::getInstance()->getCalibrationVisImage() == DISTORTEDCALIBIMAGE)
				{
					Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->setPoint(i,x,y, true);		
				}
				else
				{
					Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->setPoint(i, x, y, false);
				}
				Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->setInlier(i, true);
				reloadManualPoints();
				if (i + 1 < manualReferencesRadioButton.size())manualReferencesRadioButton[i + 1]->setChecked(true);
					return;
			}
		}
	}
	else{
		if (State::getInstance()->getUndistortion() == UNDISTORTED)
		{
			if (frame->radioButtonReference1->isChecked()){
				selectedReferencePoints[0].x = x;
				selectedReferencePoints[0].y = y;
				frame->checkBoxReference1->setChecked(true);
				frame->labelReference1->setText(QString::number(x, 'f', 2) + " / " + QString::number(y, 'f', 2));
				frame->radioButtonReference2->setChecked(true);
			}
			else if (frame->radioButtonReference2->isChecked()){
				selectedReferencePoints[1].x = x;
				selectedReferencePoints[1].y = y;
				frame->checkBoxReference2->setChecked(true);
				frame->labelReference2->setText(QString::number(x, 'f', 2) + " / " + QString::number(y, 'f', 2));
				frame->radioButtonReference3->setChecked(true);
			}
			else if (frame->radioButtonReference3->isChecked()){
				selectedReferencePoints[2].x = x;
				selectedReferencePoints[2].y = y;
				frame->checkBoxReference3->setChecked(true);
				frame->labelReference3->setText(QString::number(x, 'f', 2) + " / " + QString::number(y, 'f', 2));
				frame->radioButtonReference4->setChecked(true);
			}
			else if (frame->radioButtonReference4->isChecked()){
				selectedReferencePoints[3].x = x;
				selectedReferencePoints[3].y = y;
				frame->checkBoxReference4->setChecked(true);
				frame->labelReference4->setText(QString::number(x, 'f', 2) + " / " + QString::number(y, 'f', 2));
				//run calibration
				if (Settings::getInstance()->getBoolSetting("AutoCalibAfterReference")) on_pushButton_clicked();
			}
		}
	}
}

void WizardCalibrationCubeFrame::setDialog(){
	if (State::getInstance()->getUndistortion() == NOTUNDISTORTED)
	{
		frame->label->setText("You first have to perform an undistortion!");
		frame->frameReferences->hide();
		frame->frameModifyCalibration->hide();
		frame->pushButtonRemoveOutlierAutomatically->hide();
		frame->pushButtonResetCamera->hide();
		frame->pushButtonResetFrame->hide();
		frame->pushButton->hide();
		frame->checkBoxManual->hide();
		frame->frameManual->hide();
		return;
	}

	
	if(State::getInstance()->getActiveCamera() >= 0 && State::getInstance()->getActiveFrameCalibration() >= 0){
		frame->checkBoxManual->show();
		if(Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->isCalibrated() <= 0){
			resetReferences();
			frame->label->setText("Select the references");
			frame->frameReferences->show();
			if (frame->checkBoxManual->isChecked())
			{
				frame->frameReferences->hide();
				frame->frameManual->show();
				reloadManualPoints();
			}
			else
			{
				frame->frameReferences->show();
				frame->frameManual->hide();
			}
			frame->frameModifyCalibration->hide();
			frame->pushButtonRemoveOutlierAutomatically->hide();
			frame->pushButtonResetCamera->hide();
			frame->pushButtonResetFrame->hide();
			frame->pushButton->show();
			frame->pushButton->setText("compute calibration");
		}else{
			resetReferences();
			if (!frame->checkBoxManual->isChecked())
			{
				frame->frameManual->hide();
			}
			else
			{
				frame->frameManual->show();
				reloadManualPoints();
			}
			frame->label->setText("Modify calibration");
			frame->frameReferences->hide();
			frame->frameModifyCalibration->show();
			frame->pushButtonResetCamera->show();
			frame->pushButtonResetFrame->show();
			frame->pushButtonRemoveOutlierAutomatically->hide();
			frame->pushButton->show();
			frame->pushButton->setText("recompute calibration");
		}
	}
}

void WizardCalibrationCubeFrame::on_pushButton_clicked(){
	if (frame->checkBoxManual->isChecked())
	{
		int count = 0;
		for (int i = 0; i < manualReferencesCheckBox.size(); i++)
		{
			if (manualReferencesCheckBox[i]->isChecked())count++;
		}

		if (count < 7){
			ErrorDialog::getInstance()->showErrorDialog("You need to at least select 7 points for a manual calibration");
		}
		else
		{
			Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->setCalibrated(1);
			runCalibrationCameraAllFrames();
		}
	}
	else{
		if (!Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->isCalibrated() > 0){
			int count = 0;
			for (int i = 0; i < 4; i++){
				if (selectedReferencePoints[i].x >= 0 && selectedReferencePoints[i].y >= 0)count++;
			}
			if (count < 3){
				ErrorDialog::getInstance()->showErrorDialog("You need to at least select 3 reference points");
			}
			else{
				BlobDetection * blobdetection = new BlobDetection(State::getInstance()->getActiveCamera(), State::getInstance()->getActiveFrameCalibration());
				connect(blobdetection, SIGNAL(detectBlobs_finished()), this, SLOT(runCalibration()));
				blobdetection->detectBlobs();
			}
		}
		else{
			runCalibrationCameraAllFrames();
		}
	}
}

void WizardCalibrationCubeFrame::runCalibrationCameraAllFrames(){
	std::vector<Calibration *> calibs;
	for(unsigned int j = 0; j < Project::getInstance()->getCameras().size() ; j ++){
		if(Project::getInstance()->getCameras()[j]->isRecalibrationRequired()){
			Calibration * calibration = new Calibration(j);
			
			connect(calibration, SIGNAL(computeCameraPosesAndCam_finished()), this, SLOT(runCalibrationCameraAllFramesFinished()));
			calibs.push_back(calibration);
		}
	}
	for (int i = 0; i < calibs.size(); i++)
	{
		calibs[i]->computeCameraPosesAndCam();
	}
}

void WizardCalibrationCubeFrame::runCalibrationCameraAllFramesFinished(){
	setDialog();

	for (int i = 0; i < Project::getInstance()->getTrials().size(); i++)
	{
		Project::getInstance()->getTrials()[i]->update();
	}
	MainWindow::getInstance()->redrawGL();
}

void WizardCalibrationCubeFrame::runCalibration(){
	if(Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->getDetectedPointsAll().size() < 6 ){
		ErrorDialog::getInstance()->showErrorDialog("Not enough points found to run calibration"); 
		return;
	}

	CubeCalibration * calibration = new CubeCalibration(State::getInstance()->getActiveCamera(),State::getInstance()->getActiveFrameCalibration(), selectedReferencePoints,selectedReferencePointsIdx);
	if(!Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->isCalibrated()){
		connect(calibration, SIGNAL(computePoseAndCam_finished()), this, SLOT(runCalibrationFinished()));
		calibration->computePoseAndCam();	
	}else{
		connect(calibration, SIGNAL(computePose_finished()), this, SLOT(runCalibrationFinished()));
		calibration->computePose();	
	}
}

void WizardCalibrationCubeFrame::checkForCalibrationError(){
	QString errorMessage = "Error during Calibration\n";
	bool hasError = false;
	for(unsigned int j = 0; j < Project::getInstance()->getCameras().size() ; j ++){
		for(unsigned int k = 0; k < Project::getInstance()->getCameras()[j]->getCalibrationImages().size() ; k ++){
			if(Project::getInstance()->getCameras()[j]->getCalibrationImages()[k]->isCalibrated() < 0){
				hasError = true;
				errorMessage += "Camera " + QString::number(j) + " Frame " + QString::number(k) + "\n";
				Project::getInstance()->getCameras()[j]->getCalibrationImages()[k]->setCalibrated(0);
			}	
		}
	}
	if(hasError){
		errorMessage += "Please control all your data !";
		ErrorDialog::getInstance()->showErrorDialog(errorMessage);
	}
}

void WizardCalibrationCubeFrame::runCalibrationFinished(){
	checkForCalibrationError();

	bool calibrateOtherFramesFailed = false;
	for (int p = 0 ; p < temporaryCamIdx.size(); p++){
		if(!Project::getInstance()->getCameras()[temporaryCamIdx[p]]->getCalibrationImages()[temporaryFrameIdx[p]]->isCalibrated()  > 0)
		{
			ErrorDialog::getInstance()->showErrorDialog("Could not set Camera " + QString::number(temporaryCamIdx[p]) + " Frame " + QString::number(temporaryFrameIdx[p]) + " from other Frames. Please check your calibration."); 	
			calibrateOtherFramesFailed = true;
		}	
	}
	temporaryTransformationMatrix.clear();
	temporaryCamIdx.clear();
	temporaryFrameIdx.clear();

	if(!calibrateOtherFramesFailed)
		calibrateOtherFrames();

	setDialog();
	MainWindow::getInstance()->redrawGL();

	runCalibrationCameraAllFrames();
}

void WizardCalibrationCubeFrame::calibrateOtherFrames(){ 
	temporaryTransformationMatrix.clear();
	temporaryCamIdx.clear();
	temporaryFrameIdx.clear();

	cv::vector<cv::vector<cv::Mat> > CamJToCamKTransformation;
	cv::vector<cv::vector<bool> > CamJToCamKTransformationSet;

	for(unsigned int j = 0; j < Project::getInstance()->getCameras().size() ; j ++){
		cv::vector<cv::Mat>jTokTransformations;
		cv::vector<bool>jTokTransformationSet;
		
		for(unsigned int k = 0; k < Project::getInstance()->getCameras().size() ; k ++){
			cv::Mat t = cv::Mat::zeros(4,4,CV_64F);
			bool set = false;
			if(Project::getInstance()->getCameras()[k]->isCalibrated() && Project::getInstance()->getCameras()[j]->isCalibrated()){
				for (unsigned int m = 0; m < Project::getInstance()->getNbImagesCalibration(); m++){
					CalibrationImage* fk = Project::getInstance()->getCameras()[k]->getCalibrationImages()[m];
					CalibrationImage* fj = Project::getInstance()->getCameras()[j]->getCalibrationImages()[m];
					if(fk->isCalibrated()  > 0 && fj->isCalibrated()  > 0){
						t = fk->getTransformationMatrix().inv() * fj->getTransformationMatrix();
						set = true;
						break;
					}
				}	
			}
			jTokTransformations.push_back(t);
			jTokTransformationSet.push_back(set);
		}
		CamJToCamKTransformation.push_back(jTokTransformations);
		CamJToCamKTransformationSet.push_back(jTokTransformationSet);
	}

	for(unsigned int j = 0; j < Project::getInstance()->getCameras().size() ; j ++){
		for(unsigned int k = 0; k < Project::getInstance()->getCameras().size() ; k ++){
			if(Project::getInstance()->getCameras()[k]->isCalibrated() && Project::getInstance()->getCameras()[j]->isCalibrated()){
				for (unsigned int m = 0; m < Project::getInstance()->getNbImagesCalibration(); m++){
					CalibrationImage* fk = Project::getInstance()->getCameras()[k]->getCalibrationImages()[m];
					CalibrationImage* fj = Project::getInstance()->getCameras()[j]->getCalibrationImages()[m];
					if(!fk->isCalibrated() > 0 && fj->isCalibrated() > 0){
						bool save = true;	
						for (int p = 0 ; p < temporaryCamIdx.size(); p++){
							if(temporaryCamIdx[p] == k && temporaryFrameIdx[p] == m)
								save = false;
						}

						if(save && CamJToCamKTransformationSet[k][j]){
							temporaryCamIdx.push_back(k);
							temporaryFrameIdx.push_back(m);
							temporaryTransformationMatrix.push_back( fj->getTransformationMatrix()* CamJToCamKTransformation[k][j]);
							BlobDetection * blobdetection = new BlobDetection(k,m);
							connect(blobdetection, SIGNAL(detectBlobs_finished()), this, SLOT(setTransformationMatrix()));
							blobdetection->detectBlobs();	
						}
					}
				}	
			}
		}
	}
	//clean
	for(unsigned int j = 0; j < CamJToCamKTransformation.size() ; j ++){
		for(unsigned int k= 0; k < CamJToCamKTransformation[j].size() ; k++){
			CamJToCamKTransformation[j][k].release();
		}
		CamJToCamKTransformation[j].clear();
		CamJToCamKTransformationSet.clear();
	}
	CamJToCamKTransformation.clear();
	CamJToCamKTransformationSet.clear();
}


void WizardCalibrationCubeFrame::reloadManualPoints()
{
	if (CalibrationObject::getInstance()->getFrameSpecifications().size() > Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->getInliers().size()){
		Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->init(CalibrationObject::getInstance()->getFrameSpecifications().size());
	}

	for (int i = 0; i < CalibrationObject::getInstance()->getFrameSpecifications().size(); i++)
	{
		
		manualReferencesCheckBox[i]->setChecked(
				Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->getInliers()[i] > 0
			);
		manualReferencesLabel[i]->setText(
			QString::number(Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->getDetectedPoints()[i].x) + " / "+ 
			QString::number(Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->getDetectedPoints()[i].y)
			);
	}
}

void WizardCalibrationCubeFrame::setupManualPoints()
{
	for (int i = 0; i < manualReferencesCheckBox.size(); i++)
	{
		frame->gridLayout_10->removeWidget(manualReferencesCheckBox[i]);
		delete manualReferencesCheckBox[i];
		frame->gridLayout_10->removeWidget(manualReferencesLabel[i]);
		delete manualReferencesLabel[i];
		frame->gridLayout_10->removeWidget(manualReferencesRadioButton[i]);
		delete manualReferencesRadioButton[i];
	}
	manualReferencesCheckBox.clear();
	manualReferencesLabel.clear();
	manualReferencesRadioButton.clear();

	for (int i = 0; i < CalibrationObject::getInstance()->getFrameSpecifications().size(); i++)
	{
		manualReferencesCheckBox.push_back(new QCheckBox(this));
		manualReferencesLabel.push_back(new QLabel("",this));
		manualReferencesRadioButton.push_back(new QRadioButton(QString::number(i + 1),this));

		frame->gridLayout_10->addWidget(manualReferencesRadioButton[i], i, 0, 1, 1);
		frame->gridLayout_10->addWidget(manualReferencesLabel[i], i, 1, 1, 1);
		frame->gridLayout_10->addWidget(manualReferencesCheckBox[i], i, 2, 1, 1);
		connect(manualReferencesCheckBox[i], SIGNAL(clicked()), this, SLOT(checkBoxManualReference_clicked()));
	}
	manualReferencesRadioButton[0]->setChecked(true);
}

void WizardCalibrationCubeFrame::checkBoxManualReference_clicked()
{
	for (int i = 0; i < manualReferencesCheckBox.size(); i++)
	{
		Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->setInlier(
			i, manualReferencesCheckBox[i]->isChecked());
	}
	MainWindow::getInstance()->redrawGL();
}

void WizardCalibrationCubeFrame::on_checkBoxManual_clicked()
{
	reloadManualPoints();
	setDialog();
}

void WizardCalibrationCubeFrame::setTransformationMatrix(){
	for (int p = 0 ; p < temporaryCamIdx.size(); p++){
		CubeCalibration * calibration = new CubeCalibration(temporaryCamIdx[p],temporaryFrameIdx[p], selectedReferencePoints,selectedReferencePointsIdx);
		connect(calibration, SIGNAL(computePose_finished()), this, SLOT(runCalibrationFinished()));
		calibration->setPose(temporaryTransformationMatrix[p]);	
	}
}		

void WizardCalibrationCubeFrame::on_pushButtonDeleteFrame_clicked(){
	if(Project::getInstance()->getNbImagesCalibration() > 1 && ConfirmationDialog::getInstance()->showConfirmationDialog("Are you sure you want to delete frame " + QString::number(State::getInstance()->getActiveFrameCalibration()) + "  for all cameras?")){
		for(unsigned int j = 0; j < Project::getInstance()->getCameras().size() ; j ++){
			Project::getInstance()->getCameras()[j]->deleteFrame(State::getInstance()->getActiveFrameCalibration());
		}
		MainWindow::getInstance()->recountFrames();

		
		for(unsigned int j = 0; j < Project::getInstance()->getCameras().size() ; j ++){
			int countCalibrated = 0;
			for(std::vector<CalibrationImage*>::const_iterator it = Project::getInstance()->getCameras()[j]->getCalibrationImages().begin(); it != Project::getInstance()->getCameras()[j]->getCalibrationImages().end(); ++it){
				if((*it)->isCalibrated())countCalibrated++;
			}
			if(countCalibrated == 0){
				Project::getInstance()->getCameras()[j]->reset();
			}else if (countCalibrated > 0){
				Project::getInstance()->getCameras()[j]->setRecalibrationRequired(1);
			}
		}

		setDialog();
		MainWindow::getInstance()->redrawGL();
		runCalibrationCameraAllFrames();
	}
}

void WizardCalibrationCubeFrame::on_pushButtonResetCamera_clicked(){
	if(ConfirmationDialog::getInstance()->showConfirmationDialog("Are you sure you want to reset " + Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getName() + " and all detected frames")){
		 Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->reset();
		 setDialog();
		 MainWindow::getInstance()->redrawGL();
	}
}

void WizardCalibrationCubeFrame::on_pushButtonResetFrame_clicked(){
	if(ConfirmationDialog::getInstance()->showConfirmationDialog("Are you sure you want to reset Frame " +  QString::number(State::getInstance()->getActiveFrameCalibration()) +  " for " + Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getName())){
		int countCalibrated = 0;
		for(std::vector<CalibrationImage*>::const_iterator it = Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getCalibrationImages().begin(); it != Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getCalibrationImages().end(); ++it){
			if((*it)->isCalibrated())countCalibrated++;
		}

		if(countCalibrated == 1){
			Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->reset();
			setDialog();
			MainWindow::getInstance()->redrawGL();
		}else if(countCalibrated > 1){
			Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->reset();
			Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->init(CalibrationObject::getInstance()->getFrameSpecifications().size());
			Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->setRecalibrationRequired(1);
			runCalibrationCameraAllFrames();
		}
	}
}
