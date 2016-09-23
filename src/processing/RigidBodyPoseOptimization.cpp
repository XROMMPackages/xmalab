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
///\file RigidBodyPoseComputation.cpp
///\author Benjamin Knorlein
///\date 09/20/2016

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "processing/RigidBodyPoseOptimization.h" 

#include "ui/MainWindow.h"

#include "core/Project.h"
#include "core/Camera.h"
#include "core/Trial.h"
#include "core/RigidBody.h"
#include "core/Marker.h"
#include "core/CalibrationImage.h"
#include "core/CalibrationObject.h"

#include <math.h>

#include "levmar.h"

using namespace xma;

QMutex RigidBodyPoseOptimization::mutex;

void rosRigidBody(double* p, double* x, int m, int n, void* data)
{
	RigidBodyPoseOptimization* calib_data = static_cast<RigidBodyPoseOptimization *>(data);
	for (int i = 0; i < n / 2; i++)
	{
		calib_data->projError(i, p, &x[i * 2]);
	}
}

void jacrosRigidBody(double* p, double* jac, int m, int n, void* data)
{
	RigidBodyPoseOptimization* calib_data = static_cast<RigidBodyPoseOptimization *>(data);
	for (int i = 0; i < n/2 ; i++)
	{
		calib_data->projErrorJac(i, m, p, &jac[i* m * 2]);
	}
}

void RigidBodyPoseOptimization::projErrorJac(int n, int m, double* p, double* Jac)
{
	
	double RxCam = cameraRotationVector[cameraIdx[n]].at<double>(0, 0);
	double RyCam = cameraRotationVector[cameraIdx[n]].at<double>(1, 0);
	double RzCam = cameraRotationVector[cameraIdx[n]].at<double>(2, 0);
	double txCam = cameraTranslationVector[cameraIdx[n]].at<double>(0, 0);
	double tyCam = cameraTranslationVector[cameraIdx[n]].at<double>(1, 0);
	double tzCam = cameraTranslationVector[cameraIdx[n]].at<double>(2, 0);
	double fx = cameraMatrix[cameraIdx[n]].at<double>(0, 0);
	double fy = cameraMatrix[cameraIdx[n]].at<double>(1, 1);
	double cx = cameraMatrix[cameraIdx[n]].at<double>(0, 2);
	double cy = cameraMatrix[cameraIdx[n]].at<double>(1, 2);
	double k1, k2, p1, p2, k3;

	k1 = distortion[cameraIdx[n]].at<double>(0, 0);
	k2 = distortion[cameraIdx[n]].at<double>(1, 0);
	p1 = distortion[cameraIdx[n]].at<double>(2, 0);
	p2 = distortion[cameraIdx[n]].at<double>(3, 0);
	k3 = distortion[cameraIdx[n]].at<double>(4, 0);

	double RxBody = p[0];
	double RyBody = p[1];
	double RzBody = p[2];
	double txBody = p[3];
	double tyBody = p[4];
	double tzBody = p[5];

	double X = Pts3D[n].x;
	double Y = Pts3D[n].y;
	double Z = Pts3D[n].z;

	double u = Pts2D[n].x;
	double v = Pts2D[n].y;

	double A0[2][6];

#include "processing/optimizationFuncs/jacFuncWithDistortionPoseSeperateDimensions.h"

	for (int i = 0; i < 2; i++)
	{
		Jac[i*m + 0] = A0[i][0];
		Jac[i*m + 1] = A0[i][1];
		Jac[i*m + 2] = A0[i][2];
		Jac[i*m + 3] = A0[i][3];
		Jac[i*m + 4] = A0[i][4];
		Jac[i*m + 5] = A0[i][5];
		Jac[i*m + 6] = A0[i][6];
	}
}

void RigidBodyPoseOptimization::projError(int n, double* p, double* x)
{
	double RxCam = cameraRotationVector[cameraIdx[n]].at<double>(0,0);
	double RyCam = cameraRotationVector[cameraIdx[n]].at<double>(1,0);
	double RzCam = cameraRotationVector[cameraIdx[n]].at<double>(2,0);
	double txCam = cameraTranslationVector[cameraIdx[n]].at<double>(0, 0);
	double tyCam = cameraTranslationVector[cameraIdx[n]].at<double>(1, 0);
	double tzCam = cameraTranslationVector[cameraIdx[n]].at<double>(2, 0);
	double fx = cameraMatrix[cameraIdx[n]].at<double>(0, 0);
	double fy = cameraMatrix[cameraIdx[n]].at<double>(1, 1);
	double cx = cameraMatrix[cameraIdx[n]].at<double>(0, 2);
	double cy = cameraMatrix[cameraIdx[n]].at<double>(1, 2);
	double k1, k2, p1, p2, k3;

	k1 = distortion[cameraIdx[n]].at<double>(0, 0);
	k2 = distortion[cameraIdx[n]].at<double>(1, 0);
	p1 = distortion[cameraIdx[n]].at<double>(2, 0);
	p2 = distortion[cameraIdx[n]].at<double>(3, 0);
	k3 = distortion[cameraIdx[n]].at<double>(4, 0);

	double RxBody = p[0];
	double RyBody = p[1];
	double RzBody = p[2];
	double txBody = p[3];
	double tyBody = p[4];
	double tzBody = p[5];

	double X = Pts3D[n].x;
	double Y = Pts3D[n].y;
	double Z = Pts3D[n].z;

	double u = Pts2D[n].x;
	double v = Pts2D[n].y;

	double A0[2];

#include "processing/optimizationFuncs/projFuncWithDistortionPoseSeperateDimensions.h"

	x[0] = A0[0];
	x[1] = A0[1];
}


