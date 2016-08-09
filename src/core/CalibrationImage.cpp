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
///\file CalibrationImage.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "core/CalibrationImage.h"
#include "core/Image.h"
#include "core/Camera.h"
#include "core/UndistortionObject.h"
#include "core/HelperFunctions.h"

#include <QFileInfo>
#include <fstream>

using namespace xma;

CalibrationImage::CalibrationImage(Camera* _camera, QString _imageFileName)
{
	camera = _camera;
	imageFileName = _imageFileName;
	image = new Image(imageFileName);

	width = image->getWidth();
	height = image->getHeight();

	undistortedImage = new Image(image);

	calibrated = 0;

	rotationvector.create(3, 1,CV_64F);
	translationvector.create(3, 1,CV_64F);
	nbInlier = 0;
}


CalibrationImage::~CalibrationImage()
{
	delete image;
	delete undistortedImage;

	rotationvector.release();
	translationvector.release();
}

void CalibrationImage::reset()
{
	rotationvector.release();
	translationvector.release();
	rotationvector.create(3, 1,CV_64F);
	translationvector.create(3, 1,CV_64F);

	setCalibrated(0);

	detectedPoints_ALL.clear();
	detectedPoints.clear();
	projectedPoints.clear();
	detectedPointsUndistorted.clear();
	projectedPointsUndistorted.clear();
	Inlier.clear();
	error.clear();
	errorUndistorted.clear();
	nbInlier = 0;
}

QString CalibrationImage::getFilename()
{
	QFileInfo info(imageFileName);
	return info.fileName();
}

QString CalibrationImage::getFilenameBase()
{
	QFileInfo info(imageFileName);
	return info.completeBaseName();
}

void CalibrationImage::loadTextures()
{
	image->loadTexture();
	undistortedImage->loadTexture();
}

void CalibrationImage::init(int nbPoints)
{
	detectedPoints_ALL.clear();
	detectedPoints.clear();
	projectedPoints.clear();
	detectedPointsUndistorted.clear();
	projectedPointsUndistorted.clear();
	Inlier.clear();

	for (int i = 0; i < nbPoints; i++)
	{
		detectedPoints.push_back(cv::Point2d(-1, -1));
		projectedPoints.push_back(cv::Point2d(-1, -1));
		detectedPointsUndistorted.push_back(cv::Point2d(-1, -1));
		projectedPointsUndistorted.push_back(cv::Point2d(-1, -1));
		Inlier.push_back(0);
	}
}

void CalibrationImage::setDetectedPoints(cv::vector<cv::Point2d>& points)
{
	detectedPoints_ALL.clear();
	for (std::vector<cv::Point2d>::const_iterator it = points.begin(); it != points.end(); ++it)
	{
		detectedPoints_ALL.push_back(cv::Point2d((*it).x, (*it).y));
	}
}

void CalibrationImage::setPointsUndistorted(cv::vector<cv::Point2d>& _detectedPoints, cv::vector<cv::Point2d>& _projectedPoints, cv::vector<bool>& _Inlier)
{
	detectedPointsUndistorted.clear();
	for (std::vector<cv::Point2d>::const_iterator it = _detectedPoints.begin(); it != _detectedPoints.end(); ++it)
	{
		detectedPointsUndistorted.push_back(cv::Point2d((*it).x, (*it).y));
	}

	detectedPoints.clear();
	for (std::vector<cv::Point2d>::const_iterator it = detectedPointsUndistorted.begin(); it != detectedPointsUndistorted.end(); ++it)
	{
		detectedPoints.push_back(camera->undistortPoint((*it), false));
	}

	projectedPointsUndistorted.clear();
	for (std::vector<cv::Point2d>::const_iterator it = _projectedPoints.begin(); it != _projectedPoints.end(); ++it)
	{
		projectedPointsUndistorted.push_back(cv::Point2d((*it).x, (*it).y));
	}

	projectedPoints.clear();
	for (std::vector<cv::Point2d>::const_iterator it = projectedPointsUndistorted.begin(); it != projectedPointsUndistorted.end(); ++it)
	{
		projectedPoints.push_back(camera->undistortPoint((*it), false));
	}

	Inlier.clear();
	std::vector<cv::Point2d>::const_iterator it_pt = detectedPointsUndistorted.begin();
	for (std::vector<bool>::const_iterator it = _Inlier.begin(); it != _Inlier.end(); ++it , ++it_pt)
	{
		if ((*it))
		{
			Inlier.push_back(1);
		}
		else
		{
			if ((*it_pt).x == -1 && (*it_pt).y == -1)
			{
				Inlier.push_back(-1);
			}
			else
			{
				Inlier.push_back(0);
			}
		}
	}

	computeError();
}

