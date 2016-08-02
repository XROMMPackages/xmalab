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
///\file Camera.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "core/Image.h"
#include "core/Camera.h"
#include "core/Project.h"
#include "core/CalibrationImage.h"
#include "core/UndistortionObject.h"
#include "core/HelperFunctions.h"
#include "core/CalibrationObject.h"

#include <QApplication>

#include <fstream>

#ifdef WIN32
#define OS_SEP "\\"
#else
	#define OS_SEP "/"
#endif

#ifndef _PI
#define _PI 3.141592653
#endif
#include "CalibrationObject.h"

using namespace xma;

Camera::Camera(QString cameraName, int _id)
{
	calibrationImages.clear();
	undistortionObject = NULL;

	name = cameraName;
	id = _id;
	height = 0;
	width = 0;
	lightCamera = false;

	calibrated = false;
	cameramatrix.create(3, 3, CV_64F);
	distortion_coeffs = cv::Mat::zeros(8, 1, CV_64F);
	model_distortion = false;
	requiresRecalibration = 0;
	updateInfoRequired = false;
	optimized = false;
}

Camera::~Camera()
{
	for (std::vector<CalibrationImage*>::iterator it = calibrationImages.begin(); it != calibrationImages.end(); ++it)
		delete *it;
	calibrationImages.clear();
	if (undistortionObject)
		delete undistortionObject;

	cameramatrix.release();
}

void Camera::reset()
{
	cameramatrix.release();
	cameramatrix.create(3, 3, CV_64F);
	setCalibrated(false);
	setRecalibrationRequired(0);
	setUpdateInfoRequired(true);
	for (std::vector<CalibrationImage*>::iterator it = calibrationImages.begin(); it != calibrationImages.end(); ++it)
	{
		(*it)->reset();
		(*it)->init(CalibrationObject::getInstance()->getFrameSpecifications().size());
	}
	resetDistortion();
}

void Camera::deleteFrame(int id)
{
	delete calibrationImages[id];
	calibrationImages.erase(calibrationImages.begin() + id);
}

void Camera::getGLTransformations(int referenceCalibration, double* projection, double* modelviewMatrix)
{
	double focal[2];
	double center[2];
	double image_size[2];
	double near_plane = 1.05;
	double far_plane = 1000.0;

	focal[0] = cameramatrix.at<double>(0, 0);
	focal[1] = cameramatrix.at<double>(1, 1);

	image_size[0] = getWidth();
	image_size[1] = getHeight();

	center[0] = cameramatrix.at<double>(0, 2) + 0.5; //for opengl center is in the middle of the pixel
	center[1] = cameramatrix.at<double>(1, 2) + 0.5; //for opengl center is in the middle of the pixel

	projection[0] = 2.0 * focal[0] / image_size[0];
	projection[4] = 0;
	projection[8] = 2.0 * center[0] / image_size[0] - 1.0;
	projection[12] = 0;

	projection[1] = 0;
	projection[5] = 2.0 * focal[1] / image_size[1];
	projection[9] = 2.0 * center[1] / image_size[1] - 1.0;
	projection[13] = 0;

	projection[2] = 0;
	projection[6] = 0;
	projection[10] = (far_plane + near_plane) / (far_plane - near_plane);
	projection[14] = -2.0 * far_plane * near_plane / (far_plane - near_plane);

	projection[3] = 0;
	projection[7] = 0;
	projection[11] = 1;
	projection[15] = 0;

	cv::Mat rotationmatrix;
	rotationmatrix.create(3, 3, CV_64F);
	cv::Rodrigues(getCalibrationImages()[referenceCalibration]->getRotationVector(), rotationmatrix);

	for (int y = 0; y <3; y++)
	{
		modelviewMatrix[y] = rotationmatrix.at<double>(y, 0);
		modelviewMatrix[y + 4] = rotationmatrix.at<double>(y, 1);
		modelviewMatrix[y + 8] = rotationmatrix.at<double>(y, 2);
		modelviewMatrix[y + 12] = getCalibrationImages()[referenceCalibration]->getTranslationVector().at<double>(y, 0);
	}

	modelviewMatrix[3] = 0;
	modelviewMatrix[7] = 0;
	modelviewMatrix[11] = 0;
	modelviewMatrix[15] = 1.0;
}

