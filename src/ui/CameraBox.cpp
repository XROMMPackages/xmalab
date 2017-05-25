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
///\file CameraBox.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/CameraBox.h"
#include "ui_CameraBox.h"
#include "ui/ErrorDialog.h"

#include "core/Settings.h"

#include <QFileDialog>

using namespace xma;

CameraBox::CameraBox(QWidget* parent) :
	QWidget(parent),
	widget(new Ui::CameraBox)
{
	widget->setupUi(this);
}

CameraBox::~CameraBox()
{
	delete widget;
}

const bool CameraBox::hasUndistortion()
{
	return widget->radioButtonXRay->isChecked() && widget->checkBoxUndistortionGrid->isChecked();
}

const QString CameraBox::getUndistortionGridFileName()
{
	return widget->lineEditUndistortionGrid->text();
}

const QString CameraBox::getCameraName()
{
	return widget->groupBox_Camera->title();
}

void CameraBox::setCameraName(QString name)
{
	widget->groupBox_Camera->setTitle(name);
}

bool CameraBox::isLightCamera()
{
	return widget->radioButtonLightCamera->isChecked();
}

void CameraBox::setIsLightCamera()
{
	widget->radioButtonLightCamera->click();
}

QString commonPrefix(QStringList fileNames)
{
	bool isValid = true;
	int count = 0;
	while (isValid && count < fileNames.at(0).length())
	{
		QString prefix = QString(fileNames.at(0).left(count + 1));
		for (int i = 0; i < fileNames.size(); i++)
		{
			if (!fileNames.at(i).contains(prefix))
			{
				isValid = false;
				break;
			}
		}

		if (isValid)count++;
	}
	return QString(fileNames.at(0).left(count + 1));
}

bool CameraBox::isComplete()
{
	//No Images set
	if (imageFileNames.size() == 0)
	{
		ErrorDialog::getInstance()->showErrorDialog(widget->groupBox_Camera->title() + " is incomplete : Missing Images");

		return false;
	}
	//XRay camera and undistortion checked but no Image loaded
	if (widget->radioButtonXRay->isChecked() &&
		widget->checkBoxUndistortionGrid->isChecked() &&
		widget->lineEditUndistortionGrid->text().isEmpty())
	{
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

void CameraBox::on_toolButtonImages_clicked()
{
	imageFileNames = QFileDialog::getOpenFileNames(this,
	                                               tr("Open Calibration Images or video"), Settings::getInstance()->getLastUsedDirectory(), tr("Image Files (*.png *.jpg *.jpeg *.bmp *.tif *.avi *.cine)"));

	imageFileNames.sort();
	if (imageFileNames.size() > 0 && imageFileNames[0].isNull() == false)
	{
		widget->lineEditImages->setText(commonPrefix(imageFileNames));
		widget->labelNbImages->setText("(" + QString::number(imageFileNames.size()) + ")");
		Settings::getInstance()->setLastUsedDirectory(widget->lineEditImages->text());
	}
}

void CameraBox::on_toolButtonUndistortionGrid_clicked()
{
	QString fileName = QFileDialog::getOpenFileName(this,
	                                                tr("Open undistortion grid"), Settings::getInstance()->getLastUsedDirectory(), ("Image Files (*.png *.jpg *.jpeg *.bmp *.tif)"));
	if (fileName.isNull() == false)
	{
		Settings::getInstance()->setLastUsedDirectory(fileName);
		widget->lineEditUndistortionGrid->setText(fileName);
	}
}

void CameraBox::on_radioButtonXRay_clicked()
{
	widget->lineEditUndistortionGrid->show();
	if (widget->checkBoxUndistortionGrid->isChecked())
	{
		widget->checkBoxUndistortionGrid->show();
		widget->toolButtonUndistortionGrid->show();
	}
}

void CameraBox::on_radioButtonLightCamera_clicked()
{
	widget->lineEditUndistortionGrid->hide();
	widget->checkBoxUndistortionGrid->hide();
	widget->toolButtonUndistortionGrid->hide();
}

void CameraBox::on_checkBoxUndistortionGrid_stateChanged(int state)
{
	if (state == 0)
	{ // Unchecked
		widget->lineEditUndistortionGrid->hide();
		widget->toolButtonUndistortionGrid->hide();
	}
	else
	{ //Checked
		widget->lineEditUndistortionGrid->show();
		widget->toolButtonUndistortionGrid->show();
	}
}

