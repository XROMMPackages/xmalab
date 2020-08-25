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
///\file CubeCalibration.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "processing/CubeCalibration.h" 

#include "ui/ProgressDialog.h"
#include "ui/MainWindow.h"

#include "core/Project.h"
#include "core/Camera.h"
#include "core/CalibrationImage.h"
#include "core/CalibrationObject.h"
#include "core/UndistortionObject.h"
#include "core/Settings.h"

#include <QtCore>
#include <QtConcurrent/QtConcurrent>
#include <math.h>

using namespace xma;

int CubeCalibration::nbInstances = 0;

CubeCalibration::CubeCalibration(int camera, int image, cv::Point2d references [4], int referencesID [4]): QObject()
{
	m_camera = camera;
	m_image = image;
	nbInstances++;
	poseComputed = false;
	for (int i = 0; i < 4; i++)
	{
		selectedPoints[i].x = references[i].x;
		selectedPoints[i].y = references[i].y;
		selectedPointsID[i] = referencesID[i];
	}

	for (int i = 0; i < 4; i++)
	{
		selectedPoints[i] = Project::getInstance()->getCameras()[m_camera]->undistortPoint(selectedPoints[i], true);
	}

	coords3DSize = CalibrationObject::getInstance()->getFrameSpecifications().size();
	coords3D = new cv::Point3d[coords3DSize];
	for (int i = 0; i < coords3DSize; i++)
	{
		coords3D[i] = CalibrationObject::getInstance()->getFrameSpecifications()[i];
	}

	alldetectedPointsSize = Project::getInstance()->getCameras()[m_camera]->getCalibrationImages()[m_image]->getDetectedPointsAll().size();
	alldetectedPoints = new cv::Point2d[alldetectedPointsSize];
	for (int i = 0; i < alldetectedPointsSize; i++)
	{
		alldetectedPoints[i] = Project::getInstance()->getCameras()[m_camera]->getCalibrationImages()[m_image]->getDetectedPointsAll()[i];
	}

	for (int i = 0; i < alldetectedPointsSize; i++)
	{
		alldetectedPoints[i] = Project::getInstance()->getCameras()[m_camera]->undistortPoint(alldetectedPoints[i], true);
	}

	rotationvector.create(3, 1,CV_64F);
	translationvector.create(3, 1,CV_64F);
	projection.create(3, 4, CV_64F);
	cameramatrix.create(3, 3, CV_64F);
	calibrated = Project::getInstance()->getCameras()[camera]->isCalibrated();
}

CubeCalibration::~CubeCalibration()
{
	rotationvector.release();
	translationvector.release();
	projection.release();
	cameramatrix.release();
	transformationmatrix.release();

	delete coords3D;
	delete alldetectedPoints;

	projectedPoints.clear();
	detectedPoints.clear();
	error.clear();
	Inlier.clear();
}

void CubeCalibration::computePoseAndCam()
{
	m_FutureWatcher = new QFutureWatcher<void>();
	connect(m_FutureWatcher, SIGNAL( finished() ), this, SLOT( computePoseAndCam_threadFinished() ));

	QFuture<void> future = QtConcurrent::run(this, &CubeCalibration::computePoseAndCam_thread);
	m_FutureWatcher->setFuture(future);

	ProgressDialog::getInstance()->showProgressbar(0, 0, "Compute Camera internal parameters and pose");
}

void CubeCalibration::computePoseAndCam_threadFinished()
{
	if (calibrated)
	{
		Project::getInstance()->getCameras()[m_camera]->setCameraMatrix(cameramatrix);
		Project::getInstance()->getCameras()[m_camera]->getCalibrationImages()[m_image]->setPointsUndistorted(detectedPoints, projectedPoints, Inlier);
		Project::getInstance()->getCameras()[m_camera]->getCalibrationImages()[m_image]->setMatrices(rotationvector, translationvector);
		Project::getInstance()->getCameras()[m_camera]->setRecalibrationRequired(1);
	}
	else
	{
		Project::getInstance()->getCameras()[m_camera]->getCalibrationImages()[m_image]->setCalibrated(-1);
	}


	delete m_FutureWatcher;
	nbInstances--;
	if (nbInstances == 0)
	{
		MainWindow::getInstance()->redrawGL();
		ProgressDialog::getInstance()->closeProgressbar();
		emit computePoseAndCam_finished();
	}
	delete this;
}

