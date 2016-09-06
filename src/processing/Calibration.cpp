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
///\file Calibration.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "processing/Calibration.h" 

#include "ui/ProgressDialog.h"
#include "ui/MainWindow.h"

#include "core/Project.h"
#include "core/Camera.h"
#include "core/CalibrationImage.h"
#include "core/CalibrationObject.h"
#include "core/Settings.h"

#include "processing/MultiCameraCalibration.h"

#include <QtCore>
#include <math.h>

using namespace xma;

Calibration::Calibration(int camera, bool planar) : ThreadedProcessing("Calibrate camera internal parameters and pose for all frames")
{
	m_camera = camera;
	m_planar = planar;
	intrinsic_matrix.create(3, 3, CV_64F);
	distortion_coeffs.create(8, 1, CV_64F);

	if (Project::getInstance()->getCameras()[m_camera]->hasModelDistortion())
	{
		for (int i = 0; i < 8; ++i)
		{
			distortion_coeffs.at<double>(i, 0) = Project::getInstance()->getCameras()[m_camera]->getDistortionCoefficiants().at<double>(i, 0);
		}
	}
	else
	{
		for (int i = 0; i < 8; ++i)
		{
			distortion_coeffs.at<double>(i, 0) = 0;
		}
	}


	for (int f = 0; f < Project::getInstance()->getNbImagesCalibration(); f ++)
	{
		if ((!m_planar && Project::getInstance()->getCameras()[m_camera]->getCalibrationImages()[f]->isCalibrated() > 0)
			|| (m_planar && Project::getInstance()->getCameras()[m_camera]->getCalibrationImages()[f]->getDetectedPoints().size() > 0))
		{
			std::vector<cv::Point3f> pt3D_tmp;
			std::vector<cv::Point2f> pt2D_tmp;

			for (unsigned int k = 0; k < CalibrationObject::getInstance()->getFrameSpecifications().size(); k++)
			{
				if (Project::getInstance()->getCameras()[m_camera]->getCalibrationImages()[f]->getInliers()[k] > 0)
				{
					pt3D_tmp.push_back(cv::Point3f(CalibrationObject::getInstance()->getFrameSpecifications()[k].x, CalibrationObject::getInstance()->getFrameSpecifications()[k].y,
					                               CalibrationObject::getInstance()->getFrameSpecifications()[k].z));

					cv::Point2d tmp = Project::getInstance()->getCameras()[m_camera]->undistortPoint(Project::getInstance()->getCameras()[m_camera]->getCalibrationImages()[f]->getDetectedPoints()[k], true, false);
					pt2D_tmp.push_back(cv::Point2f(tmp.x, tmp.y));
				}
			}

			object_points.push_back(pt3D_tmp);
			image_points.push_back(pt2D_tmp);
		}
	}

	if (Project::getInstance()->getCameras()[m_camera]->isCalibrated())
	{
		for (int i = 0; i < 3; ++i)
		{
			for (int j = 0; j < 3; ++j)
			{
				intrinsic_matrix.at<double>(i, j) = Project::getInstance()->getCameras()[m_camera]->getCameraMatrix().at<double>(i, j);
			}
		}
		intrinsic_matrix.at<double>(0, 1) = 0;
	}
	else
	{
		setInitialByReferences();
	}
}

Calibration::~Calibration()
{
	intrinsic_matrix.release();
	distortion_coeffs.release();

	for (unsigned int i = 0; i < object_points.size(); i++) object_points[i].clear();
	object_points.clear();

	for (unsigned int i = 0; i < image_points.size(); i++) image_points[i].clear();
	image_points.clear();
}

void Calibration::process_finished()
{
	Project::getInstance()->getCameras()[m_camera]->setRecalibrationRequired(0);
	Project::getInstance()->getCameras()[m_camera]->setUpdateInfoRequired(true);
}

