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
///\file CalibrationSequence.cpp
///\author Benjamin Knorlein
///\date 11/15/2016

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "core/CalibrationSequence.h"
#include "core/CalibrationObject.h"
#include "core/CalibrationImage.h"
#include "core/HelperFunctions.h"
#include "core/Image.h"
#include "core/Camera.h"
#include "core/UndistortionObject.h"
#include "core/VideoStream.h"
#include "core/CineVideo.h"
#include "core/AviVideo.h"

#include <QApplication>
#include <QFile>
#include "Settings.h"
#include <QFileDialog>
#include <QFileInfo>

using namespace xma;

CalibrationSequence::CalibrationSequence(Camera * camera)
{
	m_camera = camera;
	calibrationImages.clear();
	sequence = NULL;
	undistortedImage = NULL;
	lastUndistorted = -1;
}

CalibrationSequence::~CalibrationSequence()
{
	for (std::vector<CalibrationImage*>::iterator it = calibrationImages.begin(); it != calibrationImages.end(); ++it)
		delete *it;
	calibrationImages.clear();
	if (sequence) delete sequence;
	if (undistortedImage) delete undistortedImage;
}

void CalibrationSequence::reset()
{
	for (std::vector<CalibrationImage*>::iterator it = calibrationImages.begin(); it != calibrationImages.end(); ++it)
	{
		(*it)->reset();
		(*it)->init(CalibrationObject::getInstance()->getFrameSpecifications().size());
	}
}

void CalibrationSequence::deleteFrame(int id)
{
	if (sequence){
		calibrationImages[id]->reset();
		calibrationImages[id]->init(CalibrationObject::getInstance()->getFrameSpecifications().size());
	}
	else{
		delete calibrationImages[id];
		calibrationImages.erase(calibrationImages.begin() + id);
	}
}

void CalibrationSequence::getResolution(int& width, int& height)
{
	if (sequence || !sequence_filename.isEmpty()){
		width = sequence_width;
		height = sequence_height;
	} 
	else
	{
		width = calibrationImages[0]->getWidth();
		height = calibrationImages[0]->getHeight();
	}
}

bool CalibrationSequence::checkResolution(int width, int height)
{
	if (sequence || !sequence_filename.isEmpty())
		return true;

	for (std::vector<CalibrationImage*>::iterator it = calibrationImages.begin(); it != calibrationImages.end(); ++it)
	{
		if (width != (*it)->getWidth() || height != (*it)->getHeight()) return false;
	}
	return true;
}

void CalibrationSequence::loadImages(QStringList fileNames)
{
	if (fileNames.at(0).endsWith(".cine"))
	{	
		sequence_filename = fileNames.at(0);
		sequence = new CineVideo(fileNames);
		sequence_height = sequence->getImage()->getHeight();
		sequence_width = sequence->getImage()->getWidth();
		undistortedImage = new Image(sequence->getImage());
		for (int i = 0; i < sequence->getNbImages(); i++)
			calibrationImages.push_back(new CalibrationImage(m_camera, sequence_filename + "/Frame" + QString("%1").arg(i, 6, 10, QChar('0')) , false));
	}
	else if (fileNames.at(0).endsWith(".avi"))	{
		sequence_filename = fileNames.at(0);
		sequence = new AviVideo(fileNames);
		sequence_height = sequence->getImage()->getHeight();
		sequence_width = sequence->getImage()->getWidth();	
		undistortedImage = new Image(sequence->getImage());
		for (int i = 0; i < sequence->getNbImages(); i++)
			calibrationImages.push_back(new CalibrationImage(m_camera, sequence_filename + "/Frame" + QString("%1").arg(i, 6, 10, QChar('0')), false));
	}
	else{
		for (QStringList::const_iterator constIterator = fileNames.constBegin(); constIterator != fileNames.constEnd(); ++constIterator)
			calibrationImages.push_back(new CalibrationImage(m_camera, (*constIterator)));
	}
}

CalibrationImage* CalibrationSequence::addImage(QString fileName)
{
	CalibrationImage* image = new CalibrationImage(m_camera, fileName);
	calibrationImages.push_back(image);
	return image;
}

void CalibrationSequence::save(QString folder)
{
	for (std::vector<CalibrationImage*>::iterator it = calibrationImages.begin(); it != calibrationImages.end(); ++it)
	{
		if (!sequence) 
			(*it)->getImage()->save(folder + (*it)->getFilename());
		
		if ((*it)->isCalibrated())
		{
			(*it)->savePointsInlier(folder + "data" + OS_SEP + (*it)->getFilenamePointsInlier());
			(*it)->savePointsDetected(folder + "data" + OS_SEP + (*it)->getFilenamePointsDetected());
			(*it)->savePointsDetectedAll(folder + "data" + OS_SEP + (*it)->getFilenamePointsDetectedAll());
			(*it)->saveRotationMatrix(folder + "data" + OS_SEP + (*it)->getFilenameRotationMatrix());
			(*it)->saveTranslationVector(folder + "data" + OS_SEP + (*it)->getFilenameTranslationVector());
		}
	}
}