void CubeCalibration::computePoseAndCam_thread()
{
	int count = 0;
	for (int i = 0; i < 4; i++)
	{
		if (selectedPoints[i].x > 0 && selectedPoints[i].y > 0)
		{
			count++;
		}
	}
	// First find best set with ransac
	if (count > 3) setupCorrespondancesRansac(100000, 10.0);
	else if (count == 3) setupCorrespondancesRansac(1000000, 10.0);
	// First find best set with ransac

	if (calibrateOpenCV())
		refineResults(true);
}

void CubeCalibration::computePose()
{
	cameramatrix = Project::getInstance()->getCameras()[m_camera]->getCameraMatrix();

	m_FutureWatcher = new QFutureWatcher<void>();
	connect(m_FutureWatcher, SIGNAL( finished() ), this, SLOT( computePose_threadFinished() ));

	QFuture<void> future = QtConcurrent::run(this, &CubeCalibration::computePose_thread);
	m_FutureWatcher->setFuture(future);
	ProgressDialog::getInstance()->showProgressbar(0, 0, "Compute Camera pose");
}

void CubeCalibration::computePose_threadFinished()
{
	if (poseComputed)
	{
		Project::getInstance()->getCameras()[m_camera]->getCalibrationImages()[m_image]->setPointsUndistorted(detectedPoints, projectedPoints, Inlier);
		Project::getInstance()->getCameras()[m_camera]->getCalibrationImages()[m_image]->setMatrices(rotationvector, translationvector);
		Project::getInstance()->getCameras()[m_camera]->setRecalibrationRequired(1);
	}
	else
	{
		Project::getInstance()->getCameras()[m_camera]->getCalibrationImages()[m_image]->setCalibrated(-1);
	}

	delete m_FutureWatcher;
	nbInstances--;
	if (nbInstances == 0)
	{
		ProgressDialog::getInstance()->closeProgressbar();
		emit computePose_finished();
	}
	delete this;
}

void CubeCalibration::computePose_thread()
{
	int count = 0;
	for (int i = 0; i < 4; i++)
	{
		if (selectedPoints[i].x > 0 && selectedPoints[i].y > 0)
		{
			count++;
		}
	}

	if (count == 3) setupCorrespondancesRansacPose(100000, 10.0);
	else setupCorrespondancesRansacPose(1000, 10.0);

	refineResults(false);
}

void CubeCalibration::setPose(cv::Mat trans)
{
	cameramatrix = Project::getInstance()->getCameras()[m_camera]->getCameraMatrix();
	transformationmatrix = trans.clone();

	m_FutureWatcher = new QFutureWatcher<void>();
	connect(m_FutureWatcher, SIGNAL( finished() ), this, SLOT( computePose_threadFinished() ));

	QFuture<void> future = QtConcurrent::run(this, &CubeCalibration::setPose_thread);
	m_FutureWatcher->setFuture(future);
	ProgressDialog::getInstance()->showProgressbar(0, 0, "Set Camera pose from other frame ");
}

void CubeCalibration::setPose_thread()
{
	cv::Mat trans_inv = transformationmatrix.inv();
	cv::Mat rotationmatrix;
	rotationmatrix.create(3, 3,CV_64F);
	//set R
	for (unsigned int i = 0; i < 3; ++i)
	{
		for (unsigned int j = 0; j < 3; ++j)
		{
			rotationmatrix.at<double>(i, j) = trans_inv.at<double>(i, j);
		}
	}
	//set R vec
	cv::Rodrigues(rotationmatrix, rotationvector);

	//set t
	for (unsigned int j = 0; j < 3; ++j)
	{
		translationvector.at<double>(j, 0) = trans_inv.at<double>(j, 3) ;
	}

	//redetect points;
	reprojectAndComputeError();
	maxinlier = setCorrespondances(Settings::getInstance()->getIntSetting("IdentificationThresholdCalibration"), true);
	setPoseFromInlier();

	refineResults(false);
}


