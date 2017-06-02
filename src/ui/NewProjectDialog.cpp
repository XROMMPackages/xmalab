//  ----------------------------------
//  XMALab -- Copyright © 2015, Brown University, Providence, RI.
//  
//  All Rights Reserved
//   
//  Use of the XMALab software is provided under the terms of the GNU General Public License version 3 
//  as published by the Free Software Foundation at http://www.gnu.org/licenses/gpl-3.0.html, provided 
//  that this copyright notice appear in all copies and that the name of Brown University not be used in 
//  advertising or publicity pertaining to the use or distribution of the software without specific written 
//  prior permission from Brown University.
//  
//  See license.txt for further information.
//  
//  BROWN UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE WHICH IS 
//  PROVIDED “AS IS”, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
//  FOR ANY PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR ANY 
//  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR FOR ANY DAMAGES WHATSOEVER RESULTING 
//  FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
//  OTHER TORTIOUS ACTION, OR ANY OTHER LEGAL THEORY, ARISING OUT OF OR IN CONNECTION 
//  WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
//  ----------------------------------
//  
///\file NewProjectDialog.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/NewProjectDialog.h"
#include "ui_NewProjectDialog.h"
#include "ui/CameraBox.h"
#include "ui/ErrorDialog.h"
#include "ui/MainWindow.h"
#include "ui/ProgressDialog.h"

#include "core/Settings.h"
#include "core/CalibrationObject.h"
#include "core/Camera.h"
#include "core/Project.h"

#include <QFileDialog>
#include <core/HelperFunctions.h>

using namespace xma;

NewProjectDialog::NewProjectDialog(QWidget* parent) :
	QDialog(MainWindow::getInstance()),
	diag(new Ui::NewProjectDialog)
{
	diag->setupUi(this);

	diag->frameCheckerboard->hide();
	diag->frameCube->show();

	nbCams = 2;
	diag->labelNbCameras->setText(QString::number(nbCams));

	for (int i = 0; i < nbCams; i ++)
	{
		CameraBox* box = new CameraBox();
		box->setCameraName("Camera " + QString::number(i + 1));
		diag->gridLayout_6->addWidget(box, i, 0, 1, 1);
		cameras.push_back(box);
	}
}

NewProjectDialog::~NewProjectDialog()
{
	delete diag;

	for (std::vector<CameraBox*>::iterator it = cameras.begin(); it != cameras.end(); ++it)
		delete (*it);

	cameras.clear();
}

void NewProjectDialog::addCalibrationImage(int id_camera, QString filename)
{
	while (id_camera >= (int)cameras.size())
	{
		on_toolButtonCameraPlus_clicked();
	}

	cameras[id_camera]->addCalibrationImage(filename);
}

void NewProjectDialog::addGridImage(int id_camera, QString filename)
{
	while (id_camera >= (int)cameras.size())
	{
		on_toolButtonCameraPlus_clicked();
	}
	cameras[id_camera]->addUndistortionImage(filename);
}

void NewProjectDialog::setCalibrationCubeCSV(QString filename)
{
	diag->lineEditFrameSpecifications->setText(filename);
}

void NewProjectDialog::setCalibrationCubeREF(QString filename)
{
	diag->lineEditReferencePoints->setText(filename);
}

void NewProjectDialog::setupBasedOnMissingParameters()
{
	for (std::vector<CameraBox*>::const_iterator it = getCameras().begin(); it != getCameras().end(); ++it)
	{
		if ((*it)->getUndistortionGridFileName().isEmpty())
		{
			(*it)->setIsLightCamera();
		}
	}
	if (diag->lineEditFrameSpecifications->text().isEmpty() && diag->lineEditReferencePoints->text().isEmpty())
	{
		diag->radioButtonCheckerboard->click();
	}
}

int NewProjectDialog::createProject()
{
	Project::getInstance()->setFlipImages(diag->checkBoxFlip->isChecked());

	for (std::vector<CameraBox*>::const_iterator it = getCameras().begin(); it != getCameras().end(); ++it)
	{
		Camera* cam = new Camera((*it)->getCameraName(), Project::getInstance()->getCameras().size());
		cam->loadImages((*it)->getImageFileNames());
		if ((*it)->hasUndistortion())cam->loadUndistortionImage((*it)->getUndistortionGridFileName());
		if (!cam->setResolutions())
		{
			ErrorDialog::getInstance()->showErrorDialog(cam->getName() + " : Resolutions do not match");
			return -1;
		}
		cam->setIsLightCamera((*it)->isLightCamera());
		Project::getInstance()->addCamera(cam);
	}

	if (diag->radioButtonCube->isChecked())
	{
		CalibrationObject::getInstance()->loadCoords(diag->lineEditFrameSpecifications->text(), diag->lineEditReferencePoints->text());
	}
	else
	{
		CalibrationObject::getInstance()->setCheckerboard(diag->spinBoxHorizontalSquares->value(), diag->spinBoxVerticalSquares->value(), diag->doubleSpinBoxSizeSquares->value());
	}
	return 0;
}

