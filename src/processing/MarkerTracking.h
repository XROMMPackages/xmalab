#ifndef MARKERTRACKING_H
#define MARKERTRACKING_H

#include <QFutureWatcher>
#include <QObject>

#include <opencv/cv.h>

namespace xma{
	class MarkerTracking : public QObject{

		Q_OBJECT;

	public:
		MarkerTracking(int camera, int trial, int frame_from, int frame_to, int marker, bool forward);
		virtual ~MarkerTracking();
		void trackMarker();

		static bool isRunning(){
			return (nbInstances > 0);
		}

	signals:
		void trackMarker_finished();

		private slots:
		void trackMarker_threadFinished();

	private:
		void trackMarker_thread();
		QFutureWatcher<void>* m_FutureWatcher;
		static int nbInstances;

		int m_camera;
		int m_frame_from;
		int m_frame_to;
		int m_trial;
		int m_marker;
		bool m_forward;

		double x_from;
		double y_from;

		double x_to;
		double y_to;

		double size;
		int searchArea;
		int maxPenalty;

		cv::Mat templ;
	};
}
#endif  // MARKERTRACKING_H