void Calibration::setInitialByReferences()
{
	cv::Mat projection;
	projection.create(3, 4, CV_64F);

	cv::Mat f;
	f.create(12, 1, CV_64F);

	cv::Mat A;
	A.create(image_points[0].size() * 2, 12, CV_64F);

	//Fill A-Matrix
	for (unsigned int i = 0; i < image_points[0].size(); i++)
	{
		A.at<double>(2 * i, 0) = object_points[0][i].x;
		A.at<double>(2 * i, 1) = object_points[0][i].y;
		A.at<double>(2 * i, 2) = object_points[0][i].z;
		A.at<double>(2 * i, 3) = 1.0;
		A.at<double>(2 * i, 4) = 0;
		A.at<double>(2 * i, 5) = 0;
		A.at<double>(2 * i, 6) = 0;
		A.at<double>(2 * i, 7) = 0;
		A.at<double>(2 * i, 8) = -image_points[0][i].x * object_points[0][i].x;
		A.at<double>(2 * i, 9) = -image_points[0][i].x * object_points[0][i].y;
		A.at<double>(2 * i, 10) = -image_points[0][i].x * object_points[0][i].z;
		A.at<double>(2 * i, 11) = -image_points[0][i].x;

		A.at<double>(2 * i + 1, 0) = 0;
		A.at<double>(2 * i + 1, 1) = 0;
		A.at<double>(2 * i + 1, 2) = 0;
		A.at<double>(2 * i + 1, 3) = 0;
		A.at<double>(2 * i + 1, 4) = object_points[0][i].x;
		A.at<double>(2 * i + 1, 5) = object_points[0][i].y;
		A.at<double>(2 * i + 1, 6) = object_points[0][i].z;
		A.at<double>(2 * i + 1, 7) = 1.0;
		A.at<double>(2 * i + 1, 8) = -image_points[0][i].y * object_points[0][i].x;
		A.at<double>(2 * i + 1, 9) = -image_points[0][i].y * object_points[0][i].y;
		A.at<double>(2 * i + 1, 10) = -image_points[0][i].y * object_points[0][i].z;
		A.at<double>(2 * i + 1, 11) = -image_points[0][i].y;
	}
	cv::SVD::solveZ(A, f);

	//set projection matrix
	projection.at<double>(0, 0) = f.at<double>(0, 0);
	projection.at<double>(0, 1) = f.at<double>(1, 0);
	projection.at<double>(0, 2) = f.at<double>(2, 0);
	projection.at<double>(0, 3) = f.at<double>(3, 0);

	projection.at<double>(1, 0) = f.at<double>(4, 0);
	projection.at<double>(1, 1) = f.at<double>(5, 0);
	projection.at<double>(1, 2) = f.at<double>(6, 0);
	projection.at<double>(1, 3) = f.at<double>(7, 0);

	projection.at<double>(2, 0) = f.at<double>(8, 0);
	projection.at<double>(2, 1) = f.at<double>(9, 0);
	projection.at<double>(2, 2) = f.at<double>(10, 0);
	projection.at<double>(2, 3) = f.at<double>(11, 0);

	cv::Mat B;
	B.create(3, 3, CV_64F);
	cv::Mat BT;
	BT.create(3, 3, CV_64F);
	B.at<double>(0, 0) = projection.at<double>(0, 0);
	B.at<double>(0, 1) = projection.at<double>(0, 1);
	B.at<double>(0, 2) = projection.at<double>(0, 2);
	B.at<double>(1, 0) = projection.at<double>(1, 0);
	B.at<double>(1, 1) = projection.at<double>(1, 1);
	B.at<double>(1, 2) = projection.at<double>(1, 2);
	B.at<double>(2, 0) = projection.at<double>(2, 0);
	B.at<double>(2, 1) = projection.at<double>(2, 1);
	B.at<double>(2, 2) = projection.at<double>(2, 2);
	transpose(B, BT);

	cv::Mat C = B * BT;

	double u0 = C.at<double>(0, 2) / C.at<double>(2, 2);
	double v0 = C.at<double>(1, 2) / C.at<double>(2, 2);
	double ku = C.at<double>(0, 0) / C.at<double>(2, 2);
	double kc = C.at<double>(0, 1) / C.at<double>(2, 2);
	double kv = C.at<double>(1, 1) / C.at<double>(2, 2);
	double b = ((kv - v0 * v0) > 0) ? sqrt(kv - v0 * v0) : -999;
	double g = (kc - u0 * v0) / b;
	double a = (ku - u0 * u0 - g * g > 0) ? sqrt(ku - u0 * u0 - g * g) : -999;

	intrinsic_matrix.at<double>(0, 0) = a;
	intrinsic_matrix.at<double>(0, 1) = g;
	intrinsic_matrix.at<double>(0, 2) = u0;
	intrinsic_matrix.at<double>(1, 0) = 0;
	intrinsic_matrix.at<double>(1, 1) = b;
	intrinsic_matrix.at<double>(1, 2) = v0;
	intrinsic_matrix.at<double>(2, 0) = 0;
	intrinsic_matrix.at<double>(2, 1) = 0;
	intrinsic_matrix.at<double>(2, 2) = C.at<double>(2, 2) / C.at<double>(2, 2);

	cv::Mat rot;
	cv::Mat trans;

	intrinsic_matrix.at<double>(0, 1) = 0;

	projection.release();
}

