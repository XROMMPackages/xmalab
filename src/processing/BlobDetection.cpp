#include "processing/BlobDetection.h"	
#include "ui/ProgressDialog.h"
#include "ui/MainWindow.h"
#include "core/Project.h"
#include "core/Camera.h"
#include "core/Image.h"
#include "core/CalibrationImage.h"
#include "core/UndistortionObject.h"
#include "core/Settings.h"
#include <QtCore>

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

int BlobDetection::nbInstances = 0;

BlobDetection::BlobDetection(int camera, int image):QObject(){
	m_camera = camera;
	m_image = image;
}

BlobDetection::~BlobDetection(){

}

void BlobDetection::detectBlobs(){
	m_FutureWatcher = new QFutureWatcher<void>();
	connect( m_FutureWatcher, SIGNAL( finished() ), this, SLOT( detectBlobs_threadFinished() ) );

	QFuture<void> future = QtConcurrent::run( this, &BlobDetection::detectBlobs_thread);
	m_FutureWatcher->setFuture( future );
	nbInstances++;
	ProgressDialog::getInstance()->showProgressbar(0, 0, "Detect Points");
}

void BlobDetection::detectBlobs_thread(){
	tmpPoints.clear();
	cv::Mat image;
	if(m_image < 0){
 		Project::getInstance()->getCameras()[m_camera]->getUndistortionObject()->getImage()->getImage(image);
	}else{
		Project::getInstance()->getCameras()[m_camera]->getCalibrationImages()[m_image]->getImage()->getImage(image);
	}

	cv::SimpleBlobDetector::Params paramsBlob;

	paramsBlob.thresholdStep = Settings::getBlobDetectorThresholdStep();
	paramsBlob.minThreshold = Settings::getBlobDetectorMinThreshold(); 
	paramsBlob.maxThreshold = Settings::getBlobDetectorMaxThreshold();
	paramsBlob.minRepeatability = Settings::getBlobDetectorMinRepeatability();
	paramsBlob.minDistBetweenBlobs = Settings::getBlobDetectorMinDistBetweenBlobs();

	paramsBlob.filterByColor = Settings::getBlobDetectorFilterByColor();
	if(m_image < 0){
		paramsBlob.blobColor = Settings::getBlobDetectorBlobColor();
	}else{
		paramsBlob.blobColor = 255 - Settings::getBlobDetectorBlobColor();
	}

	paramsBlob.filterByArea = Settings::getBlobDetectorFilterByArea();
	paramsBlob.minArea =Settings::getBlobDetectorMinArea();
	paramsBlob.maxArea = Settings::getBlobDetectorMaxArea();

	paramsBlob.filterByCircularity = Settings::getBlobDetectorFilterByCircularity();
	paramsBlob.minCircularity =	Settings::getBlobDetectorMinCircularity();
	paramsBlob.maxCircularity =	Settings::getBlobDetectorMaxCircularity();
				
	paramsBlob.filterByInertia = Settings::getBlobDetectorFilterByInertia();
	paramsBlob.minInertiaRatio = Settings::getBlobDetectorMinInertiaRatio();
	paramsBlob.maxInertiaRatio = Settings::getBlobDetectorMaxInertiaRatio();

	paramsBlob.filterByConvexity =  Settings::getBlobDetectorFilterByConvexity();
	paramsBlob.minConvexity = Settings::getBlobDetectorMinConvexity();
	paramsBlob.maxConvexity= Settings::getBlobDetectorMaxConvexity();

	cv::FeatureDetector * detector = new cv::SimpleBlobDetector(paramsBlob);
	cv::vector<cv::KeyPoint> keypoints;

	detector->detect(image,keypoints);

	for (unsigned int i=0; i<keypoints.size(); i++){
		tmpPoints.push_back(cv::Point2d(keypoints[i].pt.x,keypoints[i].pt.y));
	}
	image.release();
}

void BlobDetection::detectBlobs_threadFinished(){
	if(m_image < 0){
		Project::getInstance()->getCameras()[m_camera]->getUndistortionObject()->setDetectedPoints(tmpPoints);
	}else{
		Project::getInstance()->getCameras()[m_camera]->getCalibrationImages()[m_image]->setDetectedPoints(tmpPoints);
	}

	tmpPoints.clear();
	delete m_FutureWatcher;
	nbInstances--;
	MainWindow::getInstance()->redrawGL();
	if(nbInstances == 0){
		ProgressDialog::getInstance()->closeProgressbar();
		emit detectBlobs_finished();
	}
	delete this;
}