void CalibrationImage::undistortPoints()
{
	detectedPointsUndistorted.clear();
	for (std::vector<cv::Point2d>::const_iterator it = detectedPoints.begin(); it != detectedPoints.end(); ++it)
	{
		detectedPointsUndistorted.push_back(camera->undistortPoint((*it), true));
	}

	projectedPointsUndistorted.clear();
	for (std::vector<cv::Point2d>::const_iterator it = projectedPoints.begin(); it != projectedPoints.end(); ++it)
	{
		projectedPointsUndistorted.push_back(camera->undistortPoint((*it), true));
	}
	computeError();
}


void CalibrationImage::setPointsProjectedUndistorted(cv::vector<cv::Point2d>& _projectedPoints)
{
	projectedPointsUndistorted.clear();
	for (std::vector<cv::Point2d>::const_iterator it = _projectedPoints.begin(); it != _projectedPoints.end(); ++it)
	{
		projectedPointsUndistorted.push_back(cv::Point2d((*it).x, (*it).y));
	}

	projectedPoints.clear();
	for (std::vector<cv::Point2d>::const_iterator it = projectedPointsUndistorted.begin(); it != projectedPointsUndistorted.end(); ++it)
	{
		projectedPoints.push_back(camera->undistortPoint((*it), false));
	}

	computeError();
}


void CalibrationImage::computeError()
{
	error.clear();
	errorUndistorted.clear();

	cv::Point2d diff;
	for (unsigned int i = 0; i < projectedPoints.size(); i++)
	{
		diff = detectedPoints[i] - projectedPoints[i];
		error.push_back(cv::sqrt(diff.x * diff.x + diff.y * diff.y));

		diff = detectedPointsUndistorted[i] - projectedPointsUndistorted[i];
		errorUndistorted.push_back(cv::sqrt(diff.x * diff.x + diff.y * diff.y));
	}
}


void CalibrationImage::setMatrices(cv::Mat& _rotationvector, cv::Mat& _translationvector)
{
	rotationvector = _rotationvector.clone();
	translationvector = _translationvector.clone();
	setCalibrated(1);
}

cv::Mat CalibrationImage::getRotationVector()
{
	return rotationvector.clone();
}

cv::Mat CalibrationImage::getTranslationVector()
{
	return translationvector.clone();
}


void CalibrationImage::drawPoints(std::vector<cv::Point2d>& points, bool drawAllPoints)
{
	if (points.size() != Inlier.size()) return;

	std::vector<int>::const_iterator it_inlier = Inlier.begin();
	glBegin(GL_LINES);
	for (std::vector<cv::Point2d>::const_iterator it = points.begin(); it != points.end(); ++it , ++it_inlier)
	{
		if ((*it_inlier) == 1)
		{
			glColor3f(0.0, 1.0, 0.0);
		}
		else if ((*it_inlier) == 0)
		{
			glColor3f(1.0, 0.0, 0.0);
		}
		else if ((*it_inlier) == -1)
		{
			glColor3f(0.0, 0.0, 1.0);
			if (!drawAllPoints)continue;
		}

		glVertex2f((*it).x - 5, (*it).y);
		glVertex2f((*it).x + 5, (*it).y);
		glVertex2f((*it).x, (*it).y - 5);
		glVertex2f((*it).x, (*it).y + 5);
	}
	glEnd();
}

