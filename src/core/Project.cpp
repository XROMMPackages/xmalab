//  ----------------------------------
//  XMALab -- Copyright � 2015, Brown University, Providence, RI.
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
//  PROVIDED �AS IS�, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
//  FOR ANY PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR ANY 
//  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR FOR ANY DAMAGES WHATSOEVER RESULTING 
//  FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
//  OTHER TORTIOUS ACTION, OR ANY OTHER LEGAL THEORY, ARISING OUT OF OR IN CONNECTION 
//  WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
//  ----------------------------------
//  
///\file Project.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "core/Project.h"
#include "core/Camera.h"
#include "core/Trial.h"
#include "core/CalibrationObject.h"
#include "core/CalibrationImage.h"
#include "core/UndistortionObject.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>
#include <QXmlStreamReader>
#include <QDateTime>


using namespace xma;

Project* Project::instance = NULL;

Project::Project()
{
	projectFilename = "";
	calibrated = false;
	nbImagesCalibration = 0;
	calibrationType = INTERNAL;
	//StudyData
	hasStudyData = false;
	studyName = "";
	repository = "";
	studyID = -1;

	//CalibrationTrial Data
	trialName = "";
	trialID = -1;
	trialNumber = -1;
	trialType = "";;
	lab = "";;
	attribcomment = "";;
	ts = "";;
	trialDate = "";;
	calibrationType = INTERNAL;
	xml_data = QStringList();;
}

const bool& Project::getHasStudyData() const
{
	return hasStudyData;
}

const QString& Project::getStudyName() const
{
	return studyName;
}

const QString& Project::getRepository() const
{
	return repository;
}


const int& Project::getStudyId() const
{
	return studyID;
}

const QString& Project::getTrialName() const
{
	return trialName;
}

const int& Project::getTrialId() const
{
	return trialID;
}

const int& Project::getTrialNumber() const
{
	return trialNumber;
}

const QString& Project::getTrialType() const
{
	return trialType;
}

const QString& Project::getLab() const
{
	return lab;
}

const QString& Project::getAttribcomment() const
{
	return attribcomment;
}

const QString& Project::getTs() const
{
	return ts;
}

const QString& Project::getTrialDate() const
{
	return trialDate;
}


e_calibrationType Project::getCalibration()
{
	return calibrationType;
}

void Project::setCalibation(e_calibrationType type)
{
	calibrationType = type;
}


void Project::set_date_created()
{
	// Qt6: use current local date-time in ISO format
	date_created = QDateTime::currentDateTime().toString(Qt::ISODate);
}

bool Project::hasDefaultTrial()
{
	for (Trial* trial : trials)
		if (trial->getIsDefault()) return true;

	return false;
}

Trial* Project::getDefaultTrail()
{
	for (Trial* trial : trials)
		if (trial->getIsDefault()) return trial;

	return NULL;
}

bool Project::camerasOptimized()
{
	for (Camera* cam : cameras)
	{
		if (!cam->isOptimized())return false;
	}
	return true;
}


Project::~Project()
{
	for (Camera* cam : cameras)
		delete cam;
	cameras.clear();

	for (Trial* trial : trials)
		delete trial;
	trials.clear();

	delete CalibrationObject::getInstance();

	instance = NULL;
}

Project* Project::getInstance()
{
	if (!instance)
	{
		instance = new Project();
	}
	return instance;
}

QString Project::getProjectFilename()
{
	return projectFilename;
}

const std::vector<Camera*>& Project::getCameras()
{
	return cameras;
}

const std::vector<Trial*>& Project::getTrials()
{
	return trials;
}

Trial* Project::getTrialByName(QString Name)
{
	for (Trial* trial : trials)
	{
		if (trial->getName() == Name) return trial;
	}
	return NULL;
}

QString Project::getProjectBasename()
{
	if (projectFilename.isEmpty())
		return "";

	QFileInfo info(projectFilename);
	return info.completeBaseName();
}

int Project::getNbImagesCalibration()
{
	return nbImagesCalibration;
}

bool Project::isCalibrated()
{
	return calibrated;
}

void Project::checkCalibration()
{
	calibrated = true;
	for (Camera* cam : cameras)
		calibrated = cam->isCalibrated() && calibrated;
}

