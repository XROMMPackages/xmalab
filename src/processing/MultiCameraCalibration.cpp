#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "processing/MultiCameraCalibration.h"	

#include "ui/ProgressDialog.h"
#include "ui/MainWindow.h"

#include "core/Project.h"
#include "core/Camera.h"
#include "core/CalibrationImage.h"
#include "core/UndistortionObject.h"
#include "core/CalibrationObject.h"

#include <QtCore>
#include <math.h>

#include "levmar.h"

using namespace xma;

#define DEBUG


int MultiCameraCalibration::nbInstances = 0;


void ros(double *p, double *x, int m, int n, void *data)
{
	MultiCameraCalibration *   calib_data = static_cast <MultiCameraCalibration *>(data);
	int step = calib_data->getStep();
	for (int i = 0; i < n  / step; i++)
	{
		calib_data->projError(i, p, &x[i*step]);
	}
}
void jacros(double *p, double *jac, int m, int n, void *data)
{
	MultiCameraCalibration *   calib_data = static_cast <MultiCameraCalibration *>(data);
	int step = calib_data->getStep();
	for (int i = 0; i < n / step; i++)
	{
		calib_data->projErrorJac(i, m, p, &jac[i*m*step]);
	}
}

void MultiCameraCalibration::projErrorJac(int n, int m, double * p, double * Jac)
{
	double RxCam = p[nbCamParams * camIdx[n] + 0];
	double RyCam = p[nbCamParams * camIdx[n] + 1];
	double RzCam = p[nbCamParams * camIdx[n] + 2];
	double txCam = p[nbCamParams * camIdx[n] + 3];
	double tyCam = p[nbCamParams * camIdx[n] + 4];
	double tzCam = p[nbCamParams * camIdx[n] + 5];
	double fx = p[nbCamParams * camIdx[n] + 6];
	double fy = p[nbCamParams * camIdx[n] + 7];
	double cx = p[nbCamParams * camIdx[n] + 8];
	double cy = p[nbCamParams * camIdx[n] + 9];

	double k1, k2, p1, p2, k3;
	if (withDistortion)
	{
		k1 = p[nbCamParams * camIdx[n] + 10];
		k2 = p[nbCamParams * camIdx[n] + 11];
		p1 = p[nbCamParams * camIdx[n] + 12];
		p2 = p[nbCamParams * camIdx[n] + 13];
		k3 = p[nbCamParams * camIdx[n] + 14];
	}

	double RxChess = p[6 * frameIdx[n] + nbCamParams * nbCameras + 0];
	double RyChess = p[6 * frameIdx[n] + nbCamParams * nbCameras + 1];
	double RzChess = p[6 * frameIdx[n] + nbCamParams * nbCameras + 2];
	double txChess = p[6 * frameIdx[n] + nbCamParams * nbCameras + 3];
	double tyChess = p[6 * frameIdx[n] + nbCamParams * nbCameras + 4];
	double tzChess = p[6 * frameIdx[n] + nbCamParams * nbCameras + 5];

	double X = Pts3D[pts3DIdx[n]].x;
	double Y = Pts3D[pts3DIdx[n]].y;
	double Z = Pts3D[pts3DIdx[n]].z;

	double u = Pts2D[n].x;
	double v = Pts2D[n].y;

	if (!seperateDimensions){
		double A0[21];
		for (int i = 0; i < m; i++)
		{
			Jac[i] = 0;
		}
		if (withDistortion)
		{
#include "processing/optimizationFuncs/jacFuncWithDistortion.h"
		}
		else{
#include "processing/optimizationFuncs/jacFuncNoDistortion.h"
		}

		Jac[nbCamParams * camIdx[n] + 0] = A0[0];
		Jac[nbCamParams * camIdx[n] + 1] = A0[1];
		Jac[nbCamParams * camIdx[n] + 2] = A0[2];
		Jac[nbCamParams * camIdx[n] + 3] = A0[3];
		Jac[nbCamParams * camIdx[n] + 4] = A0[4];
		Jac[nbCamParams * camIdx[n] + 5] = A0[5];
		Jac[nbCamParams * camIdx[n] + 6] = A0[6];
		Jac[nbCamParams * camIdx[n] + 7] = A0[7];
		Jac[nbCamParams * camIdx[n] + 8] = A0[8];
		Jac[nbCamParams * camIdx[n] + 9] = A0[9];

		if (withDistortion)
		{
			Jac[nbCamParams * camIdx[n] + 10] = A0[10];
			Jac[nbCamParams * camIdx[n] + 11] = A0[11];
			Jac[nbCamParams * camIdx[n] + 12] = A0[12];
			Jac[nbCamParams * camIdx[n] + 13] = A0[13];
			Jac[nbCamParams * camIdx[n] + 14] = A0[14];
		}

		Jac[6 * frameIdx[n] + nbCamParams * nbCameras + 0] = A0[nbCamParams];
		Jac[6 * frameIdx[n] + nbCamParams * nbCameras + 1] = A0[nbCamParams + 1];
		Jac[6 * frameIdx[n] + nbCamParams * nbCameras + 2] = A0[nbCamParams + 2];
		Jac[6 * frameIdx[n] + nbCamParams * nbCameras + 3] = A0[nbCamParams + 3];
		Jac[6 * frameIdx[n] + nbCamParams * nbCameras + 4] = A0[nbCamParams + 4];
		Jac[6 * frameIdx[n] + nbCamParams * nbCameras + 5] = A0[nbCamParams + 5];

	}
	else
	{
		for (int i = 0; i < 2*m; i++)
		{
			Jac[i] = 0;
		}
		double A0[2][16];

#include "processing/optimizationFuncs/jacFuncNoDistortionSeperateDimensions.h"

		for (int i = 0; i < 2; i++){
			Jac[m*i + nbCamParams * camIdx[n] + 0] = A0[i][0];
			Jac[m*i + nbCamParams * camIdx[n] + 1] = A0[i][1];
			Jac[m*i + nbCamParams * camIdx[n] + 2] = A0[i][2];
			Jac[m*i + nbCamParams * camIdx[n] + 3] = A0[i][3];
			Jac[m*i + nbCamParams * camIdx[n] + 4] = A0[i][4];
			Jac[m*i + nbCamParams * camIdx[n] + 5] = A0[i][5];
			Jac[m*i + nbCamParams * camIdx[n] + 6] = A0[i][6];
			Jac[m*i + nbCamParams * camIdx[n] + 7] = A0[i][7];
			Jac[m*i + nbCamParams * camIdx[n] + 8] = A0[i][8];
			Jac[m*i + nbCamParams * camIdx[n] + 9] = A0[i][9];

			Jac[m*i + 6 * frameIdx[n] + nbCamParams * nbCameras + 0] = A0[i][nbCamParams];
			Jac[m*i + 6 * frameIdx[n] + nbCamParams * nbCameras + 1] = A0[i][nbCamParams + 1];
			Jac[m*i + 6 * frameIdx[n] + nbCamParams * nbCameras + 2] = A0[i][nbCamParams + 2];
			Jac[m*i + 6 * frameIdx[n] + nbCamParams * nbCameras + 3] = A0[i][nbCamParams + 3];
			Jac[m*i + 6 * frameIdx[n] + nbCamParams * nbCameras + 4] = A0[i][nbCamParams + 4];
			Jac[m*i + 6 * frameIdx[n] + nbCamParams * nbCameras + 5] = A0[i][nbCamParams + 5];
		}
	}
}

