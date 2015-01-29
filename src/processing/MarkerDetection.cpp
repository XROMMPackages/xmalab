#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "processing/MarkerDetection.h"	

#include "ui/MainWindow.h"

#include "core/Project.h"
#include "core/Image.h"
#include "core/Trial.h"
#include "core/Marker.h"

#include <QtCore>
#include <opencv/highgui.h>

using namespace xma;

int MarkerDetection::nbInstances = 0;

MarkerDetection::MarkerDetection(int camera, int trial, int frame, int marker, double searcharea) :QObject(){
	m_camera = camera;
	m_trial = trial;
	m_frame = frame;
	m_marker = marker;
	x = Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->getPoints2D()[m_camera][m_frame].x;
	y = Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->getPoints2D()[m_camera][m_frame].y;
	m_searchArea = (int)(searcharea + 0.5);
}

MarkerDetection::~MarkerDetection(){

}

void MarkerDetection::detectMarker(){
	m_FutureWatcher = new QFutureWatcher<void>();
	connect(m_FutureWatcher, SIGNAL(finished()), this, SLOT(detectMarker_threadFinished()));

	QFuture<void> future = QtConcurrent::run(this, &MarkerDetection::detectMarker_thread);
	m_FutureWatcher->setFuture( future );
	nbInstances++;
}

void MarkerDetection::detectMarker_thread(){
	off_x = (int)(x - m_searchArea + 0.5);
	off_y = (int)(y - m_searchArea + 0.5);

	//preprocess image
	cv::Mat image;
	Project::getInstance()->getTrials()[m_trial]->getImage(m_camera)->getSubImage(image, m_searchArea, off_x, off_y);

	cv::medianBlur(image, image, 3);
	cv::adaptiveThreshold(image, image, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV, 15, 5);
	cv::medianBlur(image, image, 3);
	
	//Find contours
	cv::vector<cv::vector<cv::Point> > contours;
	cv::vector<cv::Vec4i> hierarchy;
	cv::findContours(image, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, cv::Point(off_x, off_y));
	double dist = 1000;
	int bestIdx = -1;

	//Find closest contour
	for (int i = 0; i< contours.size(); i++)
	{
		cv::Point2f circle_center;
		float circle_radius;
		cv::minEnclosingCircle(contours[i], circle_center, circle_radius);

		double distTmp = sqrt((x - circle_center.x) * (x - circle_center.x) + (y - circle_center.y) * (y - circle_center.y));
		if (distTmp < dist){
			bestIdx = i;
			dist = distTmp;
		}
	}

	//set contour
	if (bestIdx >= 0)
	{
		cv::Point2f circle_center;
		float circle_radius;
		cv::minEnclosingCircle(contours[bestIdx], circle_center, circle_radius);
		x = circle_center.x;
		y = circle_center.y;
		size = circle_radius;
	}

	//clean
	image.release();
	hierarchy.clear();
	contours.clear();
}

void MarkerDetection::detectMarker_threadFinished(){
	Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->setSize(m_camera,m_frame,size);
	Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->setPoint(m_camera, m_frame, x, y, SET);

	delete m_FutureWatcher;
	nbInstances--;
	MainWindow::getInstance()->redrawGL();
	if(nbInstances == 0){
		emit detectMarker_finished();
	}
	delete this;
}
