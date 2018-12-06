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
///\file UndistortSequenceDialog.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

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
#include "core/ImageSequence.h"


#ifdef WIN32
#define OS_SEP "\\"
#else
	#define OS_SEP "/"
#endif


using namespace xma;

UndistortSequenceDialog::UndistortSequenceDialog(QWidget* parent) :
	QDialog(parent),
	diag(new Ui::UndistortSequenceDialog)
{
	diag->setupUi(this);
	for (int i = 0; i <  Project::getInstance()->getCameras().size(); i++)
	{
		Camera * cam = Project::getInstance()->getCameras()[i];
		if (cam->hasUndistortion() && cam->getUndistortionObject()->isComputed())
		{
			diag->comboBox->addItem(QString::number(i + 1));
		}
	}

	diag->pushButtonAddFolder->setEnabled(diag->comboBox->count() > 0);
	diag->pushButtonAddVideo->setEnabled(diag->comboBox->count() > 0);
	diag->pushButtonUndist->setEnabled(diag->comboBox->count() > 0);

	lastInputDir = Settings::getInstance()->getLastUsedDirectory();
	lastOutputDir = Settings::getInstance()->getLastUsedDirectory();
}

UndistortSequenceDialog::~UndistortSequenceDialog()
{
	delete diag;
}

bool compareNames2(const QString& s1, const QString& s2)
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


void UndistortSequenceDialog::on_pushButtonAddFolder_clicked()
{
	QString folder = QFileDialog::getExistingDirectory(this, tr("Image Directory "), lastInputDir);

	if (!folder.isEmpty())
	{
		lastInputDir = folder;
		QString outfolder = QFileDialog::getExistingDirectory(this, tr("Out Directory "), lastOutputDir);
		if (!outfolder.isEmpty())
		{
			lastOutputDir = outfolder;
			undist_item item;
			item.input = folder;
			item.output = outfolder;
			item.camera = diag->comboBox->currentText().toInt() - 1;
			item.isVideo = false;
			diag->listWidget->addItem("Camera " + diag->comboBox->currentText() + " from " + item.input + " to " + item.output);
			items.push(item);
		}
		//imageFileNames.clear();
		//QDir pdir(folder);
		//QStringList imageFileNames_rel = pdir.entryList(QStringList() << "*.png" << "*.tif" << "*.bmp" << "*.jpeg" << "*.jpg", QDir::Files | QDir::NoSymLinks);
		//for (int i = 0; i < imageFileNames_rel.size(); ++i)
		//{
		//	imageFileNames << QString("%1/%2").arg(pdir.absolutePath()).arg(imageFileNames_rel.at(i));
		//}

		
		//widget->lineEdit->setText(folder);
		//widget->label->setText("(" + QString::number(imageFileNames.size()) + ")");
		//Settings::getInstance()->setLastUsedDirectory(folder);
	}
}

void UndistortSequenceDialog::on_pushButtonAddVideo_clicked()
{
	QString videofile = QFileDialog::getOpenFileName(this,
		tr("Open video stream movie file"), lastInputDir, tr("Video Files (*.cine *.avi)"));

	if (!videofile.isEmpty())
	{
		QFileInfo fileinfo(videofile);
		lastInputDir =  fileinfo.absolutePath();
		QString outfolder = QFileDialog::getExistingDirectory(this, tr("Out Directory "), lastOutputDir);
		if (!outfolder.isEmpty())
		{
			lastOutputDir = outfolder;
			undist_item item;
			item.input = videofile;
			item.output = outfolder;
			item.camera = diag->comboBox->currentText().toInt() - 1;
			item.isVideo = true;
			diag->listWidget->addItem("Camera " + diag->comboBox->currentText() + " from " + item.input + " to " + item.output);
			items.push(item);
		}

		//imageFileNames.clear();
		//QDir pdir(folder);
		//QStringList imageFileNames_rel = pdir.entryList(QStringList() << "*.png" << "*.tif" << "*.bmp" << "*.jpeg" << "*.jpg", QDir::Files | QDir::NoSymLinks);
		//for (int i = 0; i < imageFileNames_rel.size(); ++i)
		//{
		//	imageFileNames << QString("%1/%2").arg(pdir.absolutePath()).arg(imageFileNames_rel.at(i));
		//}

		//qSort(imageFileNames.begin(), imageFileNames.end(), compareNames);
		//widget->lineEdit->setText(folder);
		//widget->label->setText("(" + QString::number(imageFileNames.size()) + ")");
		//Settings::getInstance()->setLastUsedDirectory(folder);
	}
}

void UndistortSequenceDialog::on_pushButtonUndist_clicked()
{
	while (!items.empty())
	{
		undist_item item = items.front();
		items.pop();
		VideoStream* stream;
		if (item.isVideo)
		{
			QFileInfo info(item.input);
			QStringList names;
			names << item.input;
			if (info.suffix() == "cine")
			{
				stream = new CineVideo(names);
			}
			else if (info.suffix() == "avi")
			{
				stream = new AviVideo(names);
			}
			else
			{
				continue;
			}
		} 
		else
		{
			QStringList imageFileNames;
			QDir pdir(item.input);
			QStringList imageFileNames_rel = pdir.entryList(QStringList() << "*.png" << "*.tif" << "*.bmp" << "*.jpeg" << "*.jpg", QDir::Files | QDir::NoSymLinks);
			for (int i = 0; i < imageFileNames_rel.size(); ++i)
			{
				imageFileNames << QString("%1/%2").arg(pdir.absolutePath()).arg(imageFileNames_rel.at(i));
			}
			qSort(imageFileNames.begin(), imageFileNames.end(), compareNames2);
			stream = new ImageSequence(imageFileNames);
		}

		QFileInfo info(stream->getFileBasename());

		for (int i = 0; i < stream->getNbImages(); i++)
		{
			stream->setActiveFrame(i);
			QString outname = item.output + OS_SEP + info.completeBaseName() + "_UND." + QString("%1").arg(i + 1, 4, 10, QChar('0')) + ".tif";
			stream->setActiveFrame(i);
			Project::getInstance()->getCameras()[item.camera]->getUndistortionObject()->undistort(stream->getImage(), outname);
		}
		delete stream;
		QListWidgetItem* list_item = diag->listWidget->takeItem(0);
		delete list_item;
		QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
	}
}