int MultiCameraCalibration::getStep()
{
	return (seperateDimensions) ? 2 : 1; 
}

void MultiCameraCalibration::projError(int n, double * p,  double * x)
{
	double RxCam = p[nbCamParams * camIdx[n] + 0];
	double RyCam = p[nbCamParams * camIdx[n] + 1];
	double RzCam = p[nbCamParams * camIdx[n] + 2];
	double txCam = p[nbCamParams * camIdx[n] + 3];
	double tyCam = p[nbCamParams * camIdx[n] + 4];
	double tzCam = p[nbCamParams * camIdx[n] + 5];
	double fx    = p[nbCamParams * camIdx[n] + 6];
	double fy    = p[nbCamParams * camIdx[n] + 7];
	double cx    = p[nbCamParams * camIdx[n] + 8];
	double cy    = p[nbCamParams * camIdx[n] + 9];

	double k1, k2, p1, p2, k3;
	if (withDistortion)
	{
		k1 = p[nbCamParams * camIdx[n] + 10];
		k2 = p[nbCamParams * camIdx[n] + 11];
		p1 = p[nbCamParams * camIdx[n] + 12];
		p2 = p[nbCamParams * camIdx[n] + 13];
		k3 = p[nbCamParams * camIdx[n] + 14];
	}

	double RxChess = p[6 * frameIdx[n] + nbCamParams * nbCameras + 0];
	double RyChess = p[6 * frameIdx[n] + nbCamParams * nbCameras + 1];
	double RzChess = p[6 * frameIdx[n] + nbCamParams * nbCameras + 2];
	double txChess = p[6 * frameIdx[n] + nbCamParams * nbCameras + 3];
	double tyChess = p[6 * frameIdx[n] + nbCamParams * nbCameras + 4];
	double tzChess = p[6 * frameIdx[n] + nbCamParams * nbCameras + 5];

	double X = Pts3D[pts3DIdx[n]].x;
	double Y = Pts3D[pts3DIdx[n]].y;
	double Z = Pts3D[pts3DIdx[n]].z;

	double u = Pts2D[n].x;
	double v = Pts2D[n].y;

	if (!seperateDimensions){
		if (withDistortion)
		{
#include "processing/optimizationFuncs/projFuncWithDistortion.h"
			x[0] = t0;
		}
		else{
#include "processing/optimizationFuncs/projFuncNoDistortion.h"
			x[0] = t0;
		}
	}
	else
	{
		double A0[2];
#include "processing/optimizationFuncs/projFuncNoDistortionSeperateDimensions.h"
		x[0] = A0[0];
		x[1] = A0[1];
	}
}