void Calibration::process()
{
	try
	{
		if (intrinsic_matrix.at<double>(0, 2) < 0) intrinsic_matrix.at<double>(0, 2) = 1;
		if (intrinsic_matrix.at<double>(0, 2) > Project::getInstance()->getCameras()[m_camera]->getWidth()) intrinsic_matrix.at<double>(0, 2) = Project::getInstance()->getCameras()[m_camera]->getWidth() - 1;
		if (intrinsic_matrix.at<double>(1, 2) < 0) intrinsic_matrix.at<double>(1, 2) = 0;
		if (intrinsic_matrix.at<double>(1, 2) > Project::getInstance()->getCameras()[m_camera]->getHeight()) intrinsic_matrix.at<double>(1, 2) = Project::getInstance()->getCameras()[m_camera]->getHeight() - 1;


		int flags = 0;

		if (!m_planar)
		{
			flags = CV_CALIB_USE_INTRINSIC_GUESS;
		}

		if (!Project::getInstance()->getCameras()[m_camera]->hasModelDistortion())
		{
			flags = flags + (CV_CALIB_FIX_K1 + CV_CALIB_FIX_K2 + CV_CALIB_FIX_K3 + CV_CALIB_ZERO_TANGENT_DIST);
		}

		cv::vector<cv::Mat> rvecs;
		cv::vector<cv::Mat> tvecs;

		cv::calibrateCamera(object_points, image_points, cv::Size(Project::getInstance()->getCameras()[m_camera]->getWidth(), Project::getInstance()->getCameras()[m_camera]->getHeight()), intrinsic_matrix, distortion_coeffs, rvecs, tvecs, flags);

		Project::getInstance()->getCameras()[m_camera]->setCameraMatrix(intrinsic_matrix);
		Project::getInstance()->getCameras()[m_camera]->setOptimized(false);

		if (Project::getInstance()->getCameras()[m_camera]->hasModelDistortion())
		{
			Project::getInstance()->getCameras()[m_camera]->setDistortionCoefficiants(distortion_coeffs);
		}
		else
		{
			Project::getInstance()->getCameras()[m_camera]->resetDistortion();
		}

		cv::Mat rotationvector;
		cv::Mat translationvector;
		rotationvector.create(3, 1,CV_64F);
		translationvector.create(3, 1,CV_64F);
		int count = 0;
		for (int f = 0; f < Project::getInstance()->getNbImagesCalibration(); f ++)
		{
			if ((!m_planar && Project::getInstance()->getCameras()[m_camera]->getCalibrationImages()[f]->isCalibrated() > 0)
				|| (m_planar && Project::getInstance()->getCameras()[m_camera]->getCalibrationImages()[f]->getDetectedPoints().size() > 0))
			{
				for (int i = 0; i < 3; i++)
					rotationvector.at<double>(i, 0) = rvecs[count].at<double>(i, 0);

				for (unsigned int i = 0; i < 3; i++)
					translationvector.at<double>(i, 0) = tvecs[count].at<double>(i, 0);

				count++;
				Project::getInstance()->getCameras()[m_camera]->getCalibrationImages()[f]->setMatrices(rotationvector, translationvector);
			}
		}
		rotationvector.release();
		translationvector.release();

		MultiCameraCalibration::reproject(m_camera);
	}
	catch (std::exception e)
	{
		fprintf(stderr, "Camera::calibrateCamera : cv::calibrateCamera Failed with Exception - %s\n", e.what());
	}
}

