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

//#define WRITEIMAGES 1

using namespace xma;

int MarkerTracking::nbInstances = 0;

MarkerTracking::MarkerTracking(int camera, int trial, int frame_from,int frame_to, int marker, bool forward) :QObject(),
m_camera(camera), m_trial(trial), m_frame_from(frame_from), m_frame_to(frame_to), m_marker(marker), m_forward(forward){
	nbInstances++;
	x_from = Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->getPoints2D()[m_camera][m_frame_from].x;
	y_from = Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->getPoints2D()[m_camera][m_frame_from].y;
	searchArea = 30;
	size = (int) (Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->getSize() + 0.5);
	size = (size < 5) ? 5 : size;

	Project::getInstance()->getTrials()[m_trial]->getVideoStreams()[m_camera]->getImage()->getSubImage(templ, size + 3, x_from + 0.5, y_from + 0.5);
	maxPenalty = Project::getInstance()->getTrials()[m_trial]->getMarkers()[m_marker]->getMaxPenalty();
#ifdef WRITEIMAGES
	fprintf(stderr, "Start Track Marker : Camera %d Pos %lf %lf Size %d\n", camera, x_from, y_from,size);
#endif
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
#ifdef WRITEIMAGES
	fprintf(stderr, "Prediction Track Marker : Camera %d Pos %lf %lf\n", m_camera, x_to, y_to);
#endif
	int off_x = (int)(x_to - used_size + 0.5);
	int off_y = (int)(y_to - used_size + 0.5);

	Project::getInstance()->getTrials()[m_trial]->getVideoStreams()[m_camera]->getImage()->getSubImage(ROI_to, used_size, off_x, off_y);

	/// Create the result matrix
	int result_cols = ROI_to.cols - templ.cols + 1;
	int result_rows = ROI_to.rows - templ.rows + 1;

	cv::Mat result;
	result.create(result_cols, result_rows, CV_32FC1);

	/// Do the Matching and Normalize

	matchTemplate(ROI_to, templ, result, cv::TM_CCORR_NORMED);
	normalize(result, result, 0, 125, cv::NORM_MINMAX, -1, cv::Mat());

#ifdef WRITEIMAGES
	cv::imwrite("Tra_Result.png", result);
#endif
	cv::Mat springforce;
	springforce.create(result_cols, result_rows, CV_32FC1);
	double halfcol = 0.5 * result_cols;
	double sigma = halfcol * sqrt(2 * log(255)) - 1;
	for (int i = 0; i < result_cols; i++)
	{
		for (int j = 0; j < result_cols; j++)
		{
			double val = exp(((halfcol - i) * (halfcol - i)) / (2 * sigma * sigma) +
				((halfcol - j) * (halfcol - j)) / (2 * sigma * sigma)
				);
			springforce.at<float>(i, j) = val;
		}
	}
	normalize(springforce, springforce, 0, maxPenalty, cv::NORM_MINMAX, -1, cv::Mat());

#ifdef WRITEIMAGES
	cv::imwrite("Tra_Penalty.png", springforce);
#endif

	result = result - springforce;
	normalize(result, result, 0, 255, cv::NORM_MINMAX, -1, cv::Mat());

#ifdef WRITEIMAGES
	cv::imwrite("Tra_PenResult.png", result);
#endif

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

	x_to = matchLoc.x + off_x + size + 3;
	y_to = matchLoc.y + off_y + size + 3;

#ifdef WRITEIMAGES
	fprintf(stderr, "Tracked %lf %lf\n", x_to, y_to);
	fprintf(stderr, "Stop Track Marker : Camera %d Pos %lf %lf\n", m_camera, x_to, y_to);
#endif


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