MultiCameraCalibration::MultiCameraCalibration(int method, int iterations, double initial):QObject()
		,m_method(method),m_iterations(iterations),m_initial(initial){
	nbInstances++;

	switch (m_method)
	{
	default:
	case 0:
		withDistortion = false;
		seperateDimensions = false;
		break;
	case 1:
		withDistortion = false;
		seperateDimensions = true;
		break;
	case 2:
		withDistortion = true;
		seperateDimensions = false;
		break;
	}

	if (withDistortion){
		std::cerr << "WithDistortion " << m_method << std::endl;
		nbCamParams = 15;
	}
	else
	{
		std::cerr << "NoDistortion " << m_method << std::endl;
		nbCamParams = 10;
	}
	nbCameras = Project::getInstance()->getCameras().size();
	nbFrames = Project::getInstance()->getNbImagesCalibration();
	nbPoints = 0;
	for (unsigned int c = 0; c < nbCameras; c++){
		for (unsigned int f = 0; f < nbFrames; f++){
			if (Project::getInstance()->getCameras()[c]->getCalibrationImages()[f]->isCalibrated() > 0){
				for (unsigned int k = 0; k < CalibrationObject::getInstance()->getFrameSpecifications().size(); k++){
					if (Project::getInstance()->getCameras()[c]->getCalibrationImages()[f]->getInliers()[k] > 0){
						frameIdx.push_back(f);
						camIdx.push_back(c);
						pts3DIdx.push_back(k);
						cv::Point2d pt = cv::Point2d(Project::getInstance()->getCameras()[c]->getCalibrationImages()[f]->getDetectedPoints()[k]);
						if (Project::getInstance()->getCameras()[c]->getUndistortionObject() && Project::getInstance()->getCameras()[c]->getUndistortionObject()->isComputed())
						{
							pt = Project::getInstance()->getCameras()[c]->getUndistortionObject()->transformPoint(pt, true);
						}
						Pts2D.push_back(pt);
						nbPoints++;
					}
				}
			}
		}
	}

	nbPoints = nbPoints * getStep();
	
	for (unsigned int k = 0; k < CalibrationObject::getInstance()->getFrameSpecifications().size(); k++){
		Pts3D.push_back(cv::Point3d(CalibrationObject::getInstance()->getFrameSpecifications()[k]));
	}

	//Set initial guess
	nbParams = nbCamParams * nbCameras + 6 * nbFrames;
	p = new double[nbParams];
	refIdx = -1;
	for (unsigned int f = 0; f < nbFrames; f++){
		int count = 0;
		for (unsigned int c = 0; c< nbCameras; c++){
			if (Project::getInstance()->getCameras()[c]->getCalibrationImages()[f]->isCalibrated() > 0){
				count++;
			}
		}
		if (count == nbCameras)
		{
			refIdx = f;
			break;
		};
	}

	if (refIdx >= 0)
	{
		for (unsigned int c = 0; c < nbCameras; c++){
			//We have to rotateCameras to make sure rotationmatrices are not identity matrix
			cv::Mat rot_mat;
			cv::Mat r_tmp = (cv::Mat_<double>(3, 1) << 0.1, 0.1, 0.1);
			cv::Rodrigues(r_tmp, rot_mat);
			cv::Mat rot_mat2;
			cv::Rodrigues(cv::Mat(Project::getInstance()->getCameras()[c]->getCalibrationImages()[refIdx]->getRotationVector()), rot_mat2);
			cv::Mat Rot_vec;
			Rodrigues(rot_mat2 * rot_mat, Rot_vec);
			
			cameraRotationVector.push_back(Rot_vec);
			cameraTranslationVector.push_back(cv::Mat(Project::getInstance()->getCameras()[c]->getCalibrationImages()[refIdx]->getTranslationVector()));
			cameraMatrix.push_back(cv::Mat(Project::getInstance()->getCameras()[c]->getCameraMatrix()));
			if (withDistortion)
			{
				distortion.push_back(cv::Mat(Project::getInstance()->getCameras()[c]->getDistortionCoefficiants()));
			}
		}
	}


	for (unsigned int f = 0; f < nbFrames; f++){
		cv::Mat r_vec;
		r_vec = cv::Mat::zeros(3, 1, CV_64F);
		cv::Mat t_vec;
		t_vec = cv::Mat::zeros(3, 1, CV_64F);
		int count = 0;
		for (unsigned int c = 0; c< nbCameras; c++){
			if (Project::getInstance()->getCameras()[c]->getCalibrationImages()[f]->isCalibrated() > 0){
				cv::Mat t_cam = getTransformationMatrix(cameraRotationVector[c], cameraTranslationVector[c]);
				cv::Mat t_chess = getTransformationMatrix(Project::getInstance()->getCameras()[c]->getCalibrationImages()[f]->getRotationVector(), Project::getInstance()->getCameras()[c]->getCalibrationImages()[f]->getTranslationVector());
				r_vec += getRotationVector(t_cam.inv() * t_chess);
				t_vec += getTranslation(t_cam.inv() * t_chess);
				count++;
			}
		}
		chessRotationVector.push_back(r_vec / count);
		chessTranslationVector.push_back(t_vec / count);
	}

#ifdef DEBUG
	std::cerr << "cameraRotationVector" << std::endl;
	for (int i = 0; i < cameraRotationVector.size();i++)
	{
		std::cerr << cameraRotationVector[i] << std::endl;
	}
	std::cerr << "cameraTranslationVector" << std::endl;
	for (int i = 0; i < cameraTranslationVector.size(); i++)
	{
		std::cerr << cameraTranslationVector[i] << std::endl;
	}
	std::cerr << "chessRotationVector" << std::endl;
	for (int i = 0; i < chessRotationVector.size(); i++)
	{
		std::cerr << chessRotationVector[i] << std::endl;
	}
	std::cerr << "chessTranslationVector" << std::endl;
	for (int i = 0; i < chessTranslationVector.size(); i++)
	{
		std::cerr << chessTranslationVector[i] << std::endl;
	}
#endif
	//Fill param vector
	//Cameras
	for (unsigned int c = 0; c < nbCameras; c++){
		//Rotation
		p[nbCamParams * c + 0] = cameraRotationVector[c].at<double>(0, 0);
		p[nbCamParams * c + 1] = cameraRotationVector[c].at<double>(1, 0);
		p[nbCamParams * c + 2] = cameraRotationVector[c].at<double>(2, 0);
		//Translation
		p[nbCamParams * c + 3] = cameraTranslationVector[c].at<double>(0, 0);
		p[nbCamParams * c + 4] = cameraTranslationVector[c].at<double>(1, 0);
		p[nbCamParams * c + 5] = cameraTranslationVector[c].at<double>(2, 0);
		//Focal
		p[nbCamParams * c + 6] = cameraMatrix[c].at<double>(0, 0);
		p[nbCamParams * c + 7] = cameraMatrix[c].at<double>(1, 1);
		//center
		p[nbCamParams * c + 8] = cameraMatrix[c].at<double>(0, 2);
		p[nbCamParams * c + 9] = cameraMatrix[c].at<double>(1, 2);

		if (withDistortion)
		{
			p[nbCamParams * c + 10] = distortion[c].at<double>(0, 0);
			p[nbCamParams * c + 11] = distortion[c].at<double>(1, 0);
			p[nbCamParams * c + 12] = distortion[c].at<double>(2, 0);
			p[nbCamParams * c + 13] = distortion[c].at<double>(3, 0);
			p[nbCamParams * c + 14] = distortion[c].at<double>(4, 0);
		}
	}

	for (unsigned int f = 0; f < nbFrames; f++){
		//Rotation
		p[6 * f + nbCamParams * nbCameras + 0] = chessRotationVector[f].at<double>(0, 0);
		p[6 * f + nbCamParams * nbCameras + 1] = chessRotationVector[f].at<double>(1, 0);
		p[6 * f + nbCamParams * nbCameras + 2] = chessRotationVector[f].at<double>(2, 0);

		//Translation
		p[6 * f + nbCamParams * nbCameras + 3] = chessTranslationVector[f].at<double>(0, 0);
		p[6 * f + nbCamParams * nbCameras + 4] = chessTranslationVector[f].at<double>(1, 0);
		p[6 * f + nbCamParams * nbCameras + 5] = chessTranslationVector[f].at<double>(2, 0);
	}


#ifdef DEBUG
	std::cerr << "p" << std::endl;
	for (unsigned int i = 0; i < nbParams; i++){
		std::cerr << p[i] << std::endl;
	}
	std::cerr << "p" << std::endl;
#endif

	//set error vector;
	std::cerr << nbPoints << std::endl;
	x = new double[nbPoints];
#ifdef DEBUG
	ros(p, x, nbParams, nbPoints, this);
	std::cerr << "x" << std::endl;
	double mean = 0;
	for (unsigned int i = 0; i < nbPoints; i++){
		std::cerr << x[i] << std::endl;
		mean += fabs(x[i]);
	}
	std::cerr << mean/ nbPoints << std::endl;
#endif

	for (unsigned int i = 0; i < nbPoints; i++)
	{
		x[i] = 0.0;
	}
}

