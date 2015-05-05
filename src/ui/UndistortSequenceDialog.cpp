#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif
#include "ui/UndistortSequenceDialog.h"
#include "ui_UndistortSequenceDialog.h"

#include "ui/ErrorDialog.h"
#include "ui/ConfirmationDialog.h"

#include "core/Settings.h"
#include "core/Project.h"
#include "core/Camera.h"
#include "core/CineVideo.h"
#include "core/AviVideo.h"
#include "core/UndistortionObject.h"

#include <QFileDialog>


#ifdef WIN32
	#define OS_SEP "\\"
#else
	#define OS_SEP "/"
#endif


using namespace xma;

UndistortSequenceDialog::UndistortSequenceDialog(QWidget *parent) :
												QDialog(parent),
												diag(new Ui::UndistortSequenceDialog){

	diag->setupUi(this);
	for(std::vector <Camera*>::const_iterator it = Project::getInstance()->getCameras().begin(); it != Project::getInstance()->getCameras().end(); ++it){
		if((*it)->hasUndistortion() && (*it)->getUndistortionObject()->isComputed()){
			diag->comboBox_Camera->addItem((*it)->getUndistortionObject()->getFilenameBase());
		}
	}
	diag->progressBar->setValue(0);
	diag->progressBar->hide();

	diag->lineEdit_pattern->setText(Settings::getInstance()->getQStringSetting("UndistortNamingPattern"));
}

UndistortSequenceDialog::~UndistortSequenceDialog(){
	delete diag;
}

QString UndistortSequenceDialog::commonPostfix(QStringList fileNames){
	bool isValid = true;
	int count = 0;
	//int max = (fileNames.size() > 20) ? 20 : fileNames.size();

	while (isValid && count < fileNames.at(0).length()){
		QString postfix = fileNames.at(0).right(count+1);
		for(int i = 0; i < fileNames.size() ;i++){
			if(!fileNames.at(i).contains(postfix)){
				isValid=false;
				break;
			}
		}
		
		if(isValid)count++;
	}
	return fileNames.at(0).right(count);
}

QString UndistortSequenceDialog::commonPrefix(QStringList fileNames){
	bool isValid = true;
	int count = 0;
	//int max = (fileNames.size() > 20) ? 20 : fileNames.size();

	while (isValid && count < fileNames.at(0).length()){
		QString prefix = fileNames.at(0).left(count+1);
		for(int i = 0; i <fileNames.size();i++){
			if(!fileNames.at(i).contains(prefix)){
				isValid=false;
				break;
			}
		}
		
		if(isValid)count++;
	}
	return fileNames.at(0).left(count);
}

int UndistortSequenceDialog::getNumber(QStringList fileNames){
	if(fileNames.size() > 1){
		QString number = fileNames.at(0);
		number.replace(commonPrefixString, QString(""));
		number.replace(commonPostfixString, QString(""));
		return number.toInt();
	}
	return 0;
}

void UndistortSequenceDialog::on_toolButton_Input_clicked(){
	fileNames = QFileDialog::getOpenFileNames(this,
		tr("Open Files"),Settings::getInstance()->getLastUsedDirectory(),tr("Video and Image Files (*.cine *.avi *.png *.jpg *.jpeg *.bmp *.tif)"));

	fileNames.sort();
	if ( fileNames.size() > 0 && fileNames[0].isNull() == false )
    {
        Settings::getInstance()->setLastUsedDirectory(fileNames[0]);
		commonPrefixString = commonPrefix(fileNames);
		commonPostfixString = commonPostfix(fileNames);
		diag->lineEdit_Input->setText(commonPrefixString);
		diag->spinBox_NumberStart->setValue(getNumber(fileNames));
    }
	updatePreview();
}

void UndistortSequenceDialog::on_toolButton_OutputFolder_clicked(){
	outputfolder = QFileDialog::getExistingDirectory(this,
		tr("Save to Directory "), Settings::getInstance()->getLastUsedDirectory());

	if ( outputfolder.isNull() == false )
    {
		diag->lineEdit_Outputfolder->setText(outputfolder);
		Settings::getInstance()->setLastUsedDirectory(outputfolder,true);
    }
}

void UndistortSequenceDialog::on_lineEdit_pattern_textChanged(QString text){
	Settings::getInstance()->set("UndistortNamingPattern",text);
	updatePreview();
}

void UndistortSequenceDialog::on_spinBox_NumberStart_valueChanged(int i)
{
	updatePreview();
}

void UndistortSequenceDialog::on_spinBox_NumberLength_valueChanged(int i)
{
	updatePreview();
}