void CubeCalibration::getRandomReferences(unsigned int nbPoints, std::vector<cv::Point2d>& pt2d, std::vector<cv::Point3d>& pt3d)
{
	unsigned int pts_Set = 0;
	unsigned int ptsUsed = nbPoints;
	cv::Mat idx_keypoints;
	idx_keypoints.create(ptsUsed, 1, CV_32S);
	cv::Mat idx_coords3D;
	idx_coords3D.create(ptsUsed, 1, CV_32S);

	for (unsigned int i = 0; i < 4; i++)
	{
		if (selectedPoints[i].x > 0 && selectedPoints[i].y > 0)
		{
			idx_coords3D.at<int>(pts_Set, 0) = selectedPointsID[i];

			pt3d.push_back(cv::Point3d(coords3D[idx_coords3D.at<int>(pts_Set, 0)].x,
			                           coords3D[idx_coords3D.at<int>(pts_Set, 0)].y,
			                           coords3D[idx_coords3D.at<int>(pts_Set, 0)].z));
			pt2d.push_back(cv::Point2d(selectedPoints[i].x, selectedPoints[i].y));

			idx_keypoints.at<int>(pts_Set, 0) = -1;
			pts_Set++;
		}
	}

	while (pts_Set < ptsUsed)
	{
		//find a not used 3D point
		int ran_3D = rand() % coords3DSize;
		bool okay_3D = true;
		for (unsigned int i = 0; i < pts_Set; i++)
		{
			if (idx_coords3D.at<int>(i, 0) == ran_3D)okay_3D = false;
		}

		//find a not used 2D point
		int ran_2D = rand() % alldetectedPointsSize;
		bool okay_2D = true;
		for (unsigned int i = 0; i < pts_Set; i++)
		{
			if (idx_keypoints.at<int>(i, 0) == ran_2D)okay_2D = false;
		}

		//if both are okay add them to the set
		if (okay_2D && okay_3D)
		{
			idx_coords3D.at<int>(pts_Set, 0) = ran_3D;
			pt3d.push_back(cv::Point3d(coords3D[ran_3D].x, coords3D[ran_3D].y, coords3D[ran_3D].z));
			idx_keypoints.at<int>(pts_Set, 0) = ran_2D;
			pt2d.push_back(cv::Point2d(alldetectedPoints[ran_2D].x, alldetectedPoints[ran_2D].y));
			pts_Set++;
		}
	}
}

double CubeCalibration::euclideanDist(cv::Point2d& p, cv::Point2d& q)
{
	cv::Point2d diff = p - q;
	return cv::sqrt(diff.x * diff.x + diff.y * diff.y);
}

bool CubeCalibration::calibrateOpenCV(bool singleFocal)
{
	std::vector<cv::Point3d> corr3D;
	std::vector<cv::Point2d> corr2D;

	for (unsigned int i = 0; i < detectedPoints.size(); i++)
	{
		if (Inlier[i] && detectedPoints[i].x > -1 && detectedPoints[i].y > -1)
		{
			corr3D.push_back(cv::Point3d(coords3D[i].x, coords3D[i].y, coords3D[i].z));
			corr2D.push_back(cv::Point2d(detectedPoints[i].x, detectedPoints[i].y));
		}
	}

	if (corr3D.size() > 8)
	{

		std::vector<std::vector<cv::Point3f> > object_points;
		std::vector<std::vector<cv::Point2f> > image_points;
		object_points.push_back(std::vector<cv::Point3f>());
		image_points.push_back(std::vector<cv::Point2f>());

		std::vector <float> distCoefs;
		distCoefs.resize(5, 0.0f);

		std::vector<cv::Mat> rvecs, tvecs;

		// Transfer the points into the correct size matrices
		for (unsigned int i = 0; i < corr3D.size(); ++i)
		{
			object_points[0].push_back(cv::Point3f(
				corr3D[i].x,
				corr3D[i].y,
				corr3D[i].z
			));
			image_points[0].push_back(cv::Point2f(
				corr2D[i].x,
				corr2D[i].y
			));
		}

		if (cameramatrix.at<double>(0, 2) < 0) cameramatrix.at<double>(0, 2) = 1;
		if (cameramatrix.at<double>(0, 2) > Project::getInstance()->getCameras()[m_camera]->getWidth()) cameramatrix.at<double>(0, 2) = Project::getInstance()->getCameras()[m_camera]->getWidth() - 1;
		if (cameramatrix.at<double>(1, 2) < 0) cameramatrix.at<double>(1, 2) = 0;
		if (cameramatrix.at<double>(1, 2) > Project::getInstance()->getCameras()[m_camera]->getHeight()) cameramatrix.at<double>(1, 2) = Project::getInstance()->getCameras()[m_camera]->getHeight() - 1;
		cameramatrix.at<double>(0, 1) = 0;

		int flags = cv::CALIB_USE_INTRINSIC_GUESS + cv::CALIB_FIX_K1 + cv::CALIB_FIX_K2 + cv::CALIB_FIX_K3 + cv::CALIB_ZERO_TANGENT_DIST;
		if (singleFocal)
		{
			flags += cv::CALIB_FIX_ASPECT_RATIO ;
			double focal = 0.5 * (cameramatrix.at<double>(0, 0) + cameramatrix.at<double>(1, 1));
			cameramatrix.at<double>(0, 0) = focal;
			cameramatrix.at<double>(1, 1) = focal;
		}
		if (Settings::getInstance()->getBoolSetting("FixPrincipal"))
		{
			std::cerr << "fix" << std::endl;
			flags += cv::CALIB_FIX_PRINCIPAL_POINT;
			cameramatrix.at<double>(0, 2) = 0.5 * (Project::getInstance()->getCameras()[m_camera]->getWidth() + 1);
			cameramatrix.at<double>(1, 2) = 0.5 * (Project::getInstance()->getCameras()[m_camera]->getHeight() + 1);
		}
		try
		{
			cv::calibrateCamera(
				object_points,
				image_points,
				cv::Size(Project::getInstance()->getCameras()[m_camera]->getWidth(), Project::getInstance()->getCameras()[m_camera]->getHeight()),
				cameramatrix,
				distCoefs,
				rvecs,
				tvecs,
				flags
			);
			rotationvector = rvecs[0];
			translationvector = tvecs[0];

			poseComputed = true;
			reprojectAndComputeError();
		}
		catch (std::exception& e)
		{
			fprintf(stderr, "Frame::calibrateOpenCV : cvCalibrateCamera2 Failed with Exception - %s\n", e.what());
			poseComputed = false;
		}

		corr3D.clear();
		corr2D.clear();

		return poseComputed;
	}

	corr3D.clear();
	corr2D.clear();

	return false;
}