cv::Point2d Camera::projectPoint(cv::Point3d pt3d, int referenceCalibration)
{
	cv::Point2d pt2d;
	cv::Mat projMatrs = getProjectionMatrix(referenceCalibration);
	cv::Mat pt;
	pt.create(4, 1, CV_64F);
	pt.at<double>(0, 0) = pt3d.x;
	pt.at<double>(1, 0) = pt3d.y;
	pt.at<double>(2, 0) = pt3d.z;
	pt.at<double>(3, 0) = 1;
	cv::Mat pt_out = projMatrs * pt;
	double z = pt_out.at<double>(2, 0);
	if (z != 0.0)
	{
		cv::Point2d pt_trans;
		pt_trans.x = pt_out.at<double>(0, 0) / z;
		pt_trans.y = pt_out.at<double>(1, 0) / z;

		pt2d = undistortPoint(pt_trans, false);
	}
	return pt2d;
}

void Camera::loadImages(QStringList fileNames)
{
	for (QStringList::const_iterator constIterator = fileNames.constBegin(); constIterator != fileNames.constEnd(); ++constIterator)
		calibrationImages.push_back(new CalibrationImage(this, (*constIterator)));
}

CalibrationImage* Camera::addImage(QString fileName)
{
	CalibrationImage* image = new CalibrationImage(this, fileName);
	calibrationImages.push_back(image);
	return image;
}

void Camera::loadUndistortionImage(QString undistortionImage)
{
	undistortionObject = new UndistortionObject(this, undistortionImage);
}

bool Camera::setResolutions()
{
	width = calibrationImages[0]->getWidth();
	height = calibrationImages[0]->getHeight();

	if (hasModelDistortion())
	{
		cv::initUndistortRectifyMap(cameramatrix, distortion_coeffs, cv::Mat(), cameramatrix, cv::Size(width, height), CV_32FC1, undistortionMapX, undistortionMapY);
	}

	for (std::vector<CalibrationImage*>::iterator it = calibrationImages.begin(); it != calibrationImages.end(); ++it)
	{
		if (width != (*it)->getWidth() || height != (*it)->getHeight()) return false;
	}

	if (undistortionObject && (width != undistortionObject->getWidth() || height != undistortionObject->getHeight())) return false;


	return true;
}

