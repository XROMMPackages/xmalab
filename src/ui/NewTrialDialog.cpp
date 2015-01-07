#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/NewTrialDialog.h"
#include "ui_NewTrialDialog.h"
#include "ui/CameraBoxTrial.h"
#include "ui/ErrorDialog.h"
#include "ui/MainWindow.h"
#include "ui/WorkspaceNavigationFrame.h"

#include "core/Camera.h"
#include "core/Project.h"
#include "core/Trial.h"

using namespace xma;

NewTrialDialog::NewTrialDialog(QWidget *parent) :
												QDialog(MainWindow::getInstance()),
												diag(new Ui::NewTrialDialog){
	diag->setupUi(this);

	trialname = "Trial " + QString::number(Project::getInstance()->getTrials().size() + 1);
	diag->lineEditTrialName->setText(trialname);
	for (int i = 0; i < Project::getInstance()->getCameras().size() ; i ++){
		CameraBoxTrial* box = new CameraBoxTrial();
		box->setCameraName(Project::getInstance()->getCameras()[i]->getName());
		diag->gridLayout_6->addWidget(box, i, 0, 1, 1);
		cameras.push_back(box);
	}
}

NewTrialDialog::~NewTrialDialog(){
	delete diag;

	for (std::vector <CameraBoxTrial*>::iterator it = cameras.begin(); it != cameras.end(); ++it)
		delete (*it);

	cameras.clear();
}

bool NewTrialDialog::createTrial(){
	std::vector<QStringList> list;
	for(std::vector <CameraBoxTrial*>::const_iterator it = getCameras().begin(); it != getCameras().end(); ++it){
		list.push_back((*it)->getImageFileNames());	
	}
	Project::getInstance()->addTrial(new Trial(trialname,list));
	WorkspaceNavigationFrame::getInstance()->addTrial(trialname);
	State::getInstance()->changeActiveTrial(Project::getInstance()->getTrials().size()-1,true);
	Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->bindTextures();
	State::getInstance()->changeActiveFrameTrial(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveFrame(), true);
	State::getInstance()->changeWorkspace(DIGITIZATION,true);
	return true;
}

bool NewTrialDialog::isComplete(){
	int nbImages = cameras[0]->getImageFileNames().size();
	//Check if Cameras are all complete
	bool nameUnique = true;
	for (std::vector <Trial *>::const_iterator trial = Project::getInstance()->getTrials().begin(); trial != Project::getInstance()->getTrials().end(); ++trial)
	{
		if (trialname == (*trial)->getName())nameUnique = false;

	}

	if (!nameUnique)
	{
		ErrorDialog::getInstance()->showErrorDialog("Please choose another Trialname. A trial with the name " + trialname + " already exists.");
		return false;
	}

	for (std::vector <CameraBoxTrial*>::iterator it = cameras.begin(); it != cameras.end(); ++it){
		if(!(*it)->isComplete()){
			return false;
		}else if((*it)->getImageFileNames().size() != nbImages){
			ErrorDialog::getInstance()->showErrorDialog((*it)->getCameraName() + " has wrong number of images : " + QString::number((*it)->getImageFileNames().size()) + 
					" instead of " + QString::number(nbImages));
				return false;
		}
	}

	return true;
}


/***************************************************************
UI - SLOTS
***************************************************************/

//Footer buttons
void NewTrialDialog::on_pushButton_OK_clicked(){
	if(isComplete()) this->accept();
}

void NewTrialDialog::on_pushButton_Cancel_clicked(){
	this->reject();
}

void NewTrialDialog::on_lineEditTrialName_textChanged(QString text)
{
	trialname = text;
}