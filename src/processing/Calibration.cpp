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

#include <QtCore>
#include <math.h>

using namespace xma;

int Calibration::nbInstances = 0;

Calibration::Calibration(int camera):QObject(){
	nbInstances++;
	m_camera = camera;
	intrinsic_matrix.create( 3, 3, CV_64F );
	distortion_coeffs.create( 8, 1, CV_64F );

	for( int i = 0; i < 3; ++i ){
		for( int j = 0; j < 3; ++j ){
			intrinsic_matrix.at<double>(i,j) = Project::getInstance()->getCameras()[m_camera]->getCameraMatrix().at<double>(i,j);
		}
	}
	intrinsic_matrix.at<double>(0,1) = 0;

	for( int i = 0; i < 8; ++i ){
		distortion_coeffs.at<double>(i,0) = 0;
	}
	
	for(unsigned int f = 0 ; f < Project::getInstance()->getNbImagesCalibration(); f ++){
		if(Project::getInstance()->getCameras()[m_camera]->getCalibrationImages()[f]->isCalibrated() > 0){
			std::vector<cv::Point3f>pt3D_tmp;
			std::vector<cv::Point2f>pt2D_tmp;

			for(unsigned int k = 0; k < CalibrationObject::getInstance()->getFrameSpecifications().size(); k++){
				if(Project::getInstance()->getCameras()[m_camera]->getCalibrationImages()[f]->getInliers()[k] > 0){
					pt3D_tmp.push_back(cv::Point3f(CalibrationObject::getInstance()->getFrameSpecifications()[k].x,CalibrationObject::getInstance()->getFrameSpecifications()[k].y,
						CalibrationObject::getInstance()->getFrameSpecifications()[k].z));
					pt2D_tmp.push_back(cv::Point2f(Project::getInstance()->getCameras()[m_camera]->getCalibrationImages()[f]->getDetectedPointsUndistorted()[k].x,
						Project::getInstance()->getCameras()[m_camera]->getCalibrationImages()[f]->getDetectedPointsUndistorted()[k].y));
				}
			}

			object_points.push_back(pt3D_tmp);
			image_points.push_back(pt2D_tmp);
		}
	}
}

Calibration::~Calibration(){
	intrinsic_matrix.release();
	distortion_coeffs.release();

	for(unsigned int i = 0; i < object_points.size(); i++) object_points[i].clear();
	object_points.clear();

	for(unsigned int i = 0; i < image_points.size(); i++) image_points[i].clear();
	image_points.clear();
}

void Calibration::computeCameraPosesAndCam(){
	m_FutureWatcher = new QFutureWatcher<void>();
	connect( m_FutureWatcher, SIGNAL( finished() ), this, SLOT( computeCameraPosesAndCam_threadFinished() ) );

	QFuture<void> future = QtConcurrent::run( this, &Calibration::computeCameraPosesAndCam_thread);
	m_FutureWatcher->setFuture( future );
	ProgressDialog::getInstance()->showProgressbar(0, 0, "Calibrate camera internal parameters and pose for all frames");
}

void Calibration::computeCameraPosesAndCam_threadFinished(){
	Project::getInstance()->getCameras()[m_camera]->setRecalibrationRequired(0);
	Project::getInstance()->getCameras()[m_camera]->setUpdateInfoRequired(true);

	delete m_FutureWatcher;
	nbInstances--;
	MainWindow::getInstance()->redrawGL();
	if(nbInstances == 0){
		ProgressDialog::getInstance()->closeProgressbar();
		emit computeCameraPosesAndCam_finished();
	}
	delete this;


}

void Calibration::computeCameraPosesAndCam_thread(){
	try
	{
		int flags =  CV_CALIB_USE_INTRINSIC_GUESS+CV_CALIB_FIX_K1 + CV_CALIB_FIX_K2 +CV_CALIB_FIX_K3 + CV_CALIB_ZERO_TANGENT_DIST;
		double calib_error = cv::calibrateCamera(object_points, image_points, cv::Size(Project::getInstance()->getCameras()[m_camera]->getWidth(),Project::getInstance()->getCameras()[m_camera]->getHeight()), intrinsic_matrix, distortion_coeffs, rvecs, tvecs, flags);
		
		Project::getInstance()->getCameras()[m_camera]->setCameraMatrix(intrinsic_matrix);

		cv::Mat rotationvector;
		cv::Mat translationvector;
		rotationvector.create(3,1,CV_64F);
		translationvector.create(3,1,CV_64F);
		int count = 0;
		for(unsigned int f = 0 ; f < Project::getInstance()->getNbImagesCalibration(); f ++){
			if(Project::getInstance()->getCameras()[m_camera]->getCalibrationImages()[f]->isCalibrated() > 0){
				for(int i = 0 ; i < 3 ; i++)
					rotationvector.at<double>(i,0) = rvecs[count].at<double>(i,0);

				for(unsigned int i = 0 ; i < 3 ; i++)
					translationvector.at<double>(i,0) = tvecs[count].at<double>(i,0);

				count++;
				Project::getInstance()->getCameras()[m_camera]->getCalibrationImages()[f]->setMatrices(rotationvector,translationvector);	
			}
		}
		rotationvector.release();
		translationvector.release();

		reproject();
		
	}catch(std::exception e){
		fprintf(stderr, "Camera::calibrateCamera : cv::calibrateCamera Failed with Exception - %s\n",e.what());
	}
}


