#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/UndistortionInfoFrame.h"
#include "ui_UndistortionInfoFrame.h"

#include "core/Camera.h"
#include "core/UndistortionObject.h"

using namespace xma;

UndistortionInfoFrame::UndistortionInfoFrame(QWidget *parent) :
												QFrame(parent),
												frame(new Ui::UndistortionInfoFrame){
	frame->setupUi(this);
}

UndistortionInfoFrame::~UndistortionInfoFrame(){
	delete frame;
}

void UndistortionInfoFrame::update(Camera * camera){
	if(camera->hasUndistortion() && camera->getUndistortionObject()->isComputed()){
		if(camera->getUndistortionObject()->isUpdateInfoRequired()){
			QString inlier;
			QString error;
			
			getInfo(camera, inlier, error);
			frame->label_Error->setText(error);
			frame->label_NbPoints->setText(inlier);
			camera->getUndistortionObject()->setUpdateInfoRequired(false);
		}
	}
	else{
		frame->label_Error->setText("");
		frame->label_NbPoints->setText("");
	}
}

void UndistortionInfoFrame::getInfo(Camera * camera,QString & inlier_string, QString &error_string){
	inlier_string.clear();
	error_string.clear();

	int count = 0; 
	double mean = 0;
	for (unsigned int i = 0 ; i <  camera->getUndistortionObject()->getInlier().size() ; i++){
		if(camera->getUndistortionObject()->getInlier()[i]){
			count ++;
			mean += camera->getUndistortionObject()->getError()[i];
		}
	}
	if(count > 0) mean = mean / count;

	count = 0; 
	double sd = 0;
	for (unsigned int i = 0 ; i <  camera->getUndistortionObject()->getInlier().size() ; i++){
		if(camera->getUndistortionObject()->getInlier()[i]){
			count ++;
			sd += (camera->getUndistortionObject()->getError()[i] - mean) * (camera->getUndistortionObject()->getError()[i] - mean);
		}
	}

	if(count > 0) sd = sd / count;

	inlier_string = QString::number(count) + " of " + QString::number(camera->getUndistortionObject()->getInlier().size() );  
	error_string =  QString::number(mean,'f',2) + " +/- " + QString::number(sd,'f',2);
}