void Camera::save(QString folder)
{
	if (isCalibrated())
	{
		saveCameraMatrix(folder + "data" + OS_SEP + getFilenameCameraMatrix());
		if (hasModelDistortion())saveUndistortionParam(folder + "data" + OS_SEP + getFilenameUndistortionParam());
	}

	if (undistortionObject)
	{
		undistortionObject->getImage()->save(folder + undistortionObject->getFilename());
		if (undistortionObject->isComputed())
		{
			undistortionObject->savePointsDetected(folder + "data" + OS_SEP + undistortionObject->getFilenamePointsDetected());
			undistortionObject->saveGridPointsDistorted(folder + "data" + OS_SEP + undistortionObject->getFilenameGridPointsDistorted());
			undistortionObject->saveGridPointsReferences(folder + "data" + OS_SEP + undistortionObject->getFilenameGridPointsReferences());
			undistortionObject->saveGridPointsInlier(folder + "data" + OS_SEP + undistortionObject->getFilenameGridPointsInlier());
		}
	}

	for (std::vector<CalibrationImage*>::iterator it = calibrationImages.begin(); it != calibrationImages.end(); ++it)
	{
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

void Camera::loadTextures()
{
	if (undistortionObject)
		undistortionObject->loadTextures();

	QApplication::processEvents();

	for (std::vector<CalibrationImage*>::iterator it = calibrationImages.begin(); it != calibrationImages.end(); ++it)
	{
		(*it)->loadTextures();
		QApplication::processEvents();
	}
}

void Camera::undistort()
{
	if (undistortionObject && undistortionObject->isComputed())
	{
		undistortionObject->undistort(undistortionObject->getImage(), undistortionObject->getUndistortedImage());

		for (std::vector<CalibrationImage*>::iterator it = calibrationImages.begin(); it != calibrationImages.end(); ++it)
		{
			undistortionObject->undistort((*it)->getImage(), (*it)->getUndistortedImage());
			if (hasModelDistortion())
			{
				cv::Mat imageMat;
				(*it)->getUndistortedImage()->getImage(imageMat);
				cv::remap(imageMat, imageMat, undistortionMapX, undistortionMapY, cv::INTER_LANCZOS4, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
				(*it)->getUndistortedImage()->setImage(imageMat);
				imageMat.release();
			}
			if (isCalibrated())setRecalibrationRequired(1);
		}
	}
	else if (hasModelDistortion())
	{
		for (std::vector<CalibrationImage*>::iterator it = calibrationImages.begin(); it != calibrationImages.end(); ++it)
		{
			cv::Mat imageMat;
			(*it)->getImage()->getImage(imageMat);
			cv::remap(imageMat, imageMat, undistortionMapX, undistortionMapY, cv::INTER_LANCZOS4, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
			(*it)->getUndistortedImage()->setImage(imageMat);
			imageMat.release();
		}
	}

	for (std::vector<CalibrationImage*>::iterator it = calibrationImages.begin(); it != calibrationImages.end(); ++it)
	{
		(*it)->undistortPoints();
	}
}

void Camera::setCalibrated(bool value)
{
	calibrated = value;
	Project::getInstance()->checkCalibration();
}

void Camera::setCameraMatrix(cv::Mat& _cameramatrix)
{
	cameramatrix = _cameramatrix.clone();
	setCalibrated(true);
}

cv::Mat Camera::getCameraMatrix()
{
	return cameramatrix.clone();
}

void Camera::setDistortionCoefficiants(cv::Mat& _distortion_coeff)
{
	distortion_coeffs = _distortion_coeff.clone();
	cv::initUndistortRectifyMap(cameramatrix, distortion_coeffs, cv::Mat(), cameramatrix, cv::Size(width, height), CV_32FC1, undistortionMapX, undistortionMapY);
	model_distortion = true;
	undistort();
}

void Camera::resetDistortion()
{
	if (model_distortion)
	{
		distortion_coeffs = cv::Mat::zeros(8, 1, CV_64F);
		model_distortion = false;
		undistort();
	}
}

cv::Mat Camera::getDistortionCoefficiants()
{
	return distortion_coeffs;
}

bool Camera::hasModelDistortion()
{
	return model_distortion;
}

cv::Mat Camera::getProjectionMatrix(int referenceFrame)
{
	cv::Mat projectionmatrix;
	projectionmatrix.create(3, 4, CV_64F);

	cv::Mat tmp;
	cv::Mat rotationmatrix;
	rotationmatrix.create(3, 3, CV_64F);
	cv::Rodrigues(getCalibrationImages()[referenceFrame]->getRotationVector(), rotationmatrix);
	rotationmatrix.copyTo(tmp);
	cv::hconcat(tmp, getCalibrationImages()[referenceFrame]->getTranslationVector(), tmp);
	projectionmatrix = cameramatrix * tmp;

	rotationmatrix.release();
	tmp.release();
	return projectionmatrix;
}

QString Camera::getFilenameCameraMatrix()
{
	return this->getName() + "_CameraMatrix.csv";
}


void Camera::saveCameraMatrix(QString filename)
{
	std::ofstream outfile(filename.toAscii().data());
	outfile.precision(12);
	for (unsigned int i = 0; i < 3; ++i)
	{
		outfile << cameramatrix.at<double>(i, 0) << ", " << cameramatrix.at<double>(i, 1) << ", " << cameramatrix.at<double>(i, 2) << std::endl;
	}

	outfile.close();
}

void Camera::loadCameraMatrix(QString filename)
{
	std::vector<std::vector<double>> values;
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
		cameramatrix.at<double>(y, 0) = values[y][0];
		cameramatrix.at<double>(y, 1) = values[y][1];
		cameramatrix.at<double>(y, 2) = values[y][2];
	}

	for (unsigned int y = 0; y < values.size(); y++)
	{
		values[y].clear();
	}
	values.clear();
}

QString Camera::getFilenameUndistortionParam()
{
	return this->getName() + "_UndistortionParameter.csv";
}

void Camera::saveUndistortionParam(QString filename)
{
	std::ofstream outfile(filename.toAscii().data());
	outfile.precision(12);
	for (unsigned int i = 0; i < 8; ++i)
	{
		outfile << distortion_coeffs.at<double>(i, 0) << std::endl;
	}

	outfile.close();
}

void Camera::loadUndistortionParam(QString filename)
{
	std::vector<std::vector<double>> values;
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

	for (unsigned int y = 0; y < 8; y++)
	{
		distortion_coeffs.at<double>(y, 0) = values[y][0];
	}

	for (unsigned int y = 0; y < values.size(); y++)
	{
		values[y].clear();
	}
	values.clear();

	model_distortion = true;
}

void Camera::saveMayaCamVersion2(int ImageId, QString filename)
{
	std::ofstream outfile(filename.toAscii().data());
	outfile.precision(12);
	outfile << "image size" << std::endl;
	outfile << getWidth() << "," << getHeight() << std::endl;
	outfile << std::endl;

	outfile << "camera matrix" << std::endl;
	for (unsigned int i = 0; i < 3; ++i)
	{
		outfile << cameramatrix.at<double>(i, 0) << "," << cameramatrix.at<double>(i, 1) << "," << cameramatrix.at<double>(i, 2) << std::endl;
	}
	outfile << std::endl;

	outfile << "rotation" << std::endl;
	cv::Mat rotationmatrix;
	rotationmatrix.create(3, 3, CV_64F);
	cv::Rodrigues(getCalibrationImages()[ImageId]->getRotationVector(), rotationmatrix);
	for (unsigned int i = 0; i < 3; ++i)
	{
		outfile << rotationmatrix.at<double>(i, 0) << "," << rotationmatrix.at<double>(i, 1) << "," << rotationmatrix.at<double>(i, 2) << std::endl;
	}
	rotationmatrix.release();
	outfile << std::endl;

	outfile << "translation" << std::endl;
	cv::Mat translationVector = getCalibrationImages()[ImageId]->getTranslationVector();
	for (unsigned int i = 0; i < 3; ++i)
	{
		outfile << translationVector.at<double>(i, 0) << std::endl;
	}
	translationVector.release();
	outfile.close();
}

void Camera::getMayaCam(double* out, int frame)
{
	cv::Mat transTmp;
	cv::Mat rotTmp;
	cv::Mat camTmp;
	camTmp.create(3, 3, CV_64F);
	rotTmp.create(3, 3, CV_64F);
	transTmp.create(3, 1, CV_64F);

	camTmp = cameramatrix.clone();
	transTmp = getCalibrationImages()[frame]->getTranslationVector();
	cv::Rodrigues(getCalibrationImages()[frame]->getRotationVector(), rotTmp);

	//adjust y - inversion
	transTmp.at<double>(0, 0) = -transTmp.at<double>(0, 0);
	transTmp.at<double>(0, 2) = -transTmp.at<double>(0, 2);
	for (int i = 0; i < 3; i ++)
	{
		rotTmp.at<double>(0, i) = -rotTmp.at<double>(0, i);
		rotTmp.at<double>(2, i) = -rotTmp.at<double>(2, i);
	}
	camTmp.at<double>(1, 2) = (getHeight() - 1) - camTmp.at<double>(1, 2);

	//Invert Transformation
	rotTmp = rotTmp.inv();
	transTmp = -rotTmp * transTmp;

	//0-2 Translation
	out[0] = transTmp.at<double>(0, 0);
	out[1] = transTmp.at<double>(0, 1);
	out[2] = transTmp.at<double>(0, 2);

	//3-5 rotation angles in yaw pitch roll
	out[3] = 180.0 / _PI * atan2(-rotTmp.at<double>(2, 1), rotTmp.at<double>(2, 2));
	out[4] = 180.0 / _PI * atan2(rotTmp.at<double>(2, 0), sqrt(rotTmp.at<double>(2, 1) * rotTmp.at<double>(2, 1)
		                             + rotTmp.at<double>(2, 2) * rotTmp.at<double>(2, 2)));
	out[5] = 180.0 / _PI * atan2(-rotTmp.at<double>(1, 0), -rotTmp.at<double>(0, 0));

	//6-8 planeX,planeY,planeZ
	double _near = 0.1;
	double f = - 0.5 * (camTmp.at<double>(0, 0) + camTmp.at<double>(1, 1));

	//Have to check here
	out[6] = (0.5 * getWidth() - (camTmp.at<double>(0, 2) + 1.0)) * _near; //(-0.1 for x coordinates from 1)
	out[7] = (0.5 * getHeight() - (camTmp.at<double>(1, 2) + 1.0)) * _near; //(-0.1 for y coordinates from 1)
	out[8] = _near * f;

	//9-11 center_x, center_y, f
	out[9] = camTmp.at<double>(0, 2) + 1.0; //(+1 for x coordinates from 1)
	out[10] = camTmp.at<double>(1, 2) + 1.0; //(+1 for y coordinates from 1)
	out[11] = f;

	//scale, width, height
	out[12] = _near;
	out[13] = getWidth();
	out[14] = getHeight();
}

void Camera::getDLT(double* out, int frame)
{
	cv::Mat transTmp;
	cv::Mat rotTmp;
	cv::Mat camTmp;
	camTmp.create(3, 3, CV_64F);
	rotTmp.create(3, 3, CV_64F);
	transTmp.create(3, 1, CV_64F);

	camTmp = cameramatrix.clone();
	transTmp = getCalibrationImages()[frame]->getTranslationVector();
	cv::Rodrigues(getCalibrationImages()[frame]->getRotationVector(), rotTmp);

	//adjust y - inversion
	transTmp.at<double>(0, 0) = -transTmp.at<double>(0, 0);
	transTmp.at<double>(0, 2) = -transTmp.at<double>(0, 2);
	for (int i = 0; i < 3; i ++)
	{
		rotTmp.at<double>(0, i) = -rotTmp.at<double>(0, i);
		rotTmp.at<double>(2, i) = -rotTmp.at<double>(2, i);
	}
	camTmp.at<double>(1, 2) = (getHeight() - 1) - camTmp.at<double>(1, 2);

	cv::Mat tmp;
	cv::Mat projectionmatrix;
	projectionmatrix.create(3, 4,CV_64F);
	rotTmp.copyTo(tmp);
	cv::hconcat(tmp, transTmp, tmp);
	projectionmatrix = camTmp * tmp;
	int count = 0;
	for (unsigned int i = 0; i < 3; i++)
	{
		for (unsigned int j = 0; j < 4; j ++)
		{
			out[count] = projectionmatrix.at<double>(i, j);
			count++;
		}
	}
	projectionmatrix.release();

	//normalize
	for (unsigned int i = 0; i < 4; i++)
	{
		out[i] = (out[i] + out[i + 8]) / out[11]; // (+1 for x coordinates from 1)
		out[i + 4] = (out[i + 4] + out[i + 8]) / out[11]; // (+1 for x coordinates from 1)
		out[i + 8] = out[i + 8] / out[11];
	}
}

cv::Point2d Camera::undistortPoint(cv::Point2d pt, bool undistort, bool withModel, bool withRefine)
{
	cv::Point2d pt_out = pt;
	if (undistortionObject && undistortionObject->isComputed())
	{
		pt_out = undistortionObject->transformPoint(pt_out, undistort, withRefine);
	}
	if (hasModelDistortion() && withModel)
	{
		pt_out = applyModelDistortion(pt_out, undistort);
	}
	return pt_out;
}

cv::Point2d Camera::applyModelDistortion(cv::Point2d pt, bool undistort)
{
	cv::Point2d pt_out;

	if (undistort)
	{
		cv::Mat pt_inMat;
		pt_inMat.create(1, 1, CV_64FC2);
		cv::Mat pt_outMat;
		pt_outMat.create(1, 1, CV_64FC2);
		pt_inMat.at<cv::Vec2d>(0, 0)[0] = pt.x;
		pt_inMat.at<cv::Vec2d>(0, 0)[1] = pt.y;
		//std::cerr << pt_inMat << std::endl;
		cv::undistortPoints(pt_inMat, pt_outMat, cameramatrix, distortion_coeffs);
		//std::cerr << pt_outMat << std::endl;
		pt_out.x = cameramatrix.at<double>(0, 0) * pt_outMat.at<cv::Vec2d>(0, 0)[0] + cameramatrix.at<double>(0, 2);
		pt_out.y = cameramatrix.at<double>(1, 1) * pt_outMat.at<cv::Vec2d>(0, 0)[1] + cameramatrix.at<double>(1, 2);
	}
	else
	{
		pt_out.x = (pt.x - cameramatrix.at<double>(0, 2)) / cameramatrix.at<double>(0, 0);
		pt_out.y = (pt.y - cameramatrix.at<double>(1, 2)) / cameramatrix.at<double>(1, 1);

		double x2 = pt_out.x * pt_out.x;
		double y2 = pt_out.y * pt_out.y;
		double r2 = x2 + y2;
		double xy2 = 2 * pt_out.x * pt_out.y;

		double kr = (1 + ((distortion_coeffs.at<double>(4, 0) * r2 + distortion_coeffs.at<double>(1, 0)) * r2 + distortion_coeffs.at<double>(0, 0)) * r2);
		pt_out.x = cameramatrix.at<double>(0, 0) * (pt_out.x * kr + distortion_coeffs.at<double>(2, 0) * xy2 + distortion_coeffs.at<double>(3, 0) * (r2 + 2 * x2)) + cameramatrix.at<double>(0, 2);
		pt_out.y = cameramatrix.at<double>(1, 1) * (pt_out.y * kr + distortion_coeffs.at<double>(2, 0) * (r2 + 2 * y2) + distortion_coeffs.at<double>(3, 0) * xy2) + cameramatrix.at<double>(1, 2);
	}
	return pt_out;
}

