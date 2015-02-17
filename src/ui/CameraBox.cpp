#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/CameraBox.h"
#include "ui_CameraBox.h"
#include "ui/ErrorDialog.h"

#include "core/Settings.h"

#include <QFileDialog>

using namespace xma;

CameraBox::CameraBox(QWidget *parent) :
												QWidget(parent),
												widget(new Ui::CameraBox){
	widget->setupUi(this);
	widget->radioButtonLightCamera->hide();
}

CameraBox::~CameraBox(){
	delete widget;
}

const bool CameraBox::hasUndistortion(){
	return widget->radioButtonXRay->isChecked()&&widget->checkBoxUndistortionGrid->isChecked();
}

const QString CameraBox::getUndistortionGridFileName(){
	return widget->lineEditUndistortionGrid->text();
}

const QString CameraBox::getCameraName(){
	return widget->groupBox_Camera->title();
}

void CameraBox::setCameraName(QString name){
	widget->groupBox_Camera->setTitle(name);
}

bool CameraBox::isLightCamera(){
	return widget->radioButtonLightCamera->isChecked();
}

QString commonPrefix(QStringList fileNames){
	bool isValid = true;
	int count = 0;
	while (isValid && count < fileNames.at(0).length()){
		QString prefix = QString(fileNames.at(0).left(count + 1));
		for(int i = 0; i < fileNames.size();i++){
			if(!fileNames.at(i).contains(prefix)){
				isValid=false;
				break;
			}
		}
		
		if(isValid)count++;
	}
	return QString(fileNames.at(0).left(count + 1));
}
		
bool CameraBox::isComplete(){
	//No Images set
	if(imageFileNames.size() == 0) {
		ErrorDialog::getInstance()->showErrorDialog(widget->groupBox_Camera->title() + " is incomplete : Missing Images");
			
		return false;
	}
	//XRay camera and undistortion checked but no Image loaded
	if(widget->radioButtonXRay->isChecked() &&
		widget->checkBoxUndistortionGrid->isChecked() &&
		widget->lineEditUndistortionGrid->text().isEmpty()) {
		ErrorDialog::getInstance()->showErrorDialog(widget->groupBox_Camera->title() + " is incomplete : Missing Undistortiongrid");
		return false;
	}

	//Otherwise ok
	return true;
}

void CameraBox::addCalibrationImage(QString filename)
{
	imageFileNames << filename;
	imageFileNames.sort();

	if (imageFileNames.size() > 0 && imageFileNames[0].isNull() == false)
	{
		widget->lineEditImages->setText(commonPrefix(imageFileNames));
		widget->labelNbImages->setText("(" + QString::number(imageFileNames.size()) + ")");
	}
}

void CameraBox::addUndistortionImage(QString filename)
{
	if (filename.isNull() == false)
	{
		widget->lineEditUndistortionGrid->setText(filename);
	}
}

void CameraBox::on_toolButtonImages_clicked(){
	imageFileNames = QFileDialog::getOpenFileNames(this,
									tr("Open Calibration Images"), Settings::getLastUsedDirectory(),tr("Image Files (*.png *.jpg *.jpeg *.bmp *.tif)"));

	imageFileNames.sort();
	if ( imageFileNames.size() > 0 && imageFileNames[0].isNull() == false )
    {
        widget->lineEditImages->setText(commonPrefix(imageFileNames));
		widget->labelNbImages->setText("(" + QString::number(imageFileNames.size()) + ")" );
		Settings::setLastUsedDirectory(widget->lineEditImages->text());
    }
}

void CameraBox::on_toolButtonUndistortionGrid_clicked(){
	QString fileName = QFileDialog::getOpenFileName(this,
									tr("Open undistortion grid"), Settings::getLastUsedDirectory(),("Image Files (*.png *.jpg *.jpeg *.bmp *.tif)"));
	if ( fileName.isNull() == false )
    {
		Settings::setLastUsedDirectory(fileName);
		widget->lineEditUndistortionGrid->setText(fileName);
    }
}

void CameraBox::on_radioButtonXRay_clicked(){
	widget->lineEditUndistortionGrid->show();
	if(widget->checkBoxUndistortionGrid->isChecked()){
		widget->checkBoxUndistortionGrid->show();
		widget->toolButtonUndistortionGrid->show();
	}
}

void CameraBox::on_radioButtonLightCamera_clicked(){
	widget->lineEditUndistortionGrid->hide();
	widget->checkBoxUndistortionGrid->hide();
	widget->toolButtonUndistortionGrid->hide();
}

void CameraBox::on_checkBoxUndistortionGrid_stateChanged(int state){
	if(state == 0){ // Unchecked
		widget->lineEditUndistortionGrid->hide();
		widget->toolButtonUndistortionGrid->hide();
	}else{			//Checked
		widget->lineEditUndistortionGrid->show();
		widget->toolButtonUndistortionGrid->show();
	}
}