void Project::addCamera(Camera* cam)
{
	cameras.push_back(cam);
	if (cameras.size() <= cameraIDs.size()){
		int idx = cameras.size() - 1;
		cameras[idx]->setPortalId(cameraIDs[idx]);
	}
	nbImagesCalibration = cam->getCalibrationImages().size();
}

void Project::addTrial(Trial* trial)
{
	trials.push_back(trial);
}

void Project::deleteTrial(Trial* trial)
{
	std::vector<Trial *>::iterator position = std::find(trials.begin(), trials.end(), trial);
	delete *position;
	trials.erase(position);
}

void Project::replaceTrial(Trial* old_trial, Trial* new_trial)
{
	std::vector<Trial *>::iterator position = std::find(trials.begin(), trials.end(), old_trial);
	*position = new_trial;
	delete old_trial;
}

void Project::saveXMLData(QString filename)
{
	if (hasStudyData)
	{
		QFile file(filename);
		if (file.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			QTextStream out(&file);
			for (const QString &line : qAsConst(xml_data))
			{
				out << line << '\n';
			}
			file.close();
		}
	}
}

void Project::setXMLData(QString filename)
{
	QFile file(filename); // this is a name of a file text1.txt sent from main method
	if (file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		hasStudyData = true;

		QTextStream in(&file);
		QString line = in.readLine();
		while (!line.isNull())
		{		
			xml_data << line;
			line = in.readLine();
		}
		file.close();
	}

	parseXMLData(Project::getInstance()->getXMLData());
}

void Project::parseXMLData(QString xmlData)
{
	QXmlStreamReader xml(xmlData);
	std::set<int> s;
	while (!xml.atEnd() && !xml.hasError())
	{
		QXmlStreamReader::TokenType token = xml.readNext();

		if (token == QXmlStreamReader::StartDocument)
		{
			continue;
		}

		if (token == QXmlStreamReader::StartElement)
		{
			if (xml.name() == "Repository")
			{
				repository = xml.readElementText();
			}
			else if (xml.name() == "StudyName")
			{
				studyName = xml.readElementText();
			}
			else if (xml.name() == "StudyID")
			{
				studyID = xml.readElementText().toInt();
			}
			else if (xml.name() == "TrialName")
			{
				trialName = xml.readElementText();
			}
			else if (xml.name() == "TrialID")
			{
				trialID = xml.readElementText().toInt();
			}
			else if (xml.name() == "TrialNumber")
			{
				trialNumber = xml.readElementText().toInt();
			}
			else if (xml.name() == "TrialType")
			{
				trialType = xml.readElementText();
			}
			else if (xml.name() == "lab")
			{
				lab = xml.readElementText();
			}
			else if (xml.name() == "attribcomment")
			{
				attribcomment = xml.readElementText();
			}
			else if (xml.name() == "ts")
			{
				ts = xml.readElementText();
			}
			else if (xml.name() == "trialDate")
			{
				trialDate = xml.readElementText();
			}else if (xml.name() == "cameraNumber")
			{
				int c = xml.readElementText().replace("cam", "").toInt();
				s.insert(c);
			}
		}
	}

	for (int i = 0; i < s.size(); i++)
	{ 
		if (i <= cameraIDs.size())
			cameraIDs.push_back(*std::next(s.begin(), i));
	}
}

int Project::getFileID(QString filename)
{
	if (hasStudyData){
		QXmlStreamReader xml(xml_data.join(""));

		while (!xml.atEnd() && !xml.hasError())
		{
			QXmlStreamReader::TokenType token = xml.readNext();

			if (token == QXmlStreamReader::StartDocument)
			{
				continue;
			}

			if (token == QXmlStreamReader::StartElement && xml.name() == "Filename")
			{
				QString filename_xml = xml.readElementText();

				if (filename_xml == filename)
				{
					while (!xml.atEnd() && !xml.hasError())
					{
						QXmlStreamReader::TokenType token = xml.readNext();
						if (token == QXmlStreamReader::StartElement && xml.name() == "FileID")
						{
							return xml.readElementText().toInt();
						}
					}
				}
			}
		}
	}

	return -1;
}

const QString Project::getXMLData() const
{
	return xml_data.join("");
}

