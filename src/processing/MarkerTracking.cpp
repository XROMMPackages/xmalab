#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "processing/MarkerTracking.h"	

#include "core/Project.h"
#include "core/Image.h"
#include "core/Trial.h"
#include "core/Marker.h"

#include <QtCore>
#include <opencv/highgui.h>

using namespace xma;

int MarkerTracking::nbInstances = 0;

MarkerTracking::MarkerTracking(int camera, int trial, int frame_from,int frame_to, int marker, bool forward) :QObject(),
m_camera(camera), m_trial(trial), m_frame_from(frame_from), m_frame_to(frame_to), m_marker(marker), m_forward(forward){
	nbInstances++;
	x_from = Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->getPoints2D()[m_camera][m_frame_from].x;
	y_from = Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->getPoints2D()[m_camera][m_frame_from].y;
	searchArea = 30;
	size = Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->getSize();

	Project::getInstance()->getTrials()[m_trial]->getImage(m_camera)->getSubImage(templ, size + 3, x_from, y_from + 0.5);
}

MarkerTracking::~MarkerTracking(){

}

void MarkerTracking::trackMarker(){
	m_FutureWatcher = new QFutureWatcher<void>();
	connect(m_FutureWatcher, SIGNAL(finished()), this, SLOT(trackMarker_threadFinished()));

	QFuture<void> future = QtConcurrent::run(this, &MarkerTracking::trackMarker_thread);
	m_FutureWatcher->setFuture( future );

}

void MarkerTracking::trackMarker_thread(){
	cv::Mat ROI_to;
	int used_size = size + searchArea + 3;

	Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->getMarkerPrediction(m_camera, m_frame_to, x_to, y_to, m_forward);
	int off_x = (int)(x_to - searchArea + 0.5);
	int off_y = (int)(y_to - searchArea + 0.5);

	Project::getInstance()->getTrials()[m_trial]->getImage(m_camera)->getSubImage(ROI_to, used_size, off_x, off_y);

	/// Create the result matrix
	int result_cols = ROI_to.cols - templ.cols + 1;
	int result_rows = ROI_to.rows - templ.rows + 1;

	cv::Mat result;
	result.create(result_cols, result_rows, CV_32FC1);

	/// Do the Matching and Normalize
	//cv::medianBlur(templ, templ, 3);
	//cv::medianBlur(ROI_to, ROI_to, 3);
	matchTemplate(ROI_to, templ, result, cv::TM_CCORR_NORMED);
	normalize(result, result, 0, 255, cv::NORM_MINMAX, -1, cv::Mat());
	//cv::GaussianBlur(result, result, cv::Size(3, 3), 0, 0);

	/// Localizing the best match with minMaxLoc
	double minVal; double maxVal; cv::Point minLoc; cv::Point maxLoc;
	cv::Point matchLoc;

	minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, cv::Mat());

	/// For SQDIFF and SQDIFF_NORMED, the best matches are lower values. For all the other methods, the higher the better
	//if (match_method == CV_TM_SQDIFF || match_method == CV_TM_SQDIFF_NORMED)
	//{
	//	matchLoc = minLoc;
	//}
	//else
	//{
	matchLoc = maxLoc;
	//}
	x_to = matchLoc.x + off_x + size + 2;
	y_to = matchLoc.y + off_y + size + 2;
	
	//cv::imwrite("Tracking_roi.png", ROI_to);
	//cv::imwrite("Tracking_result.png", result);
	//cv::imwrite("Tracking_template.png", templ);
	/*cv::Mat imagetmp; 
	cv::imwrite("unprocessed.png", ROI_to);
	cv::GaussianBlur(ROI_to, ROI_to, cv::Size(4 * (int)(size + 0.5) + 1, 4 * (int)(size + 0.5) + 1),0,0);
	cv::imwrite("blurred.png", ROI_to);
	cv::Sobel(ROI_to, imagetmp, CV_64F, 0, 1, 2 * (int)(size + 0.5) + 1);
	cv::Sobel(imagetmp, imagetmp, CV_64F, 1, 0, 2 * (int)(size + 0.5) + 1);

	double minVal2;
	double maxVal2;
	cv:: minMaxLoc(imagetmp, &minVal, &maxVal);

	imagetmp -= minVal;
	cv::convertScaleAbs(imagetmp, imagetmp, 255.0 / (maxVal - minVal));

	imagetmp.convertTo(ROI_to, CV_8UC1);

	cv::imwrite("processed.png", ROI_to);*/

	ROI_to.release();
	result.release();
}

void MarkerTracking::trackMarker_threadFinished(){
	Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->setPoint(m_camera, m_frame_to, x_to, y_to, TRACKED);
	delete m_FutureWatcher;
	nbInstances--;
	if(nbInstances == 0){
		emit trackMarker_finished();
	}
	delete this;
}
