#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/CameraBoxTrial.h"
#include "ui_CameraBoxTrial.h"
#include "ui/ErrorDialog.h"
#include "ui/ConsoleDockWidget.h"

#include "core/Settings.h"

#include <QFileDialog>

using namespace xma;

CameraBoxTrial::CameraBoxTrial(QWidget *parent) :
												QWidget(parent),
												widget(new Ui::CameraBoxTrial){
	widget->setupUi(this);
}

CameraBoxTrial::~CameraBoxTrial(){
	delete widget;
}


void CameraBoxTrial::setCameraName(QString name){
	widget->groupBox->setTitle(name);
}

const QString CameraBoxTrial::getCameraName(){
	return widget->groupBox->title();
}

QString CameraBoxTrial::commonPrefix(QStringList fileNames){
	bool isValid = true;
	int count = 0;
	while (isValid && count < fileNames.at(0).length()){
		QString prefix = QString(fileNames.at(0).left(count + 1));
		for (int i = 0; i < fileNames.size(); i++){
			if (!fileNames.at(i).contains(prefix)){
				isValid = false;
				break;
			}
		}

		if (isValid)count++;
	}
	return QString(fileNames.at(0).left(count + 1));
}

bool CameraBoxTrial::isComplete(){
	//No Images set
	if(imageFileNames.size() == 0) {
		ErrorDialog::getInstance()->showErrorDialog(widget->groupBox->title() + " is incomplete : Missing Images");	
		return false;
	}

	//Otherwise ok
	return true;
}

void CameraBoxTrial::on_toolButton_clicked(){
	imageFileNames = QFileDialog::getOpenFileNames(this,
		tr("Open video stream movie file or images"), Settings::getInstance()->getLastUsedDirectory(), tr("Image and Video Files (*.cine *.avi *.png *.jpg *.jpeg *.bmp *.tif)"));

	imageFileNames.sort();

	if (!imageFileNames.isEmpty())
	{
		widget->lineEdit->setText(commonPrefix(imageFileNames));
		widget->label->setText("(" + QString::number(imageFileNames.size()) + ")");
		Settings::getInstance()->setLastUsedDirectory(widget->lineEdit->text());
    }
}

