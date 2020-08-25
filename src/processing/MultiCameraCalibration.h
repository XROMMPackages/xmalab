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
///\file MultiCameraCalibration.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef MULTICAMERACALIBRATION_H
#define MULTICAMERACALIBRATION_H

#include <QFutureWatcher>
#include <QObject>

#include <opencv2/opencv.hpp>

namespace xma
{
	class MultiCameraCalibration : public QObject
	{
		Q_OBJECT;

	public:
		MultiCameraCalibration(int method, int iterations, double initial);
		virtual ~MultiCameraCalibration();

		void optimizeCameraSetup();

		static bool isRunning()
		{
			return (nbInstances > 0);
		}

		int nbCameras;
		int nbFrames;
		int nbPoints;
		int nbParams;

		int nbCamParams;


		double* p;
		double* x;
		void projError(int n, double* p, double* x);
		void projErrorJac(int n, int m, double* p, double* Jac);
		int getStep();

		static void reproject(int c);

		signals:
		void optimizeCameraSetup_finished();

	private slots:
		void optimizeCameraSetup_threadFinished();

	private:

		int m_iterations;
		int m_method;
		double m_initial;

		bool withDistortion;
		bool seperateDimensions;

		std::vector<int> frames;
		std::vector<int> frameIdx;
		std::vector<int> camIdx;
		std::vector<int> pts3DIdx;
		std::vector<cv::Point2d> Pts2D;
		std::vector<cv::Point3d> Pts3D;
		std::vector<cv::Mat> cameraRotationVector;
		std::vector<cv::Mat> cameraTranslationVector;
		std::vector<cv::Mat> cameraMatrix;
		std::vector<cv::Mat> distortion;
		std::vector<cv::Mat> chessRotationVector;
		std::vector<cv::Mat> chessTranslationVector;

		void optimizeCameraSetup_thread();
		cv::Mat getTransformationMatrix(cv::Mat rotationvector, cv::Mat translationvector);
		cv::Mat getTranslation(cv::Mat trans);
		cv::Mat getRotationVector(cv::Mat trans);
		int refIdx;


		QFutureWatcher<void>* m_FutureWatcher;
		static int nbInstances;
	};
}
#endif // MULTICAMERACALIBRATION_H