void CalibrationImage::draw(int type)
{
	//if(isCalibrated() <= 0) return;

	switch (type)
	{
	case 0:
	default:
		break;
	case 1:
		glColor3f(1.0, 0.0, 0.0);
		glBegin(GL_LINES);
		for (std::vector<cv::Point2d>::const_iterator it = detectedPoints_ALL.begin(); it != detectedPoints_ALL.end(); ++it)
		{
			glVertex2f((*it).x - 2, (*it).y);
			glVertex2f((*it).x + 2, (*it).y);
			glVertex2f((*it).x, (*it).y - 2);
			glVertex2f((*it).x, (*it).y + 2);
		}
		glEnd();
		break;
	case 2:
		drawPoints(detectedPoints);
		break;
	case 3:
		drawPoints(projectedPoints, true);
		break;
	case 4:
		drawPoints(detectedPointsUndistorted);
		break;
	case 5:
		drawPoints(projectedPointsUndistorted, true);
		break;
	}
}

void CalibrationImage::getDrawTextData(int type, bool distorted, std::vector<double>& x, std::vector<double>& y, std::vector<QString>& text, std::vector<bool>& inlier)
{
	x.clear();
	y.clear();
	text.clear();
	inlier.clear();
	if (Inlier.size() == projectedPoints.size() &&
		Inlier.size() == projectedPointsUndistorted.size())
	{
		for (unsigned int i = 0; i < Inlier.size(); i++)
		{
			if (Inlier[i] == 1)
			{
				inlier.push_back(true);
			}
			else
			{
				inlier.push_back(false);
				if (Inlier[i] < 0) text.push_back("");
			}

			if (distorted)
			{
				x.push_back(projectedPoints[i].x);
				y.push_back(projectedPoints[i].y);
				if (type >= 2 && Inlier[i] >= 0) text.push_back(QString::number(error[i], 'f', 2));
			}
			else
			{
				x.push_back(projectedPointsUndistorted[i].x);
				y.push_back(projectedPointsUndistorted[i].y);
				if (type >= 2 && Inlier[i] >= 0) text.push_back(QString::number(errorUndistorted[i], 'f', 2));
			}

			if (type == 1 && Inlier[i] >= 0) text.push_back(QString::number(i + 1));
		}
	}
}


cv::Mat CalibrationImage::getTransformationMatrix()
{
	cv::Mat trans;
	trans.create(4, 4,CV_64FC1);
	cv::Mat rotationmatrix;

	if (isCalibrated() > 0)
	{
		//set R
		cv::Rodrigues(rotationvector, rotationmatrix);

		for (unsigned int i = 0; i < 3; ++i)
		{
			for (unsigned int j = 0; j < 3; ++j)
			{
				trans.at<double>(i, j) = rotationmatrix.at<double>(i, j);
			}
		}
		//set t
		for (unsigned int j = 0; j < 3; ++j)
		{
			trans.at<double>(j, 3) = translationvector.at<double>(j, 0);
		}

		for (unsigned int j = 0; j < 3; ++j)
		{
			trans.at<double>(3, j) = 0 ;
		}
		trans.at<double>(3, 3) = 1;
	}

	return trans.inv();
}

void CalibrationImage::bindTexture(int type)
{
	if (isCalibrated() <= 0)
	{
		image->bindTexture();
	}
	else
	{
		switch (type)
		{
		case 0:
		default:
			image->bindTexture();
			break;
		case 1:
			undistortedImage->bindTexture();
			break;
		}
	}
}

