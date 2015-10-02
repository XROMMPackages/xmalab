#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "processing/CheckerboardDetection.h"	

#include "ui/ProgressDialog.h"
#include "ui/MainWindow.h"

#include "core/Project.h"
#include "core/Camera.h"
#include "core/Image.h"
#include "core/CalibrationImage.h"
#include "core/CalibrationObject.h"
#include "core/Settings.h"

#include <QtCore>

using namespace xma;

int CheckerboardDetection::nbInstances = 0;

CheckerboardDetection::CheckerboardDetection(int camera, int image) :QObject(){
	m_camera = camera;
	m_image = image;
	nbInstances++;
}

CheckerboardDetection::~CheckerboardDetection(){

}

void CheckerboardDetection::detectCorner(){
	m_FutureWatcher = new QFutureWatcher<void>();
	connect(m_FutureWatcher, SIGNAL(finished()), this, SLOT(detectCorner_threadFinished()));

	QFuture<void> future = QtConcurrent::run(this, &CheckerboardDetection::detectCorner_thread);
	m_FutureWatcher->setFuture( future );

	ProgressDialog::getInstance()->showProgressbar(0, 0, "Detect Corner");
}

void CheckerboardDetection::detectCorner_thread(){
	tmpPoints.clear();
	cv::Mat image;
	
	Project::getInstance()->getCameras()[m_camera]->getCalibrationImages()[m_image]->getImage()->getImage(image);

	findChessboardCorners(image, cv::Size(CalibrationObject::getInstance()->getNbHorizontalSquares(), CalibrationObject::getInstance()->getNbVerticalSquares()), tmpPoints,
		cv::CALIB_CB_ADAPTIVE_THRESH); 

	image.release();
}

void CheckerboardDetection::detectCorner_threadFinished(){

	if (tmpPoints.size() == CalibrationObject::getInstance()->getNbHorizontalSquares() * CalibrationObject::getInstance()->getNbVerticalSquares())
		Project::getInstance()->getCameras()[m_camera]->getCalibrationImages()[m_image]->setDetectedPoints(tmpPoints);
	
	tmpPoints.clear();
	delete m_FutureWatcher;
	nbInstances--;
	if(nbInstances == 0){
		ProgressDialog::getInstance()->closeProgressbar();
		MainWindow::getInstance()->redrawGL();
		emit detectCorner_finished();
	}
	delete this;
}