RigidBodyPoseOptimization::RigidBodyPoseOptimization(RigidBody * body, int frame) : m_frame(frame), m_iterations(10000), m_body(body), m_initial(0.01)
{
	nbPoints = 0;
	for (int c = 0; c < Project::getInstance()->getCameras().size(); c++)
	{
		for (int p = 0 ; p < body->getPointsIdx().size(); p++){
			if (body->getTrial()->getMarkers()[body->getPointsIdx()[p]]->getStatus2D()[c][m_frame] > UNDEFINED)
			{
				Pts2D.push_back(Project::getInstance()->getCameras()[c]->undistortPoint(body->getTrial()->getMarkers()[body->getPointsIdx()[p]]->getPoints2D()[c][m_frame], true));
				Pts3D.push_back(body->getReferencePoints()[p]);
				cameraIdx.push_back(c);
				nbPoints++;
			}
		}		
		cameraRotationVector.push_back(Project::getInstance()->getCameras()[c]->getCalibrationImages()[body->getTrial()->getReferenceCalibrationImage()]->getRotationVector());
		cameraTranslationVector.push_back(Project::getInstance()->getCameras()[c]->getCalibrationImages()[body->getTrial()->getReferenceCalibrationImage()]->getTranslationVector());
		cameraMatrix.push_back(Project::getInstance()->getCameras()[c]->getCameraMatrix());
		distortion.push_back(Project::getInstance()->getCameras()[c]->getDistortionCoefficiants());
	}

	nbPoints = nbPoints * 2;

	cv::Mat rot;
	cv::Rodrigues(body->getRotationVector(false)[m_frame], rot);
	cv::Rodrigues(rot.inv(),chessRotationVector);
	cv::Mat trans = -rot.inv() * cv::Mat(body->getTranslationVector(false)[m_frame]);
	chessTranslationVector = cv::Vec3d(trans.at<double>(0, 0), trans.at<double>(1, 0), trans.at<double>(2, 0));

	//Set initial guess
	p = new double[6];

	//Rotation
	p[0] = chessRotationVector[0];
	p[1] = chessRotationVector[1];
	p[2] = chessRotationVector[2];
	//Translation
	p[3] = chessTranslationVector[0];
	p[4] = chessTranslationVector[1];
	p[5] = chessTranslationVector[2];

	//set error vector;
	x = new double[2 * nbPoints];
	for (int i = 0; i < 2 * nbPoints; i++)
	{
		x[i] = 0.0;
	}
}

RigidBodyPoseOptimization::~RigidBodyPoseOptimization()
{
	delete[] x;
	delete[] p;
}

void RigidBodyPoseOptimization::setOutput()
{
	//std::cerr << "Old" << chessRotationVector << std::endl;
	//std::cerr << "Old" << chessTranslationVector << std::endl;
	//std::cerr << "Iterations " << m_iterations << std::endl;
	//Rotation
	chessRotationVector[0] = p[0];
	chessRotationVector[1] = p[1];
	chessRotationVector[2] = p[2];
	//Translation
	chessTranslationVector[0] = p[3];
	chessTranslationVector[1] = p[4];
	chessTranslationVector[2] = p[5];

	//std::cerr << "New" << chessRotationVector << std::endl;
	//std::cerr << "New" << chessTranslationVector << std::endl;


	cv::Mat rot;
	cv::Rodrigues(chessRotationVector, rot);
	cv::Rodrigues(rot.inv(), chessRotationVector);
	cv::Mat trans = -rot.inv() * cv::Mat(chessTranslationVector);
	chessTranslationVector = cv::Vec3d(trans.at<double>(0, 0), trans.at<double>(1, 0), trans.at<double>(2, 0));
	m_body->setTransformation(m_frame, chessRotationVector, chessTranslationVector);
}

void RigidBodyPoseOptimization::optimizeRigidBodySetup()
{
	double opts[LM_OPTS_SZ ], info[LM_INFO_SZ];
	opts[0] = m_initial;
	opts[1] = 1E-30;
	opts[2] = 1E-50;
	opts[3] = 1E-50;
	opts[4] = LM_DIFF_DELTA; // relevant only if the Jacobian is approximated using finite differences; specifies forward differencing 
	
	//levmar does not seem to be thread safe so we have to use a mutex
	mutex.lock();
	//std::cerr << "Initial " << nbPoints <<  std::endl;
	//double out[2];
	//for (int i = 0; i < nbPoints / 2; i++)
	//{
	//	projError(i, p, &out[0]);
	//	std::cerr << out[0] << " , " << out[1] << std::endl;
	//}
	int ret = dlevmar_der(rosRigidBody, jacrosRigidBody, p, x, 6, nbPoints, m_iterations, opts, info, NULL, NULL, this); // with analytic Jacobian
	//std::cerr << info[0] << std::endl;
	//std::cerr << info[1] << std::endl;
	//std::cerr << info[2] << std::endl;
	//std::cerr << info[3] << std::endl;
	//std::cerr << info[4] << std::endl;
	//std::cerr << info[5] << std::endl;
	//std::cerr << info[6] << std::endl;
	//std::cerr << info[7] << std::endl;
	//std::cerr << info[8] << std::endl;
	//std::cerr << info[9] << std::endl;
	mutex.unlock();

	setOutput();
}