MultiCameraCalibration::~MultiCameraCalibration(){
	delete[] x;
	delete[] p;
}

cv::Mat MultiCameraCalibration::getTransformationMatrix(cv::Mat rotationvector, cv::Mat translationvector){
	cv::Mat trans;
	trans.create(4, 4, CV_64FC1);
	cv::Mat rotationmatrix;

	//set R
	cv::Rodrigues(rotationvector, rotationmatrix);

	for (unsigned int i = 0; i < 3; ++i){
		for (unsigned int j = 0; j < 3; ++j){
			trans.at<double>(i, j) = rotationmatrix.at<double>(i, j);
		}
	}
	//set t
	for (unsigned int j = 0; j < 3; ++j){
		trans.at<double>(j, 3) = translationvector.at<double>(j, 0);
	}

	for (unsigned int j = 0; j < 3; ++j){
		trans.at<double>(3, j) = 0;
	}
	trans.at<double>(3, 3) = 1;

	return trans;
}

cv::Mat MultiCameraCalibration::getRotationVector(cv::Mat trans){
	cv::Mat rotationmatrix;
	rotationmatrix.create(3, 3, CV_64FC1);
	
	cv::Mat rotationvector;

	for (unsigned int i = 0; i < 3; ++i){
		for (unsigned int j = 0; j < 3; ++j){
			rotationmatrix.at<double>(i, j) = trans.at<double>(i, j);
		}
	}

	cv::Rodrigues(rotationmatrix, rotationvector);

	return rotationvector;
}

