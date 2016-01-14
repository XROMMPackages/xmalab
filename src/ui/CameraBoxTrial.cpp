//  ----------------------------------
//  XMA Lab -- Copyright © 2015, Brown University, Providence, RI.
//  
//  All Rights Reserved
//   
//  Use of the XMA Lab software is provided under the terms of the GNU General Public License version 3 
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
///\file CameraBoxTrial.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

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

CameraBoxTrial::CameraBoxTrial(QWidget* parent) :
	QWidget(parent),
	widget(new Ui::CameraBoxTrial)
{
	widget->setupUi(this);
}

CameraBoxTrial::~CameraBoxTrial()
{
	delete widget;
}


void CameraBoxTrial::setFilename(QString filename)
{
	imageFileNames.clear();
	if (filename.endsWith(".zip"))
	{
		QDir pdir(filename.replace(".zip",""));
		QStringList imageFileNames_rel = pdir.entryList(QStringList() << "*.png" << "*.tif" << "*.bmp" << "*.jpeg" << "*.jpg", QDir::Files | QDir::NoSymLinks);
		for (int i = 0; i < imageFileNames_rel.size(); ++i)
		{
			imageFileNames << QString("%1/%2").arg(pdir.absolutePath()).arg(imageFileNames_rel.at(i));
		}

		imageFileNames.sort();
		widget->lineEdit->setText(filename.replace(".zip", ""));
	}
	else{	
		imageFileNames << filename;
		widget->lineEdit->setText(commonPrefix(imageFileNames));	
	}
	widget->label->setText("(" + QString::number(imageFileNames.size()) + ")");
}

void CameraBoxTrial::setCameraName(QString name)
{
	widget->groupBox->setTitle(name);
}

const QString CameraBoxTrial::getCameraName()
{
	return widget->groupBox->title();
}

QString CameraBoxTrial::commonPrefix(QStringList fileNames)
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

bool CameraBoxTrial::isComplete()
{
	//No Images set
	if (imageFileNames.size() == 0)
	{
		ErrorDialog::getInstance()->showErrorDialog(widget->groupBox->title() + " is incomplete : Missing Images");
		return false;
	}

	//Otherwise ok
	return true;
}

void CameraBoxTrial::on_toolButtonImage_clicked()
{
	QString folder = QFileDialog::getExistingDirectory(this, tr("Load Directory "), Settings::getInstance()->getLastUsedDirectory());

	if (!folder.isEmpty())
	{
		imageFileNames.clear();
		QDir pdir(folder);
		QStringList imageFileNames_rel = pdir.entryList(QStringList() << "*.png" << "*.tif" << "*.bmp" << "*.jpeg" << "*.jpg", QDir::Files | QDir::NoSymLinks);
		for (int i = 0; i < imageFileNames_rel.size(); ++i)
		{
			imageFileNames << QString("%1/%2").arg(pdir.absolutePath()).arg(imageFileNames_rel.at(i));
		}

		imageFileNames.sort();
		widget->lineEdit->setText(folder);
		widget->label->setText("(" + QString::number(imageFileNames.size()) + ")");
		Settings::getInstance()->setLastUsedDirectory(folder);
	}
}

void CameraBoxTrial::on_toolButtonVideo_clicked()
{
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