bool NewProjectDialog::referencesValid()
{

	std::ifstream fin;
	std::string line;
	fin.open(diag->lineEditReferencePoints->text().toAscii().data(), std::ios::binary);

	char str[100];
	int id;
	int count = 0;
	while (!littleHelper::safeGetline(fin, line).eof())
	{
		if (sscanf(line.c_str(), "%i %20[0-9a-zA-Z ]s", &id, &str[0]) == 2)
		{
			count++;
		}
	}
	fin.close();

	return count == 4;
}

bool NewProjectDialog::isComplete()
{
	int nbImages = cameras[0]->getImageFileNames().size();
	//Check if Cameras are all complete
	for (std::vector<CameraBox*>::iterator it = cameras.begin(); it != cameras.end(); ++it)
	{
		if (!(*it)->isComplete())
		{
			return false;
		}
		else if ((*it)->getImageFileNames().size() != nbImages)
		{
			ErrorDialog::getInstance()->showErrorDialog((*it)->getCameraName() + " has wrong number of images : " + QString::number((*it)->getImageFileNames().size()) +
				" instead of " + QString::number(nbImages));
			return false;
		}
	}


	//Check calibration Object
	if (diag->radioButtonCube->isChecked())
	{
		if (diag->lineEditFrameSpecifications->text().isEmpty())
		{
			ErrorDialog::getInstance()->showErrorDialog("Calibrationtarget Cube is incomplete : Frame specifications missing");
			return false;
		}
		if (diag->lineEditReferencePoints->text().isEmpty())
		{
			ErrorDialog::getInstance()->showErrorDialog("Calibrationtarget Cube is incomplete : Reference Points missing");
			return false;
		} 
		else if (!referencesValid())
		{
			ErrorDialog::getInstance()->showErrorDialog("Reference file is invalid. Please ensure that it has 4 entries and that the format is correct");
			diag->lineEditReferencePoints->setText("");
			return false;
		}
	}
	else
	{
		if (diag->spinBoxHorizontalSquares->value() <= 0)
		{
			ErrorDialog::getInstance()->showErrorDialog("Calibrationtarget Checkerboard is incomplete : Nb squares horizontal missing");
			return false;
		}
		if (diag->spinBoxVerticalSquares->value() <= 0)
		{
			ErrorDialog::getInstance()->showErrorDialog("Calibrationtarget Checkerboard is incomplete : Nb squares vertical missing");
			return false;
		}
		if (diag->doubleSpinBoxSizeSquares->value() <= 0)
		{
			ErrorDialog::getInstance()->showErrorDialog("Calibrationtarget Checkerboard is incomplete : Squaresize missing");
			return false;
		}
	}

	return true;
}


/***************************************************************
UI - SLOTS
***************************************************************/

//Cameras
void NewProjectDialog::on_toolButtonCameraMinus_clicked()
{
	if (nbCams > 1)
	{
		diag->gridLayout_6->removeWidget(cameras[nbCams - 1]);
		delete cameras[nbCams - 1];
		cameras.pop_back();

		nbCams -= 1;
		diag->labelNbCameras->setText(QString::number(nbCams));
	}
}

void NewProjectDialog::on_toolButtonCameraPlus_clicked()
{
	nbCams += 1;
	diag->labelNbCameras->setText(QString::number(nbCams));

	CameraBox* box = new CameraBox();
	box->setCameraName("Camera " + QString::number(nbCams));
	diag->gridLayout_6->addWidget(box, nbCams - 1, 0, 1, 1);
	cameras.push_back(box);
}

//Toggle Calibrationobject
void NewProjectDialog::on_radioButtonCheckerboard_clicked()
{
	diag->frameCheckerboard->show();
	diag->frameCube->hide();
}

void NewProjectDialog::on_radioButtonCube_clicked()
{
	diag->frameCheckerboard->hide();
	diag->frameCube->show();
}

//Calibrationcube files
void NewProjectDialog::on_toolButtonFrameSpecifications_clicked()
{
	QString fileName = QFileDialog::getOpenFileName(this,
	                                                tr("Open frame specification of calibration object"), Settings::getInstance()->getLastUsedDirectory(), tr("Framespec File (*.csv)"));
	if (fileName.isNull() == false)
	{
		Settings::getInstance()->setLastUsedDirectory(fileName);
		diag->lineEditFrameSpecifications->setText(fileName);
	}
}

void NewProjectDialog::on_toolButtonReferencePoints_clicked()
{
	QString fileName = QFileDialog::getOpenFileName(this,
	                                                tr("Open reference points file of calibration object"), Settings::getInstance()->getLastUsedDirectory(), tr("Reference File (*.ref)"));
	if (fileName.isNull() == false)
	{
		Settings::getInstance()->setLastUsedDirectory(fileName);
		diag->lineEditReferencePoints->setText(fileName);
	}
}

//Footer buttons
void NewProjectDialog::on_pushButton_OK_clicked()
{
	if (isComplete()) this->accept();
}

void NewProjectDialog::on_pushButton_Cancel_clicked()
{
	this->reject();
}