cv::Mat MultiCameraCalibration::getTranslation(cv::Mat trans){
	cv::Mat translationvector;
	translationvector.create(3, 1, CV_64FC1);

	for (unsigned int j = 0; j < 3; ++j){
		translationvector.at<double>(j, 0) = trans.at<double>(j, 3);
	}

	return translationvector;
}

void MultiCameraCalibration::optimizeCameraSetup(){
	m_FutureWatcher = new QFutureWatcher<void>();
	connect(m_FutureWatcher, SIGNAL(finished()), this, SLOT(optimizeCameraSetup_threadFinished()));

	QFuture<void> future = QtConcurrent::run(this, &MultiCameraCalibration::optimizeCameraSetup_thread);
	m_FutureWatcher->setFuture( future );
	ProgressDialog::getInstance()->showProgressbar(0, 0, "Optimize camera setup");
}

void MultiCameraCalibration::optimizeCameraSetup_threadFinished(){

	for (unsigned int c = 0; c < nbCameras; c++){
		//Rotation
		cameraRotationVector[c].at<double>(0, 0) = p[nbCamParams * c + 0];
		cameraRotationVector[c].at<double>(1, 0) = p[nbCamParams * c + 1];
		cameraRotationVector[c].at<double>(2, 0) = p[nbCamParams * c + 2];
		//Translation
		cameraTranslationVector[c].at<double>(0, 0) = p[nbCamParams * c + 3];
		cameraTranslationVector[c].at<double>(1, 0) = p[nbCamParams * c + 4];
		cameraTranslationVector[c].at<double>(2, 0) = p[nbCamParams * c + 5];
		//Focal
		cameraMatrix[c].at<double>(0, 0) = p[nbCamParams * c + 6];
		cameraMatrix[c].at<double>(1, 1) = p[nbCamParams * c + 7];
		//center
		cameraMatrix[c].at<double>(0, 2) = p[nbCamParams * c + 8];
		cameraMatrix[c].at<double>(1, 2) = p[nbCamParams * c + 9];

		if (withDistortion)
		{
			distortion[c].at<double>(0, 0) = p[nbCamParams * c + 10];
			distortion[c].at<double>(1, 0) = p[nbCamParams * c + 11];
			distortion[c].at<double>(2, 0) = p[nbCamParams * c + 12];
			distortion[c].at<double>(3, 0) = p[nbCamParams * c + 13];
			distortion[c].at<double>(4, 0) = p[nbCamParams * c + 14];
			Project::getInstance()->getCameras()[c]->setDistortionCoefficiants(distortion[c]);
		}
		else
		{
			Project::getInstance()->getCameras()[c]->resetDistortion();
		}

		Project::getInstance()->getCameras()[c]->setCameraMatrix(cameraMatrix[c]);
		Project::getInstance()->getCameras()[c]->setUpdateInfoRequired(true);
		Project::getInstance()->getCameras()[c]->setRecalibrationRequired(false);
		Project::getInstance()->getCameras()[c]->setOptimized(true);
	}

	for (unsigned int f = 0; f < nbFrames; f++){
		//Rotation
		chessRotationVector[f].at<double>(0, 0) = p[6 * f + nbCamParams * nbCameras + 0];
		chessRotationVector[f].at<double>(1, 0) = p[6 * f + nbCamParams * nbCameras + 1];
		chessRotationVector[f].at<double>(2, 0) = p[6 * f + nbCamParams * nbCameras + 2];

		//Translation
		chessTranslationVector[f].at<double>(0, 0) = p[6 * f + nbCamParams * nbCameras + 3];
		chessTranslationVector[f].at<double>(1, 0) = p[6 * f + nbCamParams * nbCameras + 4];
		chessTranslationVector[f].at<double>(2, 0) = p[6 * f + nbCamParams * nbCameras + 5];

		cv::Mat t_chess = getTransformationMatrix(chessRotationVector[f], chessTranslationVector[f]);

		for (unsigned int c = 0; c < nbCameras; c++){
			if (Project::getInstance()->getCameras()[c]->getCalibrationImages()[f]->isCalibrated() > 0){
				cv::Mat t_cam = getTransformationMatrix(cameraRotationVector[c], cameraTranslationVector[c]);
                cv::Mat r_tmp = getRotationVector(t_cam * t_chess);
                cv::Mat t_tmp = getTranslation(t_cam * t_chess);
                
				Project::getInstance()->getCameras()[c]->getCalibrationImages()[f]->setMatrices(r_tmp,t_tmp);
			}
		}
	}
	for (unsigned int c = 0; c < nbCameras; c++){
		reproject(c);
	}

	delete m_FutureWatcher;
	nbInstances--;
	MainWindow::getInstance()->redrawGL();
	if(nbInstances == 0){
		ProgressDialog::getInstance()->closeProgressbar();
		emit optimizeCameraSetup_finished();
	}
	delete this;
}