void CalibrationSequence::loadTextures()
{ 
	if (!sequence_filename.isEmpty() && sequence == NULL)
	{
		QFile file(sequence_filename);
		if (!file.exists())
		{
			QFileInfo info(sequence_filename);
			QString name = QFileDialog::getOpenFileName(NULL, info.fileName() + " not found. Locate and select file " + info.fileName(), Settings::getInstance()->getLastUsedDirectory(), ("Video Files (*.avi *.cine)"));
			if (!name.isEmpty())
			{
				sequence_filename = name;
			}
		}

		QStringList list;
		list << sequence_filename;
		if (sequence_filename.endsWith(".cine"))
		{
			sequence = new CineVideo(list);
			undistortedImage = new Image(sequence->getImage());
		}
		else if (sequence_filename.endsWith(".avi"))	{
			sequence = new AviVideo(list);
			undistortedImage = new Image(sequence->getImage());
		}
	}
	else if (!sequence){
		for (std::vector<CalibrationImage*>::iterator it = calibrationImages.begin(); it != calibrationImages.end(); ++it)
		{
			(*it)->loadTextures();
			QApplication::processEvents();
		}
	}
}

void CalibrationSequence::undistort()
{
	if (!sequence){
		if (m_camera->getUndistortionObject() && m_camera->getUndistortionObject()->isComputed())
		{
			for (std::vector<CalibrationImage*>::iterator it = calibrationImages.begin(); it != calibrationImages.end(); ++it)
			{
				m_camera->getUndistortionObject()->undistort((*it)->getImage(), (*it)->getUndistortedImage());
				if (m_camera->hasModelDistortion())
				{
					cv::Mat imageMat;
					(*it)->getUndistortedImage()->getImage(imageMat);
					cv::remap(imageMat, imageMat, *m_camera->getUndistortionMapX(), *m_camera->getUndistortionMapY(), cv::INTER_LANCZOS4, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
					(*it)->getUndistortedImage()->setImage(imageMat);
					imageMat.release();
				}
			}
		}
		else if (m_camera->hasModelDistortion())
		{
			for (std::vector<CalibrationImage*>::iterator it = calibrationImages.begin(); it != calibrationImages.end(); ++it)
			{
				cv::Mat imageMat;
				(*it)->getImage()->getImage(imageMat);
				cv::remap(imageMat, imageMat, *m_camera->getUndistortionMapX(), *m_camera->getUndistortionMapY(), cv::INTER_LANCZOS4, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
				(*it)->getUndistortedImage()->setImage(imageMat);
				imageMat.release();
			}
		}
	}

	for (std::vector<CalibrationImage*>::iterator it = calibrationImages.begin(); it != calibrationImages.end(); ++it)
	{
		(*it)->undistortPoints();
	}
}

Image* CalibrationSequence::getImage(int id, bool dist)
{
	if (!sequence){
		if (dist)
		{
			return calibrationImages[id]->getImage();
		}
		else
		{
			return calibrationImages[id]->getUndistortedImage();
		}
	}
	else{
		if (dist)
		{
			sequence->setActiveFrame(id);
			return sequence->getImage();
		}
		else
		{
			sequence->setActiveFrame(id);
			undstortSequenceImage(id);
			return undistortedImage;
		}
	}
}

void CalibrationSequence::bindTexture(int id, int type)
{
	if (!sequence){
		calibrationImages[id]->bindTexture(type);
	} 
	else
	{
		sequence->setActiveFrame(id);
		if (calibrationImages[id]->isCalibrated() <= 0)
		{
			sequence->getImage()->bindTexture();
		}
		else
		{
			switch (type)
			{
			case 0:
			default:
				sequence->getImage()->bindTexture();
				break;
			case 1:
				undstortSequenceImage(id);
				undistortedImage->bindTexture();
				break;
			}
		}
	}
}

bool CalibrationSequence::hasCalibrationSequence()
{
	return (sequence != NULL);
}

QString CalibrationSequence::getFilename()
{
	return sequence_filename;
}

int CalibrationSequence::getNbImages()
{
	return sequence->getNbImages();
}

void CalibrationSequence::setCalibrationSequence(QString filename, int nbImages, int width, int height)
{
	sequence_filename = filename;
	sequence_height = height;
	sequence_width = width;
	for (int i = 0; i < nbImages; i++)
		calibrationImages.push_back(new CalibrationImage(m_camera, sequence_filename + "/Frame" + QString("%1").arg(i, 6, 10, QChar('0')), false));
}

void CalibrationSequence::undstortSequenceImage(int id)
{
	if (lastUndistorted == id) return;
	lastUndistorted = id;

	if (m_camera->getUndistortionObject() && m_camera->getUndistortionObject()->isComputed())
	{
		for (std::vector<CalibrationImage*>::iterator it = calibrationImages.begin(); it != calibrationImages.end(); ++it)
		{
			m_camera->getUndistortionObject()->undistort(sequence->getImage(), undistortedImage);

			if (m_camera->hasModelDistortion())
			{
				cv::Mat imageMat;
				undistortedImage->getImage(imageMat);
				cv::remap(imageMat, imageMat, *m_camera->getUndistortionMapX(), *m_camera->getUndistortionMapY(), cv::INTER_LANCZOS4, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
				undistortedImage->setImage(imageMat);
				imageMat.release();
			}
		}
	}
	else if (m_camera->hasModelDistortion())
	{
		for (std::vector<CalibrationImage*>::iterator it = calibrationImages.begin(); it != calibrationImages.end(); ++it)
		{
			cv::Mat imageMat;
			sequence->getImage()->getImage(imageMat);
			cv::remap(imageMat, imageMat, *m_camera->getUndistortionMapX(), *m_camera->getUndistortionMapY(), cv::INTER_LANCZOS4, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
			undistortedImage->setImage(imageMat);
			imageMat.release();
		}
	}
}