void CubeCalibration::reprojectAndComputeError()
{

	std::vector<cv::Point3f>object_points;
	std::vector<cv::Point2f>image_points;

	// Transfer the points into the correct size matrices
	for (unsigned int i = 0; i < coords3DSize; ++i)
	{
		object_points.push_back(cv::Point3f(
			coords3D[i].x,
			coords3D[i].y,
			coords3D[i].z));
		image_points.push_back(cv::Point2f(0, 0));
	}

	try
	{
		std::vector<float> distCoeff;
		cv::projectPoints(
			object_points,
			rotationvector,
			translationvector,
			cameramatrix,
			distCoeff,
			image_points);

		projectedPoints.clear();
		for (int i = 0; i < coords3DSize; i++)
		{
			cv::Point2d pt = image_points[i];
			projectedPoints.push_back(pt);
		}

		for (unsigned int i = 0; i < detectedPoints.size(); i++)
		{
			error[i] = euclideanDist(projectedPoints[i], detectedPoints[i]);
		}
	}
	catch (std::exception e)
	{
		fprintf(stderr, "Frame::reprojectAndComputeError : cvProjectPoints2 Failed with Exception - %s\n", e.what());
	}
}

void CubeCalibration::setupCorrespondancesRansac(unsigned int loop_max, double threshold)
{
	cv::Mat tmp_projection;
	tmp_projection.create(3, 4, CV_64F);
	maxinlier = 0;
	//fprintf(stderr,"Start Ransac\n", maxinlier);
	for (unsigned int loop = 0; loop < loop_max; loop ++)
	{
		//Vector for points
		std::vector<cv::Point2d> pt2d;
		std::vector<cv::Point3d> pt3d;
		getRandomReferences(6, pt2d, pt3d);

		double error_projection = computeProjection(pt2d, pt3d, tmp_projection);

		//Then compute the projection and find the inlier in case the projection fits the points well
		if (error_projection < threshold)
		{
			int inlier = computeInlier(tmp_projection, threshold);

			//if the inlier set is better than the previous one, set it
			if (inlier > maxinlier)
			{
				maxinlier = inlier;

				projection = tmp_projection.clone();
				if (maxinlier > 0)
				{
					computeProjectionFromInliers(threshold);
				}
				fprintf(stderr, "Inlier %d\n", maxinlier);
			}
		}
	}
}

int CubeCalibration::selectCorrespondances(double threshold)
{
	if (error.size() != Inlier.size())
	{
		fprintf(stderr, "Frame::selectCorrespondances : error.size() != Inlier.size()\n");
		return 0;
	}

	int count = 0;
	for (unsigned int i = 0; i < Inlier.size(); i++)
		if (error[i] < threshold)
		{
			Inlier[i] = true;
			count ++;
		}
		else
		{
			Inlier[i] = false;
		}

	return count;
}