void Project::recountFrames()
{
	if (cameras.size() > 0)
	{
		nbImagesCalibration = cameras[0]->getCalibrationImages().size();
		for (Camera* cam : cameras)
		{
			if (nbImagesCalibration != cam->getCalibrationImages().size())
			{
				fprintf(stderr, "Error Invalid number of images");
			}
		}
	}
}

void Project::loadTextures()
{
	for (Camera* cam : cameras)
	{
		cam->loadTextures();
		QApplication::processEvents();
	}

}

void Project::reloadTextures()
{
	for (auto tr : getTrials())
	{
		for (auto vid : tr->getVideoStreams())
		{
			vid->getImage()->resetImage();
		}
	}
	for (Camera* cam : cameras)
	{
		cam->reloadTextures();
		QApplication::processEvents();
	}
}

void Project::exportDLT(QString foldername)
{
	for (int frame = 0; frame < nbImagesCalibration; frame++)
	{
		std::vector<double*> dlts;
		double allCamsSet = true;

		for (Camera* cam : cameras)
		{
			if (cam->getCalibrationImages()[frame]->isCalibrated() > 0)
			{
				double* out = new double[12];
				cam->getDLT(&out[0], frame);
				dlts.push_back(out);
			}
			else
			{
				allCamsSet = false;
			}
		}

		if (allCamsSet)
		{
			std::ofstream outfile((foldername + QDir::separator() + "MergedDlts_Frame" + QString::number(frame) + ".csv").toStdString());
			outfile.precision(17);
			for (unsigned int i = 0; i < 11; i++)
			{
				for (unsigned int c = 0; c < dlts.size(); c++)
				{
					outfile << dlts[c][i];
					if (c == dlts.size() - 1)
					{
						outfile << std::endl;
					}
					else
					{
						outfile << ",";
					}
				}
			}
			outfile.close();
		}

		for (unsigned int c = 0; c < dlts.size(); c++)
		{
			delete[] dlts[c];
		}
		dlts.clear();
	}
}

void Project::exportMayaCam(QString foldername, int frame)
{
	
	for (Camera* cam : cameras)
	{
		int start = (frame == -1) ? 0 : frame;
		int stop = (frame == -1) ? cam->getCalibrationImages().size() : frame + 1;

		for (unsigned int f = start; f < stop; f++)
		{
			if (cam->getCalibrationImages()[f]->isCalibrated() > 0)
			{
				double out[15];
				cam->getMayaCam(&out[0], f);
				std::ofstream outfile((foldername + QDir::separator() + cam->getCalibrationImages()[f]->getFilenameBase() + "_MayaCam.csv").toStdString());
				outfile.precision(17);
				for (unsigned int i = 0; i < 5; i++)
				{
					outfile << out[i * 3] << "," << out[i * 3 + 1] << "," << out[i * 3 + 2] << "\n";
				}
				outfile.close();
			}
		}
	}
}

void Project::exportMayaCamVersion2(QString foldername, int frame, int id)
{
	for (Camera* cam : cameras)
	{
		if (id == -1 || cam->getID() == id){

			int start = (frame == -1) ? 0 : frame;
			int stop = (frame == -1) ? cam->getCalibrationImages().size() : frame + 1;

			for (unsigned int f = start; f < stop; f++)
			{
				if (cam->getCalibrationImages()[f]->isCalibrated() > 0)
				{
					cam->saveMayaCamVersion2(f, foldername + QDir::separator() + cam->getCalibrationImages()[f]->getFilenameBase() + "_MayaCam" + QString::number(cam->getID() + 1) + ".txt");
				}
			}
		}
	}
}

void Project::exportLUT(QString foldername)
{
	for (Camera* cam : cameras)
	{
		if (cam->hasUndistortion() && cam->getUndistortionObject()->isComputed())
		{
			cam->getUndistortionObject()->exportData(foldername + QDir::separator() + cam->getUndistortionObject()->getFilenameBase() + "_LUT.csv",
			                                         foldername + QDir::separator() + cam->getUndistortionObject()->getFilenameBase() + "_INPTS.csv",
			                                         foldername + QDir::separator() + cam->getUndistortionObject()->getFilenameBase() + "_BSPTS.csv");
		}
	}
}