void Calibration::reproject(){
	CvMat* object_points2 = cvCreateMat( CalibrationObject::getInstance()->getFrameSpecifications().size(), 3, CV_64FC1 );
	CvMat* image_points2 = cvCreateMat( CalibrationObject::getInstance()->getFrameSpecifications().size(), 2, CV_64FC1 );

	// Transfer the points into the correct size matrices
	for(unsigned int i = 0; i <  CalibrationObject::getInstance()->getFrameSpecifications().size(); ++i ){
		CV_MAT_ELEM( *object_points2, double, i, 0) = CalibrationObject::getInstance()->getFrameSpecifications()[i].x;
		CV_MAT_ELEM( *object_points2, double, i, 1) = CalibrationObject::getInstance()->getFrameSpecifications()[i].y;
		CV_MAT_ELEM( *object_points2, double, i, 2) = CalibrationObject::getInstance()->getFrameSpecifications()[i].z;
	}

	// initialize camera and distortion initial guess
	CvMat* intrinsic_matrix2	= cvCreateMat( 3, 3, CV_64FC1 );
	CvMat* distortion_coeffs2	= cvCreateMat( 5, 1, CV_64FC1 );
	for(unsigned int i = 0; i < 3; ++i ){
		CV_MAT_ELEM( *intrinsic_matrix2, double, i, 0) = intrinsic_matrix.at<double>(i,0);
		CV_MAT_ELEM( *intrinsic_matrix2, double, i, 1) = intrinsic_matrix.at<double>(i,1);
		CV_MAT_ELEM( *intrinsic_matrix2, double, i, 2) = intrinsic_matrix.at<double>(i,2);
	}
	CV_MAT_ELEM( *intrinsic_matrix2, double, 0, 1) = 0;

	for(unsigned int i = 0; i < 5; ++i ){
		CV_MAT_ELEM( *distortion_coeffs2, double, i, 0) = distortion_coeffs.at<double>(i,0);
	}

	CvMat* r_matrices = cvCreateMat( 1, 1, CV_64FC3 );
	CvMat* t_matrices = cvCreateMat( 1, 1, CV_64FC3 );
	cv::vector<cv::Point2d> projectedPoints;

	for(unsigned int f = 0 ; f < Project::getInstance()->getNbImagesCalibration(); f ++){
		if(Project::getInstance()->getCameras()[m_camera]->getCalibrationImages()[f]->isCalibrated() > 0){
			projectedPoints.clear();

			for(unsigned int i= 0; i < 3 ; i++){
				CV_MAT_ELEM( *r_matrices, cv::Vec3d, 0, 0)[i] = Project::getInstance()->getCameras()[m_camera]->getCalibrationImages()[f]->getRotationVector().at<double>(i,0);
				CV_MAT_ELEM( *t_matrices, cv::Vec3d, 0, 0)[i] = Project::getInstance()->getCameras()[m_camera]->getCalibrationImages()[f]->getTranslationVector().at<double>(i,0);
			}
			try{
				cvProjectPoints2(object_points2,r_matrices,t_matrices,intrinsic_matrix2,distortion_coeffs2,image_points2);

				projectedPoints.clear();
				for (unsigned int i = 0 ; i <  CalibrationObject::getInstance()->getFrameSpecifications().size() ; i++){
					cv::Point2d pt = cv::Point2d(CV_MAT_ELEM( *image_points2, double, i, 0), CV_MAT_ELEM( *image_points2, double, i, 1));
					projectedPoints.push_back(pt);
				}	
				Project::getInstance()->getCameras()[m_camera]->getCalibrationImages()[f]->setPointsProjectedUndistorted(projectedPoints);

			}catch(std::exception e){
				fprintf(stderr, "Frame::reprojectAndComputeError : cvProjectPoints2 Failed with Exception - %s\n",e.what());
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