void CubeCalibration::refineResults(bool withCameraRefinement)
{
	maxinlier = setCorrespondances(0.5 * Settings::getInstance()->getIntSetting("IdentificationThresholdCalibration"), true);

	if (maxinlier > 5)
	{
		int inlier_tmp = 0;

		while (inlier_tmp == 0 || inlier_tmp < maxinlier)
		{
			inlier_tmp = maxinlier;

			if (withCameraRefinement)
			{
				calibrateFromInliers();
			}
			else
			{
				setPoseFromInlier();
			}

			setCorrespondances(Settings::getInstance()->getIntSetting("IdentificationThresholdCalibration"), false);
			maxinlier = selectCorrespondances(Settings::getInstance()->getIntSetting("OutlierThresholdForCalibration"));
		}

		if (withCameraRefinement) calibrated = true;
		poseComputed = true;

		if (withCameraRefinement)
		{
			calibrateFromInliers();
		}
		else
		{
			setPoseFromInlier();
		}
	}
	else
	{
		projectedPoints.clear();
		detectedPoints.clear();
		if (withCameraRefinement) calibrated = false;
		poseComputed = false;
	}
}

void CubeCalibration::setPoseFromInlier()
{
	std::vector<cv::Point2d> pt2d;
	std::vector<cv::Point3d> pt3d;

	if (Inlier.size() == detectedPoints.size() && coords3DSize == Inlier.size())
	{
		for (unsigned int i = 0; i < Inlier.size(); i++)
		{
			if (Inlier[i])
			{
				pt3d.push_back(cv::Point3d(coords3D[i].x,
				                           coords3D[i].y,
				                           coords3D[i].z));
				pt2d.push_back(cv::Point2d(detectedPoints[i].x, detectedPoints[i].y));
			}
		}

		computePose(pt2d, pt3d);
	}
	pt2d.clear();
	pt3d.clear();
}

void CubeCalibration::calibrateFromInliers(bool singleFocal)
{
	computeProjectionMatrixFromInlier();

	//compute K
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

	cameramatrix.at<double>(0, 0) = a;
	cameramatrix.at<double>(0, 1) = g;
	cameramatrix.at<double>(0, 2) = u0;
	cameramatrix.at<double>(1, 0) = 0;
	cameramatrix.at<double>(1, 1) = b;
	cameramatrix.at<double>(1, 2) = v0;
	cameramatrix.at<double>(2, 0) = 0;
	cameramatrix.at<double>(2, 1) = 0;
	cameramatrix.at<double>(2, 2) = C.at<double>(2, 2) / C.at<double>(2, 2);

	calibrateOpenCV(singleFocal);
}

void CubeCalibration::computeProjectionMatrixFromInlier()
{
	std::vector<cv::Point3d> corr3D;
	std::vector<cv::Point2d> corr2D;

	for (unsigned int i = 0; i < detectedPoints.size(); i++)
	{
		if (Inlier[i] && detectedPoints[i].x > -1 && detectedPoints[i].y > -1)
		{
			corr3D.push_back(cv::Point3d(coords3D[i].x, coords3D[i].y, coords3D[i].z));
			corr2D.push_back(cv::Point2d(detectedPoints[i].x, detectedPoints[i].y));
		}
	}

	if (computeProjection(corr2D, corr3D, projection) < 1000)
	{
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

		projection = projection / sqrt(C.at<double>(2, 2));
	}

	corr3D.clear();
	corr2D.clear();
}

void CubeCalibration::computePose(std::vector<cv::Point2d> pt2d, std::vector<cv::Point3d> pt3d)
{
	if (calibrated && pt2d.size() == pt3d.size() && pt2d.size() >= 5)
	{

		std::vector<cv::Point3f> object_points;
		std::vector<cv::Point2f> image_points;
		
		std::vector <float> distCoefs;
		cv::Mat rvec, tvec;

		// Transfer the points into the correct size matrices
		for (unsigned int i = 0; i < pt3d.size(); ++i)
		{
			object_points.push_back(cv::Point3f(
				pt3d[i].x,
				pt3d[i].y,
				pt3d[i].z
			));
			image_points.push_back(cv::Point2f(
				pt2d[i].x,
				pt2d[i].y
			));
		}

		try
		{
			cv::solvePnP(object_points, image_points, cameramatrix, distCoefs, rvec, tvec, false);

			rotationvector = rvec;
			translationvector = tvec;
		
			reprojectAndComputeError();
		}
		catch (std::exception& e)
		{
			fprintf(stderr, "Frame::computePose : cvFindExtrinsicCameraParams2 Failed with Exception - %s\n", e.what());
			poseComputed = false;
		}
	}
}

