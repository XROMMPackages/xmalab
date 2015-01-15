#ifndef MARKERDETECTION_H
#define MARKERDETECTION_H

#include <QFutureWatcher>
#include <QObject>

#include <opencv/cv.h>

namespace xma{
	class MarkerDetection : public QObject{

		Q_OBJECT;

	public:
		MarkerDetection(int camera, int trial, int frame, int marker, double searcharea = 30.0);
		virtual ~MarkerDetection();
		void detectMarker();

		static bool isRunning(){
			return (nbInstances > 0);
		}

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

		double x, y;
		double size;

		int off_x;
		int off_y;
		int m_searchArea;

	};
}
#endif  // MARKERDETECTION_H