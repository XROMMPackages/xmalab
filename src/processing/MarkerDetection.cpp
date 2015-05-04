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

//#define WRITEIMAGES 1

using namespace xma;

int MarkerDetection::nbInstances = 0;

MarkerDetection::MarkerDetection(int camera, int trial, int frame, int marker, double searcharea, bool refinementAfterTracking) :QObject(){
	nbInstances++;
	m_camera = camera;
	m_trial = trial;
	m_frame = frame;
	m_marker = marker;
	m_refinementAfterTracking = refinementAfterTracking;
	x = Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->getPoints2D()[m_camera][m_frame].x;
	y = Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->getPoints2D()[m_camera][m_frame].y;
	m_searchArea = (int)(searcharea + 0.5);
	if (m_searchArea < 10) m_searchArea = 10;

	m_input_size = (Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->getSizeOverride() > 0) ? Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->getSizeOverride() : 
		(Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->getSize() > 0) ? Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->getSize() : 5;
	
	thresholOffset = Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->getThresholdOffset();
#ifdef WRITEIMAGES
	fprintf(stderr, "Start Marker Detection : Camera %d Pos %lf %lf Size %lf\n", m_camera, x, y, m_input_size);
#endif
}

MarkerDetection::~MarkerDetection(){

}

void MarkerDetection::detectMarker(){
	m_FutureWatcher = new QFutureWatcher<void>();
	connect(m_FutureWatcher, SIGNAL(finished()), this, SLOT(detectMarker_threadFinished()));

	QFuture<void> future = QtConcurrent::run(this, &MarkerDetection::detectMarker_thread);
	m_FutureWatcher->setFuture( future );

}

void MarkerDetection::detectMarker_thread(){

	off_x = (int)(x - m_searchArea + 0.5);
	off_y = (int)(y - m_searchArea + 0.5);

	//preprocess image
	cv::Mat image;
	Project::getInstance()->getTrials()[m_trial]->getVideoStreams()[m_camera]->getImage()->getSubImage(image, m_searchArea, off_x, off_y);

#ifdef WRITEIMAGES
	cv::imwrite("Det_original.png", image);
#endif

	//Convert To float
	cv::Mat img_float;
	image.convertTo(img_float, CV_32FC1);
	
	//Create Blurred image
	int radius = (int) (1.5*m_input_size + 0.5);
	double sigma = radius * sqrt(2 * log(255)) - 1;
	cv::Mat blurred;
	cv::GaussianBlur(img_float, blurred, cv::Size(2 * radius + 1, 2 * radius + 1), sigma);

#ifdef WRITEIMAGES
	cv::imwrite("Det_blur.png", blurred);
#endif

	//Substract Background
	cv::Mat diff = img_float - blurred;
	cv::normalize(diff, diff,0,255, cv::NORM_MINMAX, -1, cv::Mat());
	diff.convertTo(image, CV_8UC1);

#ifdef WRITEIMAGES
	cv::imwrite("Det_diff.png", diff);
#endif

	//Median
	cv::medianBlur(image, image, 3);

#ifdef WRITEIMAGES
	cv::imwrite("Det_med.png", image);
#endif

	//Thresholding
	double minVal;
	double maxVal;
	minMaxLoc(image, &minVal, &maxVal);
	double thres = 0.5 *minVal + 0.5 * image.at<uchar>(m_searchArea, m_searchArea) + thresholOffset * 0.01 * 255;
	cv::threshold(image, image, thres, 255, cv::THRESH_BINARY_INV);
	//cv::adaptiveThreshold(image, image, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV, 15, 10);

	cv::GaussianBlur(image, image, cv::Size(3, 3), 1.3);
#ifdef WRITEIMAGES
	fprintf(stderr, "Thres %lf Selected %d\n", thres, image.at<uchar>(m_searchArea, m_searchArea));
	cv::imwrite("Det_thresh.png", image);
#endif

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
#ifdef WRITEIMAGES	
	else
	{
		fprintf(stderr, "Not found\n");
	}

	fprintf(stderr, "Stop Marker Detection : Camera %d Pos %lf %lf Size %lf\n", m_camera, x, y, size);
#endif
	//clean
	img_float.release();
	blurred.release();
	diff.release();
	image.release();
	hierarchy.clear();
	contours.clear();
}

void MarkerDetection::detectMarker_threadFinished(){
	Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->setSize(m_camera,m_frame,size);
	Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->setPoint(m_camera, m_frame, x, y, m_refinementAfterTracking ? TRACKED : SET);

	delete m_FutureWatcher;
	nbInstances--;
	MainWindow::getInstance()->redrawGL();
	if(nbInstances == 0){
		emit detectMarker_finished();
	}
	delete this;
}