int CubeCalibration::setCorrespondances(double threshold, bool setAsInliers)
{
	if (projectedPoints.size() != coords3DSize || alldetectedPointsSize == 0) return 0;

	double distmin;
	double d;
	int inlier;

	cv::Mat mask_dist;
	mask_dist.create(alldetectedPointsSize, 1,CV_64F);
	for (int j = 0; j < alldetectedPointsSize; j++)
	{
		mask_dist.at<double>(j, 0) = threshold;
	}

	bool duplicates = true;
	while (duplicates)
	{
		detectedPoints.clear();
		if (setAsInliers)Inlier.clear();
		error.clear();

		inlier = 0;
		duplicates = false;
		for (int i = 0; i < coords3DSize; i++)
		{
			//Project cube points
			cv::Point2d pt2d = projectedPoints[i];

			//find closest point
			distmin = threshold;
			int idx = 0;
			for (int j = 0; j < alldetectedPointsSize; j++)
			{
				cv::Point2d pttmp = cv::Point2d(alldetectedPoints[j].x, alldetectedPoints[j].y);

				d = euclideanDist(pt2d, pttmp);
				//best match below threshold for this point and no other point is already closer
				if (d < distmin && d < threshold && d <= mask_dist.at<double>(j, 0))
				{
					distmin = d;
					idx = j;
				}
			}

			if (distmin < threshold)
			{
				if (distmin < mask_dist.at<double>(idx, 0)) duplicates = true;
				mask_dist.at<double>(idx, 0) = distmin;

				detectedPoints.push_back(cv::Point2d(alldetectedPoints[idx].x, alldetectedPoints[idx].y));
				if (setAsInliers)Inlier.push_back(true);
				error.push_back(distmin);

				inlier ++;
			}
			else
			{
				detectedPoints.push_back(cv::Point2d(-1, -1));
				if (setAsInliers)Inlier.push_back(false);
				error.push_back(10000);
			}
		}
	}
	return inlier;
}

double CubeCalibration::computeProjection(std::vector<cv::Point2d> pt2d, std::vector<cv::Point3d> pt3d, cv::Mat& _projection)
{
	if (pt2d.size() != pt3d.size() || pt2d.size() < 6) return 1000;

	cv::Mat f;
	f.create(12, 1, CV_64F);

	cv::Mat A;
	A.create(pt2d.size() * 2, 12, CV_64F);

	//Fill A-Matrix
	for (unsigned int i = 0; i < pt2d.size(); i++)
	{
		A.at<double>(2 * i, 0) = pt3d[i].x;
		A.at<double>(2 * i, 1) = pt3d[i].y;
		A.at<double>(2 * i, 2) = pt3d[i].z;
		A.at<double>(2 * i, 3) = 1.0;
		A.at<double>(2 * i, 4) = 0;
		A.at<double>(2 * i, 5) = 0;
		A.at<double>(2 * i, 6) = 0;
		A.at<double>(2 * i, 7) = 0;
		A.at<double>(2 * i, 8) = -pt2d[i].x * pt3d[i].x;
		A.at<double>(2 * i, 9) = -pt2d[i].x * pt3d[i].y;
		A.at<double>(2 * i, 10) = -pt2d[i].x * pt3d[i].z;
		A.at<double>(2 * i, 11) = -pt2d[i].x;

		A.at<double>(2 * i + 1, 0) = 0;
		A.at<double>(2 * i + 1, 1) = 0;
		A.at<double>(2 * i + 1, 2) = 0;
		A.at<double>(2 * i + 1, 3) = 0;
		A.at<double>(2 * i + 1, 4) = pt3d[i].x;
		A.at<double>(2 * i + 1, 5) = pt3d[i].y;
		A.at<double>(2 * i + 1, 6) = pt3d[i].z;
		A.at<double>(2 * i + 1, 7) = 1.0;
		A.at<double>(2 * i + 1, 8) = -pt2d[i].y * pt3d[i].x;
		A.at<double>(2 * i + 1, 9) = -pt2d[i].y * pt3d[i].y;
		A.at<double>(2 * i + 1, 10) = -pt2d[i].y * pt3d[i].z;
		A.at<double>(2 * i + 1, 11) = -pt2d[i].y;
	}

	try
	{
		cv::SVD::solveZ(A, f);

		//set projection matrix
		_projection.at<double>(0, 0) = f.at<double>(0, 0) ;
		_projection.at<double>(0, 1) = f.at<double>(1, 0);
		_projection.at<double>(0, 2) = f.at<double>(2, 0);
		_projection.at<double>(0, 3) = f.at<double>(3, 0);

		_projection.at<double>(1, 0) = f.at<double>(4, 0);
		_projection.at<double>(1, 1) = f.at<double>(5, 0);
		_projection.at<double>(1, 2) = f.at<double>(6, 0);
		_projection.at<double>(1, 3) = f.at<double>(7, 0);

		_projection.at<double>(2, 0) = f.at<double>(8, 0);
		_projection.at<double>(2, 1) = f.at<double>(9, 0);
		_projection.at<double>(2, 2) = f.at<double>(10, 0);
		_projection.at<double>(2, 3) = f.at<double>(11, 0);

		//compute and return reprojection error
		cv::Mat pt4d;
		pt4d.create(4, 1, CV_64F);
		cv::Mat pt3di;
		double d = 0;
		for (unsigned int i = 0; i < pt2d.size(); i++)
		{
			//Project cube points
			pt4d.at<double>(0, 0) = pt3d[i].x;
			pt4d.at<double>(1, 0) = pt3d[i].y;
			pt4d.at<double>(2, 0) = pt3d[i].z;
			pt4d.at<double>(3, 0) = 1.0;
			pt3di = _projection * pt4d;
			cv::Point2d pt2di = cv::Point2d(pt3di.at<double>(0, 0) / pt3di.at<double>(2, 0), pt3di.at<double>(1, 0) / pt3di.at<double>(2, 0));
			//add error
			d += euclideanDist(pt2di, pt2d[i]);
		}
		//return reprojection error per point
		return d / pt2d.size();
	}
	catch (std::exception e)
	{
		fprintf(stderr, "Frame::computeProjection : cv::SVD::solveZ Failed with Exception - %s\n", e.what());
		return 1000;
	}
}

