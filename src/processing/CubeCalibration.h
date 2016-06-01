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
///\file CubeCalibration.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef CUBECALIBRATION_H
#define CUBECALIBRATION_H

#include <QFutureWatcher>
#include <QObject>

#include <opencv/cv.h>

namespace xma
{
	class CubeCalibration : public QObject
	{
		Q_OBJECT;

	public:
		CubeCalibration(int camera, int image, cv::Point2d references[4], int referencesID[4]);
		virtual ~CubeCalibration();

		void computePoseAndCam();
		void computePose();
		void setPose(cv::Mat trans);

		static bool isRunning()
		{
			return (nbInstances > 0);
		}

		signals:
		void computePoseAndCam_finished();
		void computePose_finished();

	private slots:
		void computePoseAndCam_threadFinished();
		void computePose_threadFinished();

	private:
		int m_camera;
		int m_image;

		void computePoseAndCam_thread();
		void computePose_thread();
		void setPose_thread();

		QFutureWatcher<void>* m_FutureWatcher;
		static int nbInstances;

		cv::Point2d selectedPoints[4];
		int selectedPointsID[4];

		void getRandomReferences(unsigned int nbPoints, cv::vector<cv::Point2d>& pt2d, cv::vector<cv::Point3d>& pt3d);
		double euclideanDist(cv::Point2d& p, cv::Point2d& q);
		bool calibrateOpenCV(bool singleFocal = false, bool useImageCenter = false);
		void reprojectAndComputeError();

		void setupCorrespondancesRansac(unsigned int loop_max, double threshold);
		double computeProjection(cv::vector<cv::Point2d> pt2d, cv::vector<cv::Point3d> pt3d, cv::Mat& _projection);
		int computeInlier(cv::Mat& projection, double threshold);
		void computeProjectionFromInliers(double threshold);
		void refineResults(bool withCameraRefinement);
		int setCorrespondances(double threshold, bool setAsInliers);
		void setPoseFromInlier();
		void calibrateFromInliers(bool singleFocal = false, bool useImageCenter = false);
		void computeProjectionMatrixFromInlier();
		void computePose(cv::vector<cv::Point2d> pt2d, cv::vector<cv::Point3d> pt3d);
		int selectCorrespondances(double threshold);

		void setupCorrespondancesRansacPose(unsigned int loop_max, double threshold);
		//temporary variables
		int maxinlier;
		bool poseComputed;
		bool calibrated;
		int coords3DSize;
		cv::Point3d* coords3D;
		int alldetectedPointsSize;
		cv::Point2d* alldetectedPoints;

		cv::vector<cv::Point2d> projectedPoints;
		cv::vector<cv::Point2d> detectedPoints;
		cv::vector<double> error;
		cv::vector<bool> Inlier;

		cv::Mat projection;
		cv::Mat rotationvector;
		cv::Mat translationvector;
		cv::Mat cameramatrix;
		cv::Mat transformationmatrix;
	};
}

#endif // CUBECALIBRATION_H