void CalibrationImage::toggleInlier(double x, double y, bool isDistortedView)
{
	int idx = -1;
	double mindist = 20;
	double dist = mindist;
	cv::Point2d diff;
	cv::Point2d pt = cv::Point2d(x, y);
	for (unsigned int i = 0; i < projectedPoints.size(); i ++)
	{
		if (isDistortedView)
		{
			diff = pt - projectedPoints[i];
		}
		else
		{
			diff = pt - projectedPointsUndistorted[i];
		}
		dist = cv::sqrt(diff.x * diff.x + diff.y * diff.y);

		if (dist < mindist)
		{
			mindist = dist;
			idx = i;
		}
	}

	if (idx >= 0)
	{
		if (Inlier[idx] == 0)
		{
			Inlier[idx] = 1;
			camera->setRecalibrationRequired(1);
		}
		else if (Inlier[idx] >= 1)
		{
			Inlier[idx] = 0;
			camera->setRecalibrationRequired(1);
		}
	}
}

void CalibrationImage::setInlier(int idx, bool value)
{
	if (value)
	{
		Inlier[idx] = 1;
	}
	else
	{
		Inlier[idx] = 0;
	}
	camera->setRecalibrationRequired(1);
}

void CalibrationImage::setPoint(int idx, double x, double y, bool distorted)
{
	if (distorted)
	{
		detectedPoints[idx].x = x;
		detectedPoints[idx].y = y;
		detectedPointsUndistorted[idx] = camera->undistortPoint(detectedPoints[idx], true);
	}
	else
	{
		detectedPointsUndistorted[idx].x = x;
		detectedPointsUndistorted[idx].y = y;
		detectedPoints[idx] = camera->undistortPoint(detectedPointsUndistorted[idx], false);
	}
}

void CalibrationImage::setPointManual(double x, double y, bool isDistortedView)
{
	int idx = -1;
	double mindist = 20;
	double dist = mindist;
	cv::Point2d diff;
	cv::Point2d pt = cv::Point2d(x, y);
	for (unsigned int i = 0; i < projectedPoints.size(); i ++)
	{
		if (isDistortedView)
		{
			diff = pt - projectedPoints[i];
		}
		else
		{
			diff = pt - projectedPointsUndistorted[i];
		}
		dist = cv::sqrt(diff.x * diff.x + diff.y * diff.y);

		if (dist < mindist)
		{
			mindist = dist;
			idx = i;
		}
	}

	if (idx >= 0)
	{
		Inlier[idx] = 1;
		camera->setRecalibrationRequired(1);
		if (isDistortedView)
		{
			detectedPoints[idx] = pt;
			detectedPointsUndistorted[idx] = camera->undistortPoint(detectedPoints[idx], true);
		}
		else
		{
			detectedPointsUndistorted[idx] = pt;
			detectedPoints[idx] = camera->undistortPoint(detectedPointsUndistorted[idx], false);
		}

		cv::Point2d diff;
		diff = detectedPoints[idx] - projectedPoints[idx];
		error[idx] = cv::sqrt(diff.x * diff.x + diff.y * diff.y);

		diff = detectedPointsUndistorted[idx] - projectedPointsUndistorted[idx];
		errorUndistorted[idx] = cv::sqrt(diff.x * diff.x + diff.y * diff.y);
	}
}

QString CalibrationImage::getFilenamePointsInlier()
{
	QFileInfo info(imageFileName);
	return info.completeBaseName() + "_PointsInlier.csv";
}

QString CalibrationImage::getFilenamePointsDetected()
{
	QFileInfo info(imageFileName);
	return info.completeBaseName() + "_PointsDetected.csv";
}

QString CalibrationImage::getFilenamePointsDetectedAll()
{
	QFileInfo info(imageFileName);
	return info.completeBaseName() + "_PointsDetectedAll.csv";
}

QString CalibrationImage::getFilenameRotationMatrix()
{
	QFileInfo info(imageFileName);
	return info.completeBaseName() + "_RotationMatrix.csv";
}

QString CalibrationImage::getFilenameTranslationVector()
{
	QFileInfo info(imageFileName);
	return info.completeBaseName() + "_TranslationVector.csv";
}