void MultiCameraCalibration::optimizeCameraSetup_thread(){
	double opts[LM_OPTS_SZ], info[LM_INFO_SZ];
	opts[0] = m_initial; opts[1] = 1E-30; opts[2] = 1E-30; opts[3] = 1E-40;
	opts[4] = LM_DIFF_DELTA; // relevant only if the Jacobian is approximated using finite differences; specifies forward differencing 

	int ret = dlevmar_der(ros, jacros, p, x, nbParams, nbPoints, m_iterations, opts, info, NULL, NULL, this); // with analytic Jacobian
	//int ret = dlevmar_dif(ros, p, x, nbParams, nbPoints, 10000, opts, info, NULL, NULL, this);
	printf("Levenberg-Marquardt returned %d in %g iter, reason %g ", ret, info[5], info[6]);
	printf("\n\nMinimization info:");
	for (int i = 0; i < LM_INFO_SZ; ++i)
		printf("%g ", info[i]);
	printf("\n");

#ifdef DEBUG
	ros(p, x, nbParams, nbPoints, this);
	for (unsigned int i = 0; i < nbParams; i++){
		std::cerr << p[i] << std::endl;
	}
	double mean = 0;
	for (unsigned int i = 0; i < nbPoints; i++){
		//std::cerr << x[i] << std::endl;
		mean += fabs(x[i]);
	}
	std::cerr << "NewMean : " << mean / nbPoints << std::endl;
#endif
}