void UndistortSequenceDialog::updatePreview()
{
	if (fileNames.size() > 0){
		QFileInfo info(fileNames.at(0));
		QString name = getFilename(info, diag->spinBox_NumberStart->value());
		name.replace(OS_SEP, "");
		diag->label_Preview->setText(name);
	}
}

QString UndistortSequenceDialog::getFilename(QFileInfo fileinfo, int numberFrame)
{
	QFileInfo infoNameBase(commonPrefixString);
	QString filenameBase = infoNameBase.completeBaseName();
	QString filename = fileinfo.completeBaseName();

	QString number = QString("%1").arg(numberFrame, diag->spinBox_NumberLength->value(), 10, QChar('0'));
	QString outfilename = outputfolder + OS_SEP + diag->lineEdit_pattern->text() + ".tif";
	outfilename.replace(QString("%NAME%"), filename);
	outfilename.replace(QString("%NAMEBASE%"), filenameBase);
	outfilename.replace(QString("%NUMBER%"), number);
	return outfilename;
}

bool UndistortSequenceDialog::overwriteFile(QString outfilename, bool &overwrite)
{
	QFile outfile(outfilename);
	if (!overwrite && outfile.exists()){
		if (!ConfirmationDialog::getInstance()->showConfirmationDialog("The file " + outfilename + " already exists. Are you sure you want to overwrite it?")){
			return false;
		}
		else{
			overwrite = true;
			return true;
		}
	}
	return true;
}

void UndistortSequenceDialog::on_pushButton_clicked(){
	int frameStart = diag->spinBox_NumberStart->value();
	bool overwrite = false;

	int camera_id = -1;
	int count = 0;
	for(std::vector <Camera*>::const_iterator it = Project::getInstance()->getCameras().begin(); it != Project::getInstance()->getCameras().end(); ++it){
		if((*it)->hasUndistortion() && (*it)->getUndistortionObject()->isComputed()){
			if(diag->comboBox_Camera->currentText() == (*it)->getUndistortionObject()->getFilenameBase())
				camera_id = count;
		}
		count++;
	}
	if(camera_id >= 0){
		diag->progressBar->setMaximum(fileNames.size());
		diag->progressBar->setValue(0);
		diag->progressBar->show();
		for(int i = 0; i <fileNames.size();i++){
			QFileInfo infoName(fileNames.at(i));
			if (infoName.suffix() == "cine")
			{
				QStringList list;
				list << fileNames.at(i);
				CineVideo video(list);
				diag->progressBar->setMaximum(video.getNbImages());
				for (int f = 0; i < video.getNbImages(); i++)
				{
					QString outfilename = getFilename(infoName, frameStart + f);
					if (!overwriteFile(outfilename, overwrite)) return;
					video.setActiveFrame(f);
					if (!Project::getInstance()->getCameras()[camera_id]->getUndistortionObject()->undistort(video.getImage(), outfilename)){
						ErrorDialog::getInstance()->showErrorDialog("There was a PRoblem during Undistortion. Check your data, e.g. the resolutions.");
						break;
					}
					frameStart++;
					diag->progressBar->setValue(f);
					QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
				}
			}
			else if (infoName.suffix() == "avi")
			{
				QStringList list;
				list << fileNames.at(i);
				AviVideo video(list);
				diag->progressBar->setMaximum(video.getNbImages());
				for (int f = 0; i < video.getNbImages(); i++)
				{
					QString outfilename = getFilename(infoName, frameStart + f);
					if (!overwriteFile(outfilename, overwrite)) return;
					video.setActiveFrame(f);
					if (!Project::getInstance()->getCameras()[camera_id]->getUndistortionObject()->undistort(video.getImage(), outfilename)){
						ErrorDialog::getInstance()->showErrorDialog("There was a PRoblem during Undistortion. Check your data, e.g. the resolutions.");
						break;
					}
					frameStart++;
					diag->progressBar->setValue(f);
					QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
				}
			}
			else{
				QString outfilename = getFilename(infoName, frameStart);
				if (!overwriteFile(outfilename,overwrite)) return;

				if (!Project::getInstance()->getCameras()[camera_id]->getUndistortionObject()->undistort(fileNames.at(i), outfilename)){
					ErrorDialog::getInstance()->showErrorDialog("There was a PRoblem during Undistortion. Check your data, e.g. the resolutions.");
					break;
				}
				frameStart++;
				diag->progressBar->setValue(i);
				QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
			}	
		}
	}
	diag->progressBar->hide();
}