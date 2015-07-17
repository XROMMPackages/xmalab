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
#include <core/Settings.h>

//#define WRITEIMAGES 0

using namespace xma;

int MarkerDetection::nbInstances = 0;

MarkerDetection::MarkerDetection(int camera, int trial, int frame, int marker, double searcharea, bool refinementAfterTracking) :QObject(){
	nbInstances++;
	m_camera = camera;
	m_trial = trial;
	m_frame = frame;
	m_marker = marker;
	m_method = Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->getMethod();
	m_refinementAfterTracking = refinementAfterTracking;
	m_x = Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->getPoints2D()[m_camera][m_frame].x;
	m_y = Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->getPoints2D()[m_camera][m_frame].y;
	m_searchArea = (int)(searcharea + 0.5);
	
	if (m_method == 1)
	{
		if(m_searchArea < 50) m_searchArea = 50;
	}
	else if (m_searchArea < 10) m_searchArea = 10;

	m_input_size = (Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->getSizeOverride() > 0) ? Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->getSizeOverride() : 
		(Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->getSize() > 0) ? Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->getSize() : 5;
	
	m_thresholdOffset = Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->getThresholdOffset();
#ifdef WRITEIMAGES
	//fprintf(stderr, "Start Marker Detection : Camera %d Pos %lf %lf Size %lf\n", m_camera, cerx, y, m_input_size);
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

cv::Point2d MarkerDetection::detectionPoint(Image* image, int method, cv::Point2d center, int searchArea, int masksize, double threshold, double *size)
{
	cv::Point2d point_out(center.x, center.y);
	double tmp_size;
	
	int off_x = (int)(center.x - searchArea + 0.5);
	int off_y = (int)(center.y - searchArea + 0.5);

	//preprocess image
	cv::Mat subimage;
	image->getSubImage(subimage, searchArea, off_x, off_y);

#ifdef WRITEIMAGES
	cv::imwrite("Det_original.png", subimage);
#endif
	if (method == 0 || method == 2){

		if (method == 2) subimage = cv::Scalar::all(255) - subimage;

		//Convert To float
		cv::Mat img_float;
		subimage.convertTo(img_float, CV_32FC1);

		//Create Blurred image
		int radius = (int)(1.5*masksize + 0.5);
		double sigma = radius * sqrt(2 * log(255)) - 1;
		cv::Mat blurred;
		cv::GaussianBlur(img_float, blurred, cv::Size(2 * radius + 1, 2 * radius + 1), sigma);

#ifdef WRITEIMAGES
		cv::imwrite("Det_blur.png", blurred);
#endif

		//Substract Background
		cv::Mat diff = img_float - blurred;
		cv::normalize(diff, diff, 0, 255, cv::NORM_MINMAX, -1, cv::Mat());
		diff.convertTo(subimage, CV_8UC1);

#ifdef WRITEIMAGES
		cv::imwrite("Det_diff.png", diff);
#endif

		//Median
		cv::medianBlur(subimage, subimage, 3);

#ifdef WRITEIMAGES
		cv::imwrite("Det_med.png", subimage);
#endif

		//Thresholding
		double minVal;
		double maxVal;
		minMaxLoc(subimage, &minVal, &maxVal);
		double thres = 0.5 *minVal + 0.5 * subimage.at<uchar>(searchArea, searchArea) + threshold * 0.01 * 255;
		cv::threshold(subimage, subimage, thres, 255, cv::THRESH_BINARY_INV);
		//cv::adaptiveThreshold(image, image, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV, 15, 10);

		cv::GaussianBlur(subimage, subimage, cv::Size(3, 3), 1.3);
#ifdef WRITEIMAGES
		//fprintf(stderr, "Thres %lf Selected %d\n", thres, image.at<uchar>(searchArea, searchArea));
		cv::imwrite("Det_thresh.png", subimage);
#endif

		//Find contours
		cv::vector<cv::vector<cv::Point> > contours;
		cv::vector<cv::Vec4i> hierarchy;
		cv::findContours(subimage, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, cv::Point(off_x, off_y));
		double dist = 1000;
		int bestIdx = -1;

		//Find closest contour
		for (int i = 0; i < contours.size(); i++)
		{
			cv::Point2f circle_center;
			float circle_radius;
			cv::minEnclosingCircle(contours[i], circle_center, circle_radius);

			double distTmp = sqrt((center.x - circle_center.x) * (center.x - circle_center.x) + (center.y - circle_center.y) * (center.y - circle_center.y));
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
			point_out.x = circle_center.x;
			point_out.y = circle_center.y;
			tmp_size = circle_radius;
		}
#ifdef WRITEIMAGES	
		else
		{
			fprintf(stderr, "Not found\n");
		}

		//fprintf(stderr, "Stop Marker Detection : Camera %d Pos %lf %lf Size %lf\n", m_camera, x, y, size);
#endif
		//clean
		img_float.release();
		blurred.release();
		diff.release();
		hierarchy.clear();
		contours.clear();
	}
	else if (method == 1)
	{
		cv::SimpleBlobDetector::Params paramsBlob;

		paramsBlob.thresholdStep = Settings::getInstance()->getFloatSetting("BlobDetectorThresholdStep");
		paramsBlob.minThreshold = Settings::getInstance()->getFloatSetting("BlobDetectorMinThreshold");
		paramsBlob.maxThreshold = Settings::getInstance()->getFloatSetting("BlobDetectorMaxThreshold");
		paramsBlob.minRepeatability = Settings::getInstance()->getIntSetting("BlobDetectorMinRepeatability");
		paramsBlob.minDistBetweenBlobs = Settings::getInstance()->getFloatSetting("BlobDetectorMinDistBetweenBlobs");

		paramsBlob.filterByColor = Settings::getInstance()->getBoolSetting("BlobDetectorFilterByColor");
		paramsBlob.blobColor = 255 - Settings::getInstance()->getIntSetting("BlobDetectorBlobColor");

		paramsBlob.filterByArea = Settings::getInstance()->getBoolSetting("BlobDetectorFilterByArea");
		paramsBlob.minArea = Settings::getInstance()->getFloatSetting("BlobDetectorMinArea");
		paramsBlob.maxArea = Settings::getInstance()->getFloatSetting("BlobDetectorMaxArea");

		paramsBlob.filterByCircularity = Settings::getInstance()->getBoolSetting("BlobDetectorFilterByCircularity");
		paramsBlob.minCircularity = Settings::getInstance()->getFloatSetting("BlobDetectorMinCircularity");
		paramsBlob.maxCircularity = Settings::getInstance()->getFloatSetting("BlobDetectorMaxCircularity");

		paramsBlob.filterByInertia = Settings::getInstance()->getBoolSetting("BlobDetectorFilterByInertia");
		paramsBlob.minInertiaRatio = Settings::getInstance()->getFloatSetting("BlobDetectorMinInertiaRatio");
		paramsBlob.maxInertiaRatio = Settings::getInstance()->getFloatSetting("BlobDetectorMaxInertiaRatio");

		paramsBlob.filterByConvexity = Settings::getInstance()->getBoolSetting("BlobDetectorFilterByConvexity");
		paramsBlob.minConvexity = Settings::getInstance()->getFloatSetting("BlobDetectorMinConvexity");
		paramsBlob.maxConvexity = Settings::getInstance()->getFloatSetting("BlobDetectorMaxConvexity");

		cv::FeatureDetector * detector = new cv::SimpleBlobDetector(paramsBlob);
		cv::vector<cv::KeyPoint> keypoints;

		detector->detect(subimage, keypoints);

		double dist_min = searchArea*searchArea;
		double dist;
		for (unsigned int i = 0; i<keypoints.size(); i++){
			dist = cv::sqrt((keypoints[i].pt.x - (searchArea + 1)) * (keypoints[i].pt.x - (searchArea + 1)) + (keypoints[i].pt.y - (searchArea + 1))*(keypoints[i].pt.y - (searchArea + 1)));
			if (dist < dist_min)
			{
				point_out.x = off_x + keypoints[i].pt.x;
				point_out.y = off_y + keypoints[i].pt.y;
				tmp_size = keypoints[i].size;
			}
		}
		keypoints.clear();
	}

	subimage.release();

if (size != NULL)
{
	*size = tmp_size;
}

 return point_out;
}

void MarkerDetection::detectMarker_thread(){

	cv::Point pt = detectionPoint(Project::getInstance()->getTrials()[m_trial]->getVideoStreams()[m_camera]->getImage(), m_method, cv::Point2d(m_x, m_y), m_searchArea, m_input_size, m_thresholdOffset,&m_size);

	m_x = pt.x;
	m_y = pt.y;
}

void MarkerDetection::detectMarker_threadFinished(){
	Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->setSize(m_camera, m_frame, m_size);
	Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->setPoint(m_camera, m_frame, m_x, m_y, m_refinementAfterTracking ? TRACKED : SET);

	delete m_FutureWatcher;
	nbInstances--;
	MainWindow::getInstance()->redrawGL();
	if(nbInstances == 0){
		emit detectMarker_finished();
	}
	delete this;
}