void MultiCameraCalibration::reproject(int c){

	CvMat* object_points2 = cvCreateMat(CalibrationObject::getInstance()->getFrameSpecifications().size(), 3, CV_64FC1);
	CvMat* image_points2 = cvCreateMat(CalibrationObject::getInstance()->getFrameSpecifications().size(), 2, CV_64FC1);

	// Transfer the points into the correct size matrices
	for (unsigned int i = 0; i < CalibrationObject::getInstance()->getFrameSpecifications().size(); ++i){
		CV_MAT_ELEM(*object_points2, double, i, 0) = CalibrationObject::getInstance()->getFrameSpecifications()[i].x;
		CV_MAT_ELEM(*object_points2, double, i, 1) = CalibrationObject::getInstance()->getFrameSpecifications()[i].y;
		CV_MAT_ELEM(*object_points2, double, i, 2) = CalibrationObject::getInstance()->getFrameSpecifications()[i].z;
	}

	// initialize camera and distortion initial guess
	CvMat* intrinsic_matrix2 = cvCreateMat(3, 3, CV_64FC1);
	CvMat* distortion_coeffs2 = cvCreateMat(8, 1, CV_64FC1);

	for (unsigned int i = 0; i < 3; ++i){
		CV_MAT_ELEM(*intrinsic_matrix2, double, i, 0) = Project::getInstance()->getCameras()[c]->getCameraMatrix().at<double>(i, 0);
		CV_MAT_ELEM(*intrinsic_matrix2, double, i, 1) = Project::getInstance()->getCameras()[c]->getCameraMatrix().at<double>(i, 1);
		CV_MAT_ELEM(*intrinsic_matrix2, double, i, 2) = Project::getInstance()->getCameras()[c]->getCameraMatrix().at<double>(i, 2);
	}

	//We save the  undistorted points
	for (unsigned int i = 0; i < 8; ++i){
		CV_MAT_ELEM(*distortion_coeffs2, double, i, 0) = 0;
	}

	CvMat* r_matrices = cvCreateMat(1, 1, CV_64FC3);
	CvMat* t_matrices = cvCreateMat(1, 1, CV_64FC3);
	cv::vector<cv::Point2d> projectedPoints;
	for (unsigned int f = 0; f < Project::getInstance()->getNbImagesCalibration(); f++){
		if (Project::getInstance()->getCameras()[c]->getCalibrationImages()[f]->isCalibrated() > 0){
			projectedPoints.clear();
			for (unsigned int i = 0; i < 3; i++){
				CV_MAT_ELEM(*r_matrices, cv::Vec3d, 0, 0)[i] = Project::getInstance()->getCameras()[c]->getCalibrationImages()[f]->getRotationVector().at<double>(i, 0);
				CV_MAT_ELEM(*t_matrices, cv::Vec3d, 0, 0)[i] = Project::getInstance()->getCameras()[c]->getCalibrationImages()[f]->getTranslationVector().at<double>(i, 0);
			}
			try{
				cvProjectPoints2(object_points2, r_matrices, t_matrices, intrinsic_matrix2, distortion_coeffs2, image_points2);
				projectedPoints.clear();
				for (unsigned int i = 0; i < CalibrationObject::getInstance()->getFrameSpecifications().size(); i++){
					cv::Point2d pt = cv::Point2d(CV_MAT_ELEM(*image_points2, double, i, 0), CV_MAT_ELEM(*image_points2, double, i, 1));
					projectedPoints.push_back(pt);
				}
				Project::getInstance()->getCameras()[c]->getCalibrationImages()[f]->setPointsProjectedUndistorted(projectedPoints);

			}
			catch (std::exception e){
				fprintf(stderr, "Frame::reprojectAndComputeError : cvProjectPoints2 Failed with Exception - %s\n", e.what());
			}
		}
	}

	projectedPoints.clear();
	cvReleaseMat(&intrinsic_matrix2);
	cvReleaseMat(&distortion_coeffs2);
	cvReleaseMat(&object_points2);
	cvReleaseMat(&image_points2);
	cvReleaseMat(&r_matrices);
	cvReleaseMat(&t_matrices);
	
}