void CalibrationImage::savePoints(std::vector<cv::Point2d>& points, QString filename)
{
	std::ofstream outfile(filename.toAscii().data());
	outfile.precision(12);
	for (std::vector<cv::Point2d>::const_iterator it = points.begin(); it != points.end(); ++it)
	{
		outfile << (*it).x << ", " << (*it).y << std::endl;
	}
	outfile.close();
}

void CalibrationImage::savePointsInlier(QString filename)
{
	std::ofstream outfile(filename.toAscii().data());
	outfile.precision(12);
	for (std::vector<int>::const_iterator it = Inlier.begin(); it != Inlier.end(); ++it)
	{
		outfile << (*it) << std::endl;
	}
	outfile.close();
}

void CalibrationImage::savePointsDetected(QString filename)
{
	savePoints(detectedPoints, filename);
}

void CalibrationImage::savePointsDetectedAll(QString filename)
{
	savePoints(detectedPoints_ALL, filename);
}

void CalibrationImage::saveRotationMatrix(QString filename)
{
	cv::Mat rotationmatrix;
	rotationmatrix.create(3, 3,CV_64F);
	cv::Rodrigues(rotationvector, rotationmatrix);

	std::ofstream outfile(filename.toAscii().data());
	outfile.precision(12);
	for (unsigned int i = 0; i < 3; ++i)
	{
		outfile << rotationmatrix.at<double>(i, 0) << ", " << rotationmatrix.at<double>(i, 1) << ", " << rotationmatrix.at<double>(i, 2) << std::endl;
	}

	outfile.close();
	rotationmatrix.release();
}

void CalibrationImage::saveTranslationVector(QString filename)
{
	std::ofstream outfile(filename.toAscii().data());
	outfile.precision(12);
	for (unsigned int i = 0; i < 3; ++i)
	{
		outfile << translationvector.at<double>(i, 0) << std::endl;
	}
	outfile.close();
}

void CalibrationImage::loadPoints(cv::vector<cv::Point2d>& points, QString filename)
{
	std::vector<std::vector<double> > values;
	std::ifstream fin(filename.toAscii().data());
	std::istringstream in;
	std::string line;
	while (!littleHelper::safeGetline(fin, line).eof())
	{
		in.clear();
		in.str(line);
		std::vector<double> tmp;
		for (double value; in >> value; littleHelper::comma(in))
		{
			tmp.push_back(value);
		}
		if (tmp.size() > 0) values.push_back(tmp);
	}
	fin.close();

	points.clear();

	for (unsigned int y = 0; y < values.size(); y++)
	{
		points.push_back(cv::Point2d(values[y][0], values[y][1]));
	}

	for (unsigned int y = 0; y < values.size(); y++)
	{
		values[y].clear();
	}
	values.clear();
}

void CalibrationImage::loadPointsInlier(QString filename)
{
	std::vector<std::vector<double> > values;
	std::ifstream fin(filename.toAscii().data());
	std::istringstream in;
	std::string line;
	while (!littleHelper::safeGetline(fin, line).eof())
	{
		in.clear();
		in.str(line);
		std::vector<double> tmp;
		for (double value; in >> value; littleHelper::comma(in))
		{
			tmp.push_back(value);
		}
		if (tmp.size() > 0) values.push_back(tmp);
	}
	fin.close();

	Inlier.clear();

	for (unsigned int y = 0; y < values.size(); y++)
	{
		Inlier.push_back(values[y][0]);
	}

	for (unsigned int y = 0; y < values.size(); y++)
	{
		values[y].clear();
	}
	values.clear();
}

void CalibrationImage::loadPointsDetected(QString filename)
{
	loadPoints(detectedPoints, filename);
}

void CalibrationImage::loadPointsDetectedAll(QString filename)
{
	loadPoints(detectedPoints_ALL, filename);
}

