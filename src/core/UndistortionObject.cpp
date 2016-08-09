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
///\file UndistortionObject.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "core/UndistortionObject.h"
#include "core/Image.h"
#include "core/Camera.h"
#include "core/HelperFunctions.h"

#include <QFileInfo>
#include <fstream>

#include <opencv2/contrib/contrib.hpp>
#include <opencv2/core/core.hpp>
#include <opencv/highgui.h>

using namespace xma;

UndistortionObject::UndistortionObject(Camera* _camera, QString _imageFileName)
{
	imageFileName = _imageFileName;
	camera = _camera;
	image = new Image(imageFileName);
	undistortedImage = new Image(image);
	tmpImage = new Image(image);
	width = image->getWidth();
	height = image->getHeight();
	computed = false;
	tmpImageType = 0;
	centerSet = false;
	requiresRecalibration = 0;
	updateInfoRequired = false;
}

UndistortionObject::~UndistortionObject()
{
	delete image;
	delete undistortedImage;
	delete tmpImage;
}

void UndistortionObject::loadTextures()
{
	image->loadTexture();
	undistortedImage->loadTexture();
	tmpImage->loadTexture();
}

bool UndistortionObject::undistort(Image* distorted, Image* undistorted)
{
	if (computed)
	{
		cv::Mat imageMat;
		distorted->getImage(imageMat);
		cv::remap(imageMat, imageMat, undistortionMapX, undistortionMapY, cv::INTER_LANCZOS4, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
		undistorted->setImage(imageMat);
		imageMat.release();
	}
	return false;
}

bool UndistortionObject::undistort(Image* distorted, QString filenameOut)
{
	bool success = false;
	if (computed)
	{
		cv::Mat imageMat;
		distorted->getImage(imageMat);
		if (imageMat.size().width == undistortionMapX.size().width &&
			imageMat.size().height == undistortionMapX.size().height)
		{
			cv::remap(imageMat, imageMat, undistortionMapX, undistortionMapY, cv::INTER_LANCZOS4, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
			cv::imwrite(filenameOut.toAscii().data(), imageMat);
			success = true;
		}
		imageMat.release();
	}
	return success;
}

bool UndistortionObject::undistort(QString filenameIn, QString filenameOut)
{
	bool success = false;
	if (computed)
	{
		cv::Mat imageMat;
		imageMat = cv::imread(filenameIn.toAscii().data());
		if (imageMat.size().width == undistortionMapX.size().width &&
			imageMat.size().height == undistortionMapX.size().height)
		{
			cv::remap(imageMat, imageMat, undistortionMapX, undistortionMapY, cv::INTER_LANCZOS4, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
			cv::imwrite(filenameOut.toAscii().data(), imageMat);
			success = true;
		}
		imageMat.release();
	}
	return success;
}

void UndistortionObject::setDetectedPoints(cv::vector<cv::Point2d>& points)
{
	points_detected.clear();
	for (std::vector<cv::Point2d>::const_iterator it = points.begin(); it != points.end(); ++it)
	{
		points_detected.push_back(cv::Point2d((*it).x, (*it).y));
	}
}

void UndistortionObject::getDetectedPoints(cv::vector<cv::Point2d>& points)
{
	points.clear();
	for (std::vector<cv::Point2d>::const_iterator it = points_detected.begin(); it != points_detected.end(); ++it)
	{
		points.push_back(cv::Point2d((*it).x, (*it).y));
	}
}

void UndistortionObject::removeOutlier(double threshold_circle, double threshold_border)
{
	cv::Point2f center;
	float radius;
	cv::vector<cv::Point2f> points_float;

	cv::Mat image_val;
	image->getImage(image_val);
	for (int x = 0; x < image->getWidth(); x++)
	{
		for (int y = 0; y < image->getHeight(); y++)
		{
			if (image_val.at<uchar>(y, x) > 100) points_float.push_back(cv::Point2f(x, y));
		}
	}

	cv::minEnclosingCircle(points_float, center, radius);

	//remove circle
	for (unsigned int i = 0; i < points_grid_distorted.size(); i++)
	{
		double dist = pow(points_grid_distorted[i].x - center.x, 2) + pow(points_grid_distorted[i].y - center.y, 2);

		points_grid_inlier[i] = !(dist > pow(radius - 1.5 * threshold_circle, 2));
	}

	//remove border
	for (unsigned int i = 0; i < points_grid_distorted.size(); i++)
	{
		if (points_grid_distorted[i].x < threshold_border) points_grid_inlier[i] = false;
		if (points_grid_distorted[i].y < threshold_border) points_grid_inlier[i] = false;
		if (image->getWidth() - points_grid_distorted[i].x < threshold_border) points_grid_inlier[i] = false;
		if (image->getHeight() - points_grid_distorted[i].y < threshold_border) points_grid_inlier[i] = false;
	}
	image_val.release();

	setRecalibrationRequired(1);
}

void UndistortionObject::setGridPoints(cv::vector<cv::Point2d>& points_distorted, cv::vector<cv::Point2d>& points_references, cv::vector<bool>& points_inlier)
{
	points_grid_distorted.clear();
	for (std::vector<cv::Point2d>::const_iterator it = points_distorted.begin(); it != points_distorted.end(); ++it)
	{
		points_grid_distorted.push_back(cv::Point2d((*it).x, (*it).y));
	}
	points_grid_references.clear();
	for (std::vector<cv::Point2d>::const_iterator it = points_references.begin(); it != points_references.end(); ++it)
	{
		points_grid_references.push_back(cv::Point2d((*it).x, (*it).y));
	}
	points_grid_inlier.clear();
	for (std::vector<bool>::const_iterator it = points_inlier.begin(); it != points_inlier.end(); ++it)
	{
		points_grid_inlier.push_back((*it));
	}
}

void UndistortionObject::getGridPoints(cv::vector<cv::Point2d>& points_distorted, cv::vector<cv::Point2d>& points_references, cv::vector<bool>& points_inlier)
{
	points_distorted.clear();
	for (std::vector<cv::Point2d>::const_iterator it = points_grid_distorted.begin(); it != points_grid_distorted.end(); ++it)
	{
		points_distorted.push_back(cv::Point2d((*it).x, (*it).y));
	}
	points_references.clear();
	for (std::vector<cv::Point2d>::const_iterator it = points_grid_references.begin(); it != points_grid_references.end(); ++it)
	{
		points_references.push_back(cv::Point2d((*it).x, (*it).y));
	}
	points_inlier.clear();
	for (std::vector<bool>::const_iterator it = points_grid_inlier.begin(); it != points_grid_inlier.end(); ++it)
	{
		points_inlier.push_back((*it));
	}
}

void UndistortionObject::setMaps(cv::Mat& map_x, cv::Mat& map_y)
{
	undistortionMapX.release();
	undistortionMapX = map_x.clone();
	undistortionMapY.release();
	undistortionMapY = map_y.clone();
}

void UndistortionObject::setLWMMatrices(cv::Mat& A, cv::Mat& B, cv::Mat& radii, cv::Mat& points,
                                        cv::Mat& A_inverse, cv::Mat& B_inverse, cv::Mat& radii_inverse, cv::Mat& points_inverse)
{
	distortionMatrixA.release();
	distortionMatrixA = A.clone();
	distortionMatrixB.release();
	distortionMatrixB = B.clone();
	distortionMatrixRadii.release();
	distortionMatrixRadii = radii.clone();
	distortionPoints.release();
	distortionPoints = points.clone();

	undistortionMatrixA.release();
	undistortionMatrixA = A_inverse.clone();
	undistortionMatrixB.release();
	undistortionMatrixB = B_inverse.clone();
	undistortionMatrixRadii.release();
	undistortionMatrixRadii = radii_inverse.clone();
	undistortionPoints.release();
	undistortionPoints = points_inverse.clone();
}

int UndistortionObject::findClosestPoint(std::vector<cv::Point2d>& points, double x, double y, double maxDistSquare)
{
	int idx = -1;
	int count = 0;
	double xx, yy, distSquare;
	for (std::vector<cv::Point2d>::const_iterator it = points.begin(); it != points.end(); ++it)
	{
		xx = ((*it).x - x);
		yy = ((*it).y - y);
		distSquare = xx * xx + yy * yy;
		if (distSquare < maxDistSquare)
		{
			idx = count;
			maxDistSquare = distSquare;
		}
		count ++;
	}
	return idx;
}

void UndistortionObject::toggleOutlier(int vispoints, double x, double y)
{
	int idx = -1;

	switch (vispoints)
	{
	default:
		break;
	case 2:
		idx = findClosestPoint(points_grid_distorted, x, y);
		break;
	case 3:
		idx = findClosestPoint(points_grid_undistorted, x, y);
		break;
	case 4:
		idx = findClosestPoint(points_grid_references, x, y);
		break;
	}
	if (idx >= 0)
	{
		points_grid_inlier[idx] = ! points_grid_inlier[idx];
		requiresRecalibration = (requiresRecalibration > 1) ? requiresRecalibration : 1;
	}
}

void UndistortionObject::setCenter(double x, double y)
{
	center.x = x;
	center.y = y;
	centerSet = true;
	requiresRecalibration = 2;
}

void UndistortionObject::undistortPoints()
{
	points_grid_undistorted.clear();
	for (std::vector<cv::Point2d>::const_iterator it = points_grid_distorted.begin(); it != points_grid_distorted.end(); ++it)
	{
		points_grid_undistorted.push_back(transformPoint((*it), true));
	}
	computeError();
}

cv::Point2d UndistortionObject::transformLWM(cv::Point2d& ptIn, cv::Mat& controlPts, cv::Mat& A, cv::Mat& B, cv::Mat& radii)
{
	cv::Point2d pt_out;
	double xy = ptIn.x * ptIn.y;
	double x2 = ptIn.x * ptIn.x;
	double y2 = ptIn.y * ptIn.y;

	//compute the UV coordinates with all points
	double u_numerator = 0.0;
	double v_numerator = 0.0;
	double denominator = 0.0;

	double dx, dy, dist_to_cp, Ri, Ri2, Ri3, u, v, w;
	//for (i=0; i <controlPts_Mat.rows; i++) {
	for (int p = 0; p < controlPts.rows; p++)
	{
		dx = ptIn.x - controlPts.at<double>(p, 0);
		dy = ptIn.y - controlPts.at<double>(p, 1);
		dist_to_cp = sqrt(dx * dx + dy * dy);

		Ri = dist_to_cp / radii.at<double>(p);
		if (Ri < 1.0)
		{
			Ri2 = Ri * Ri;
			Ri3 = Ri * Ri2;
			w = 1.0 - 3.0 * Ri2 + 2.0 * Ri3; /* weight for ControlPoint i */

			//without ptrs
			u = A.at<double>(p, 0) + A.at<double>(p, 1) * ptIn.x + A.at<double>(p, 2) * ptIn.y +
				A.at<double>(p, 3) * xy + A.at<double>(p, 4) * x2 + A.at<double>(p, 5) * y2;

			v = B.at<double>(p, 0) + B.at<double>(p, 1) * ptIn.x + B.at<double>(p, 2) * ptIn.y +
				B.at<double>(p, 3) * xy + B.at<double>(p, 4) * x2 + B.at<double>(p, 5) * y2;

			u_numerator = u_numerator + w * u;
			v_numerator = v_numerator + w * v ;
			denominator = denominator + w ;
		}
	}

	if (denominator != 0.0)
	{
		//set uv and apply shift
		//qDebug("pt %i %i to %lf %lf",x_out,y_out,u_numerator/denominator + center_detectedPts.x,v_numerator/denominator + center_detectedPts.y );
		//without ptrs

		pt_out.x = u_numerator / denominator;
		pt_out.y = v_numerator / denominator;
	}
	else
	{
		/*
			* no control points influence this (x,y)
			* issue warning, and set warning flag to warn user that there
			* are one or more such points
			*/

		pt_out.x = -1;
		pt_out.y = -1;
	}

	return pt_out;
}

cv::Point2d UndistortionObject::transformPoint(cv::Point2d pt, bool undistort, bool withRefine)
{
	cv::Point2d pt_out;

	if (undistort)
	{
		pt_out = transformLWM(pt, undistortionPoints, undistortionMatrixA, undistortionMatrixB, undistortionMatrixRadii);
	}
	else
	{
		pt_out = transformLWM(pt, distortionPoints, distortionMatrixA, distortionMatrixB, distortionMatrixRadii);
	}

	if (pt_out.x == -1 && pt_out.y == -1)
	{
		pt_out.x = pt.x;
		pt_out.y = pt.y;
		return pt_out;
	}

	if (undistort && withRefine)
	{
		cv::Point2d pt_test;
		pt_test = transformLWM(pt_out, distortionPoints, distortionMatrixA, distortionMatrixB, distortionMatrixRadii);
		double dist = sqrt((pt.x - pt_test.x) * (pt.x - pt_test.x) + (pt.y - pt_test.y) * (pt.y - pt_test.y));
		double distTMP = dist;
		cv::Point2d pt_outTMP;
		double multiplier = 0.5;
		int k = 0;
		while (dist > 0.000001)
		{
			pt_outTMP.x = pt_out.x + multiplier * (pt.x - pt_test.x);
			pt_outTMP.y = pt_out.y + multiplier * (pt.y - pt_test.y);
			pt_test = transformLWM(pt_outTMP, distortionPoints, distortionMatrixA, distortionMatrixB, distortionMatrixRadii);
			distTMP = sqrt((pt.x - pt_test.x) * (pt.x - pt_test.x) + (pt.y - pt_test.y) * (pt.y - pt_test.y));
			if (distTMP < dist)
			{
				dist = distTMP;
				pt_out.x = pt_outTMP.x;
				pt_out.y = pt_outTMP.y;
			}
			else
			{
				multiplier = multiplier * 0.8;
			}
			if (pt_test.x == -1 || k == 100)
			{
				//fprintf(stderr,"Error %d %lf %lf\n",k,dist,multiplier);
				break;
			}
			k++;
		}
		//fprintf(stderr,"Error %d %lf\n",k,dist);
	}

	if (pt_out.x == -1 && pt_out.y == -1)
	{
		pt_out.x = pt.x;
		pt_out.y = pt.y;
	}

	return pt_out;
}

void UndistortionObject::drawPoints(std::vector<cv::Point2d>& points)
{
	std::vector<bool>::const_iterator it_inlier = points_grid_inlier.begin();
	glBegin(GL_LINES);
	for (std::vector<cv::Point2d>::const_iterator it = points.begin(); it != points.end(); ++it , ++it_inlier)
	{
		if ((*it_inlier))
		{
			glColor3f(0.0f, 0.8f, 0.0f);
		}
		else
		{
			glColor3f(0.8f, 0.0f, 0.0f);
		}
		glVertex2f((*it).x - 2, (*it).y);
		glVertex2f((*it).x + 2, (*it).y);
		glVertex2f((*it).x, (*it).y - 2);
		glVertex2f((*it).x, (*it).y + 2);
	}

	glEnd();
	if (points.size() > 0)
	{
		if ((points_grid_inlier[0]))
		{
			glColor3f(0.0f, 0.8f, 0.0f);
		}
		else
		{
			glColor3f(0.8f, 0.0f, 0.0f);
		}
		glBegin(GL_LINES);
		glVertex2f(points[0].x - 5, points[0].y);
		glVertex2f(points[0].x + 5, points[0].y);
		glVertex2f(points[0].x, points[0].y - 5);
		glVertex2f(points[0].x, points[0].y + 5);
		glEnd();
	}
}

void UndistortionObject::drawData(int type)
{
	switch (type)
	{
	case 0:
	default:
		break;
	case 1:
		glColor3f(1.0, 0.0, 0.0);
		glBegin(GL_LINES);
		for (std::vector<cv::Point2d>::const_iterator it = points_detected.begin(); it != points_detected.end(); ++it)
		{
			glVertex2f((*it).x - 2, (*it).y);
			glVertex2f((*it).x + 2, (*it).y);
			glVertex2f((*it).x, (*it).y - 2);
			glVertex2f((*it).x, (*it).y + 2);
		}
		glEnd();
		break;
	case 2:
		drawPoints(points_grid_distorted);
		break;
	case 3:
		drawPoints(points_grid_undistorted);
		break;
	case 4:
		drawPoints(points_grid_references);
		break;
	}

	if (isCenterSet())
	{
		glColor3f(0.0, 0.0, 1.0);
		glBegin(GL_LINES);
		glVertex2f(center.x - 5, center.y - 5);
		glVertex2f(center.x + 5, center.y + 5);
		glVertex2f(center.x + 5, center.y - 5);
		glVertex2f(center.x - 5, center.y + 5);
		glEnd();
	}
}

void UndistortionObject::computeTmpImage(int type)
{
	tmpImageType = type;

	cv::Mat imageMat;
	imageMat.create(tmpImage->getHeight(), tmpImage->getWidth(), CV_8UC(3));
	switch (tmpImageType)
	{
	case 0:
	case 1:
	default:
		break;
	case 2:
		getDisplacementScaledAngle(imageMat);
		break;
	case 3:
		getDisplacementAngle(imageMat);
		break;
	case 4:
		getDisplacementLength(imageMat);
		break;
	case 5:
		getDisplacementX(imageMat);
		break;
	case 6:
		getDisplacementY(imageMat);
		break;
	}

	tmpImage->setImage(imageMat, true);
	imageMat.release();
}

void UndistortionObject::getDisplacementLength(cv::Mat& imageOut)
{
	cv::Mat disp_length;
	disp_length.create(cv::Size(undistortionMapX.cols, undistortionMapX.rows), CV_32FC1);
	float* disp_length_ptr = (float*)disp_length.ptr(0);
	float* undistortionMapX_ptr = (float*)undistortionMapX.ptr(0);
	float* undistortionMapY_ptr = (float*)undistortionMapY.ptr(0);
	float x_disp;
	float y_disp;

	for (int y = 0; y < undistortionMapX.rows; y++)
	{
		for (int x = 0; x < undistortionMapX.cols; x++)
		{
			x_disp = *undistortionMapX_ptr++;
			y_disp = *undistortionMapY_ptr++;
			if ((y_disp != 0.0) && (x_disp != 0.0))
			{
				*disp_length_ptr++ = sqrt((y_disp - y) * (y_disp - y) + (x_disp - x) * (x_disp - x));
			}
			else
			{
				*disp_length_ptr++ = 0;
			}
		}
	}

	double min;
	double max;
	cv::minMaxIdx(disp_length, &min, &max);
	disp_length -= min;
	cv::Mat adjMap;
	cv::convertScaleAbs(disp_length, adjMap, 255.0 / double(max - min));

	applyColorMap(adjMap, imageOut, cv::COLORMAP_JET);

	disp_length.release();
	adjMap.release();
}

void UndistortionObject::getDisplacementAngle(cv::Mat& imageOut)
{
	cv::Mat disp_angle;
	disp_angle.create(cv::Size(undistortionMapX.cols, undistortionMapX.rows), CV_32FC1);
	float* disp_angle_ptr = (float*)disp_angle.ptr(0);
	float* undistortionMapX_ptr = (float*)undistortionMapX.ptr(0);
	float* undistortionMapY_ptr = (float*)undistortionMapY.ptr(0);
	float x_disp;
	float y_disp;

	for (int y = 0; y < undistortionMapX.rows; y++)
	{
		for (int x = 0; x < undistortionMapX.cols; x++)
		{
			x_disp = *undistortionMapX_ptr++;
			y_disp = *undistortionMapY_ptr++;
			if ((y_disp != 0.0) && (x_disp != 0.0))
			{
				*disp_angle_ptr++ = atan2(y_disp - y, x_disp - x);
			}
			else
			{
				*disp_angle_ptr++ = 0;
			}
		}
	}

	double min;
	double max;
	cv::minMaxIdx(disp_angle, &min, &max);
	disp_angle -= min;
	cv::Mat adjMap;
	cv::convertScaleAbs(disp_angle, adjMap, 255.0 / double(max - min));

	applyColorMap(adjMap, imageOut, cv::COLORMAP_HSV);

	disp_angle.release();
	adjMap.release();
}

void UndistortionObject::getDisplacementScaledAngle(cv::Mat& imageOut)
{
	cv::Mat disp_length;
	disp_length.create(cv::Size(undistortionMapX.cols, undistortionMapX.rows), CV_32FC1);
	float* disp_length_ptr = (float*)disp_length.ptr(0);

	cv::Mat disp_angle;
	disp_angle.create(cv::Size(undistortionMapX.cols, undistortionMapX.rows), CV_32FC1);
	float* disp_angle_ptr = (float*)disp_angle.ptr(0);
	float* undistortionMapX_ptr = (float*)undistortionMapX.ptr(0);
	float* undistortionMapY_ptr = (float*)undistortionMapY.ptr(0);
	float x_disp;
	float y_disp;

	for (int y = 0; y < undistortionMapX.rows; y++)
	{
		for (int x = 0; x < undistortionMapX.cols; x++)
		{
			x_disp = *undistortionMapX_ptr++;
			y_disp = *undistortionMapY_ptr++;
			if ((y_disp != 0.0) && (x_disp != 0.0))
			{
				*disp_angle_ptr++ = atan2(y_disp - y, x_disp - x);
				*disp_length_ptr++ = sqrt((y_disp - y) * (y_disp - y) + (x_disp - x) * (x_disp - x));
			}
			else
			{
				*disp_angle_ptr++ = 0;
				*disp_length_ptr++ = 0;
			}
		}
	}

	double min;
	double max;
	cv::minMaxIdx(disp_angle, &min, &max);
	disp_angle -= min;
	cv::Mat adjMap;
	cv::convertScaleAbs(disp_angle, adjMap, 255.0 / double(max - min));

	applyColorMap(adjMap, imageOut, cv::COLORMAP_HSV);

	cv::minMaxIdx(disp_length, &min, &max);
	disp_length -= min;
	cv::convertScaleAbs(disp_length, adjMap, 255.0 / double(max - min));

	cv::cvtColor(adjMap, adjMap, CV_GRAY2RGB);
	cv::multiply(adjMap, imageOut, imageOut, 1.0 / 255.0);

	disp_angle.release();
	adjMap.release();
	disp_length.release();
}

void UndistortionObject::getDisplacementX(cv::Mat& imageOut)
{
	cv::Mat diff_x;
	diff_x.create(cv::Size(undistortionMapX.cols, undistortionMapX.rows), CV_32FC1);
	float* diff_x_ptr = (float*)diff_x.ptr(0);
	float* undistortionMapX_ptr = (float*)undistortionMapX.ptr(0);
	float x_disp;
	for (int y = 0; y < undistortionMapX.rows; y++)
	{
		for (int x = 0; x < undistortionMapX.cols; x++)
		{
			x_disp = *undistortionMapX_ptr++;
			if (x_disp != 0.0)
			{
				*diff_x_ptr++ = x_disp - x;
			}
			else
			{
				*diff_x_ptr++ = 0;
			}
		}
	}

	double min;
	double max;
	cv::minMaxIdx(diff_x, &min, &max);
	diff_x -= min;
	cv::Mat adjMap;
	cv::convertScaleAbs(diff_x, adjMap, 255.0 / double(max - min));

	applyColorMap(adjMap, imageOut, cv::COLORMAP_JET);

	diff_x.release();
	adjMap.release();
}

void UndistortionObject::getDisplacementY(cv::Mat& imageOut)
{
	cv::Mat diff_y;
	diff_y.create(cv::Size(undistortionMapY.cols, undistortionMapY.rows), CV_32FC1);
	float* diff_y_ptr = (float*)diff_y.ptr(0);
	float* undistortionMapY_ptr = (float*)undistortionMapY.ptr(0);
	float y_disp;
	for (int y = 0; y < undistortionMapY.rows; y++)
	{
		for (int x = 0; x < undistortionMapY.cols; x++)
		{
			y_disp = *undistortionMapY_ptr++;
			if (y_disp != 0.0)
			{
				*diff_y_ptr++ = y_disp - y;
			}
			else
			{
				*diff_y_ptr++ = 0;
			}
		}
	}

	double min;
	double max;
	cv::minMaxIdx(diff_y, &min, &max);
	diff_y -= min;
	cv::Mat adjMap;
	cv::convertScaleAbs(diff_y, adjMap, 255.0 / double(max - min));

	applyColorMap(adjMap, imageOut, cv::COLORMAP_JET);

	diff_y.release();
	adjMap.release();
}

void UndistortionObject::bindTexture(int type)
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
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
		if (tmpImageType != type)
		{
			computeTmpImage(type);
		}
		tmpImage->bindTexture();
		break;
	}
}

QString UndistortionObject::getFilename()
{
	QFileInfo info(imageFileName);
	return info.fileName();
}

QString UndistortionObject::getFilenameBase()
{
	QFileInfo info(imageFileName);
	return info.completeBaseName();
}

QString UndistortionObject::getFilenamePointsDetected()
{
	QFileInfo info(imageFileName);
	return info.completeBaseName() + "_PointsDetected.csv";
}

QString UndistortionObject::getFilenameGridPointsDistorted()
{
	QFileInfo info(imageFileName);
	return info.completeBaseName() + "_GridPointsDistorted.csv";
}

QString UndistortionObject::getFilenameGridPointsReferences()
{
	QFileInfo info(imageFileName);
	return info.completeBaseName() + "_GridPointsReferences.csv";
}

QString UndistortionObject::getFilenameGridPointsInlier()
{
	QFileInfo info(imageFileName);
	return info.completeBaseName() + "_GridPointsInlier.csv";
}

void UndistortionObject::savePoints(std::vector<cv::Point2d>& points, QString filename)
{
	std::ofstream outfile(filename.toAscii().data());
	outfile.precision(12);
	for (std::vector<cv::Point2d>::const_iterator it = points.begin(); it != points.end(); ++it)
	{
		outfile << (*it).x << ", " << (*it).y << std::endl;
	}
	outfile.close();
}

void UndistortionObject::savePointsDetected(QString filename)
{
	savePoints(points_detected, filename);
}

void UndistortionObject::saveGridPointsDistorted(QString filename)
{
	savePoints(points_grid_distorted, filename);
}

void UndistortionObject::saveGridPointsReferences(QString filename)
{
	savePoints(points_grid_references, filename);
}

void UndistortionObject::saveGridPointsInlier(QString filename)
{
	std::ofstream outfile(filename.toAscii().data());
	outfile.precision(12);
	for (std::vector<bool>::const_iterator it = points_grid_inlier.begin(); it != points_grid_inlier.end(); ++it)
	{
		if ((*it))
		{
			outfile << 1 << std::endl;
		}
		else
		{
			outfile << 0 << std::endl;
		}
	}
	outfile.close();
}

void UndistortionObject::exportData(QString csvFileNameLUT, QString csvFileNameInPoints, QString csvFileNameBasePoints)
{
	cv::Formatter const* c_formatter(cv::Formatter::get("CSV"));
	std::ofstream outfile(csvFileNameLUT.toAscii().data());
	outfile.precision(12);
	c_formatter->write(outfile, undistortionMapX);
	c_formatter->write(outfile, undistortionMapY);
	outfile.close();

	std::ofstream outfile2(csvFileNameInPoints.toStdString().c_str());
	outfile2.precision(12);
	int count = 0;
	for (std::vector<bool>::const_iterator it = points_grid_inlier.begin(); it != points_grid_inlier.end(); ++it)
	{
		if ((*it))
		{
			outfile2 << points_grid_distorted[count].x << "," << points_grid_distorted[count].y << std::endl;
		}
		count ++;
	}
	outfile2.close();

	std::ofstream outfile3(csvFileNameBasePoints.toStdString().c_str());
	outfile3.precision(12);
	count = 0;
	for (std::vector<bool>::const_iterator it = points_grid_inlier.begin(); it != points_grid_inlier.end(); ++it)
	{
		if ((*it))
		{
			outfile3 << points_grid_references[count].x << "," << points_grid_references[count].y << std::endl;
		}
		count ++;
	}
	outfile3.close();
}

void UndistortionObject::loadPoints(cv::vector<cv::Point2d>& points, QString filename)
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

void UndistortionObject::loadPointsDetected(QString filename)
{
	loadPoints(points_detected, filename);
}

void UndistortionObject::loadGridPointsDistorted(QString filename)
{
	loadPoints(points_grid_distorted, filename);
}

void UndistortionObject::loadGridPointsReferences(QString filename)
{
	loadPoints(points_grid_references, filename);
}

void UndistortionObject::loadGridPointsInlier(QString filename)
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

	points_grid_inlier.clear();

	for (unsigned int y = 0; y < values.size(); y++)
	{
		if (values[y][0] > 0.5)
		{
			points_grid_inlier.push_back(true);
		}
		else
		{
			points_grid_inlier.push_back(false);
		}
	}

	for (unsigned int y = 0; y < values.size(); y++)
	{
		values[y].clear();
	}
	values.clear();
}

void UndistortionObject::computeError()
{
	error.clear();

	cv::Point2d diff;
	for (unsigned int i = 0; i < points_grid_references.size(); i++)
	{
		diff = points_grid_undistorted[i] - points_grid_references[i];
		error.push_back(cv::sqrt(diff.x * diff.x + diff.y * diff.y));
	}
}