int CubeCalibration::computeInlier(cv::Mat& projection, double threshold)
{
	cv::Mat pt4d;
	pt4d.create(4, 1, CV_64F);
	cv::Mat pt3d;
	double distmin;
	double d;
	int inlier;

	cv::Mat mask_dist;
	mask_dist.create(alldetectedPointsSize, 1,CV_64F);
	for (int j = 0; j < alldetectedPointsSize; j++)
	{
		mask_dist.at<double>(j, 0) = threshold;
	}

	bool duplicates = true;
	while (duplicates)
	{
		inlier = 0;
		duplicates = false;
		for (int i = 0; i < coords3DSize; i++)
		{
			//Project cube points
			pt4d.at<double>(0, 0) = coords3D[i].x;
			pt4d.at<double>(1, 0) = coords3D[i].y;
			pt4d.at<double>(2, 0) = coords3D[i].z;
			pt4d.at<double>(3, 0) = 1.0;
			pt3d = projection * pt4d;
			cv::Point2d pt2d = cv::Point2d(pt3d.at<double>(0, 0) / pt3d.at<double>(2, 0), pt3d.at<double>(1, 0) / pt3d.at<double>(2, 0));

			//find closest point
			distmin = threshold;
			int idx = 0;
			for (int j = 0; j < alldetectedPointsSize; j++)
			{
				cv::Point2d pttmp = cv::Point2d(alldetectedPoints[j].x, alldetectedPoints[j].y);
				d = euclideanDist(pt2d, pttmp);
				//best match below threshold for this point and no other point is already closer

				if (d < distmin && d < threshold && d <= mask_dist.at<double>(j, 0))
				{
					distmin = d;
					idx = j;
				}
			}

			if (distmin < threshold)
			{
				if (distmin < mask_dist.at<double>(idx, 0)) duplicates = true;
				mask_dist.at<double>(idx, 0) = distmin;
				inlier ++;
			}
		}
	}

	return inlier;
}

