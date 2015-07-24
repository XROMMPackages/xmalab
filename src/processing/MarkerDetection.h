#ifndef MARKERDETECTION_H
#define MARKERDETECTION_H

#include <QFutureWatcher>
#include <QObject>

#include <opencv/cv.h>

namespace xma{
	class Image;

	class MarkerDetection : public QObject{

		Q_OBJECT;

	public:
		MarkerDetection(int camera, int trial, int frame, int marker, double searcharea = 30.0, bool refinementAfterTracking = false);
		virtual ~MarkerDetection();
		void detectMarker();

		static bool isRunning(){
			return (nbInstances > 0);
		}

		static cv::Point2d detectionPoint(Image* image, int method, cv::Point2d center, int searchArea, int masksize, double threshold = 8, double *size = NULL);

		static void refinePointPolynomialFit(cv::Point2d &pt, double &radius, bool darkMarker, int camera, int trial);

	signals:
		void detectMarker_finished();

		private slots:
		void detectMarker_threadFinished();

	private:
		void detectMarker_thread();
		QFutureWatcher<void>* m_FutureWatcher;
		static int nbInstances;

		int m_camera;
		int m_frame;
		int m_trial;
		int m_marker;
		int m_method;
		double m_x, m_y;
		double m_size;
		double m_input_size;
		uchar m_thresholdOffset;

		int m_searchArea;
		bool m_refinementAfterTracking;
	};
}
#endif  // MARKERDETECTION_H