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

bool compareNames(const QString& s1, const QString& s2)
{
	// ignore common prefix..
	int i = 0;
	while ((i < s1.length()) && (i < s2.length()) && (s1.at(i).toLower() == s2.at(i).toLower()))
		++i;
	++i;
	// something left to compare?
	if ((i < s1.length()) && (i < s2.length()))
	{
		// get number prefix from position i - doesnt matter from which string
		int k = i - 1;
		//If not number return native comparator
		if (!s1.at(k).isNumber() || !s2.at(k).isNumber())
		{
			//Two next lines
			//E.g. 1_... < 12_...
			if (s1.at(k).isNumber())
				return false;
			if (s2.at(k).isNumber())
				return true;
			return QString::compare(s1, s2, Qt::CaseSensitive) < 0;
		}
		QString n = "";
		k--;
		while ((k >= 0) && (s1.at(k).isNumber()))
		{
			n = s1.at(k) + n;
			--k;
		}
		// get relevant/signficant number string for s1
		k = i - 1;
		QString n1 = "";
		while ((k < s1.length()) && (s1.at(k).isNumber()))
		{
			n1 += s1.at(k);
			++k;
		}

		// get relevant/signficant number string for s2
		//Decrease by
		k = i - 1;
		QString n2 = "";
		while ((k < s2.length()) && (s2.at(k).isNumber()))
		{
			n2 += s2.at(k);
			++k;
		}

		// got two numbers to compare?
		if (!n1.isEmpty() && !n2.isEmpty())
			return (n + n1).toInt() < (n + n2).toInt();
		else
		{
			// not a number has to win over a number.. number could have ended earlier... same prefix..
			if (!n1.isEmpty())
				return false;
			if (!n2.isEmpty())
				return true;
			return s1.at(i) < s2.at(i);
		}
	}
	else
	{
		// shortest string wins
		return s1.length() < s2.length();
	}
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

		qSort(imageFileNames.begin(), imageFileNames.end(), compareNames);
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

		qSort(imageFileNames.begin(), imageFileNames.end(), compareNames);
		widget->lineEdit->setText(folder);
		widget->label->setText("(" + QString::number(imageFileNames.size()) + ")");
		Settings::getInstance()->setLastUsedDirectory(folder);
	}
}

void CameraBoxTrial::on_toolButtonVideo_clicked()
{
	imageFileNames = QFileDialog::getOpenFileNames(this,
	                                               tr("Open video stream movie file or images"), Settings::getInstance()->getLastUsedDirectory(), tr("Image and Video Files (*.cine *.avi *.png *.jpg *.jpeg *.bmp *.tif)"));

	qSort(imageFileNames.begin(), imageFileNames.end(), compareNames);

	if (!imageFileNames.isEmpty())
	{
		widget->lineEdit->setText(commonPrefix(imageFileNames));
		widget->label->setText("(" + QString::number(imageFileNames.size()) + ")");
		Settings::getInstance()->setLastUsedDirectory(widget->lineEdit->text());
	}
}