void CubeCalibration::computeProjectionFromInliers(double threshold)
{
	bool inlier_changed = true;
	int inlier_prev = maxinlier;

	while (inlier_changed)
	{
		inlier_changed = false;
		std::vector<cv::Point3d> corr3D;
		std::vector<cv::Point2d> corr2D;

		cv::Mat pt4d;
		pt4d.create(4, 1, CV_64F);
		cv::Mat pt3d;
		double distmin;
		double d;
		int inlier;

		cv::Mat mask_dist;
		mask_dist.create(alldetectedPointsSize, 1,CV_64F);
		for (int j = 0; j < alldetectedPointsSize; j++)
		{
			mask_dist.at<double>(j, 0) = threshold;
		}

		bool duplicates = true;
		while (duplicates)
		{
			corr3D.clear();
			corr2D.clear();
			projectedPoints.clear();
			detectedPoints.clear();
			Inlier.clear();
			error.clear();

			inlier = 0;
			duplicates = false;
			for (int i = 0; i < coords3DSize; i++)
			{
				//Project cube points
				pt4d.at<double>(0, 0) = coords3D[i].x;
				pt4d.at<double>(1, 0) = coords3D[i].y;
				pt4d.at<double>(2, 0) = coords3D[i].z;
				pt4d.at<double>(3, 0) = 1.0;
				pt3d = projection * pt4d;
				cv::Point2d pt2d = cv::Point2d(pt3d.at<double>(0, 0) / pt3d.at<double>(2, 0), pt3d.at<double>(1, 0) / pt3d.at<double>(2, 0));

				//find closest point
				distmin = threshold;
				int idx = 0;
				for (int j = 0; j < alldetectedPointsSize; j++)
				{
					cv::Point2d pttmp = cv::Point2d(alldetectedPoints[j].x, alldetectedPoints[j].y);
					d = euclideanDist(pt2d, pttmp);
					//best match below threshold for this point and no other point is already closer
					if (d < distmin && d < threshold && d <= mask_dist.at<double>(j, 0))
					{
						distmin = d;
						idx = j;
					}
				}

				if (distmin < threshold)
				{
					if (distmin < mask_dist.at<double>(idx, 0)) duplicates = true;
					mask_dist.at<double>(idx, 0) = distmin;
					corr3D.push_back(cv::Point3d(coords3D[i].x, coords3D[i].y, coords3D[i].z));
					corr2D.push_back(cv::Point2d(alldetectedPoints[idx].x, alldetectedPoints[idx].y));

					projectedPoints.push_back(pt2d);
					detectedPoints.push_back(cv::Point2d(alldetectedPoints[idx].x, alldetectedPoints[idx].y));
					Inlier.push_back(true);
					error.push_back(distmin);

					inlier ++;
				}
				else
				{
					projectedPoints.push_back(pt2d);
					detectedPoints.push_back(cv::Point2d(-1, -1));
					Inlier.push_back(false);
					error.push_back(10000);
				}
			}
		}

		if (inlier > maxinlier)
		{
			maxinlier = inlier;
			inlier_changed = true;
			inlier_prev = maxinlier;
		}

		computeProjection(corr2D, corr3D, projection);

		//compute K
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

		cameramatrix.create(3, 3, CV_64F);
		cameramatrix.at<double>(0, 0) = a;
		cameramatrix.at<double>(0, 1) = g;
		cameramatrix.at<double>(0, 2) = u0;
		cameramatrix.at<double>(1, 0) = 0;
		cameramatrix.at<double>(1, 1) = b;
		cameramatrix.at<double>(1, 2) = v0;
		cameramatrix.at<double>(2, 0) = 0;
		cameramatrix.at<double>(2, 1) = 0;
		cameramatrix.at<double>(2, 2) = C.at<double>(2, 2) / C.at<double>(2, 2);

		projection = projection / sqrt(C.at<double>(2, 2));
	}
}

void CubeCalibration::setupCorrespondancesRansacPose(unsigned int loop_max, double threshold)
{
	maxinlier = 0;

	std::vector<cv::Point2d> pt2d_best;
	std::vector<cv::Point3d> pt3d_best;
	//fprintf(stderr,"Start Ransac\n", maxinlier);
	for (unsigned int loop = 0; loop < loop_max; loop ++)
	{
		//Vector for points
		std::vector<cv::Point2d> pt2d;
		std::vector<cv::Point3d> pt3d;

		getRandomReferences(5, pt2d, pt3d);
		//Then compute the projection and find the inlier in case the projection fits the points well
		{
			computePose(pt2d, pt3d);
			int inlier = setCorrespondances(0.5 * Settings::getInstance()->getIntSetting("IdentificationThresholdCalibration"), true);

			//if the inlier set is better than the previous one, set it
			if (inlier > maxinlier)
			{
				pt2d_best.clear();
				pt3d_best.clear();

				maxinlier = inlier;

				for (unsigned int i = 0; i < Inlier.size(); i++)
				{
					if (Inlier[i])
					{
						pt2d_best.push_back(detectedPoints[i]);
						pt3d_best.push_back(coords3D[i]);
					}
				}
				//fprintf(stderr,"Inlier %d\n", maxinlier);
			}
		}
	}

	if (maxinlier >= 4)
	{
		computePose(pt2d_best, pt3d_best);
		reprojectAndComputeError();
		maxinlier = setCorrespondances(Settings::getInstance()->getIntSetting("IdentificationThresholdCalibration"), true);
	}
}