void CalibrationImage::loadRotationMatrix(QString filename)
{
	std::vector<std::vector<double> > values;
	std::ifstream fin(filename.toAscii().data());
	std::istringstream in;
	std::string line;
	while (!littleHelper::safeGetline(fin, line).eof())
	{
		in.clear();
		in.str(line);
		std::vector<double> tmp;
		for (double value; in >> value; littleHelper::comma(in))
		{
			tmp.push_back(value);
		}
		if (tmp.size() > 0) values.push_back(tmp);
	}
	fin.close();

	cv::Mat rotationmatrix;
	rotationmatrix.create(3, 3,CV_64F);
	for (unsigned int y = 0; y < 3; y++)
	{
		rotationmatrix.at<double>(y, 0) = values[y][0];
		rotationmatrix.at<double>(y, 1) = values[y][1];
		rotationmatrix.at<double>(y, 2) = values[y][2];
	}
	cv::Rodrigues(rotationmatrix, rotationvector);
	rotationmatrix.release();

	for (unsigned int y = 0; y < values.size(); y++)
	{
		values[y].clear();
	}
	values.clear();
}

void CalibrationImage::loadTranslationVector(QString filename)
{
	std::vector<std::vector<double> > values;
	std::ifstream fin(filename.toAscii().data());
	std::istringstream in;
	std::string line;
	while (!littleHelper::safeGetline(fin, line).eof())
	{
		in.clear();
		in.str(line);
		std::vector<double> tmp;
		for (double value; in >> value; littleHelper::comma(in))
		{
			tmp.push_back(value);
		}
		if (tmp.size() > 0) values.push_back(tmp);
	}
	fin.close();

	for (unsigned int y = 0; y < 3; y++)
	{
		translationvector.at<double>(y, 0) = values[y][0];
	}

	for (unsigned int y = 0; y < values.size(); y++)
	{
		values[y].clear();
	}
	values.clear();
}

void CalibrationImage::sortGridByReference(double x, double y)
{
	Inlier.clear();
	detectedPoints.clear();
	detectedPointsUndistorted.clear();
	projectedPoints.clear();
	projectedPointsUndistorted.clear();

	int idx = 0;
	double dist = 10000000;
	for (unsigned int i = 0; i < detectedPoints_ALL.size(); i += detectedPoints_ALL.size() - 1)
	{
		double tmp = (x - detectedPoints_ALL[i].x) * (x - detectedPoints_ALL[i].x) + (y - detectedPoints_ALL[i].y) * (y - detectedPoints_ALL[i].y);
		if (tmp < dist)
		{
			dist = tmp;
			idx = i;
		}
	}

	if (idx == 0)
	{
		//correct do nothing
		for (unsigned int i = 0; i < detectedPoints_ALL.size(); i++)
		{
			detectedPoints.push_back(cv::Point2d(detectedPoints_ALL[i].x, detectedPoints_ALL[i].y));
			detectedPointsUndistorted.push_back(cv::Point2d(detectedPoints_ALL[i].x, detectedPoints_ALL[i].y));
			projectedPoints.push_back(cv::Point2d(detectedPoints_ALL[i].x, detectedPoints_ALL[i].y));
			projectedPointsUndistorted.push_back(cv::Point2d(detectedPoints_ALL[i].x, detectedPoints_ALL[i].y));

			Inlier.push_back(1);
		}
	}
	else if (idx = detectedPoints_ALL.size() - 1)
	{
		//invert
		for (int i = detectedPoints_ALL.size() - 1; i >= 0; i--)
		{
			detectedPoints.push_back(cv::Point2d(detectedPoints_ALL[i].x, detectedPoints_ALL[i].y));
			detectedPointsUndistorted.push_back(cv::Point2d(detectedPoints_ALL[i].x, detectedPoints_ALL[i].y));
			projectedPoints.push_back(cv::Point2d(detectedPoints_ALL[i].x, detectedPoints_ALL[i].y));
			projectedPointsUndistorted.push_back(cv::Point2d(detectedPoints_ALL[i].x, detectedPoints_ALL[i].y));

			Inlier.push_back(1);
		}
	}
}

