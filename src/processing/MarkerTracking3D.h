#ifndef MARKERTRACKING3D_H
#define MARKERTRACKING3D_H

#include <QFutureWatcher>
#include <QObject>
#include <opencv2/opencv.hpp>
#include <vector>

namespace xma
{
	class MarkerTracking3D : public QObject
	{
		Q_OBJECT;

	public:
		MarkerTracking3D(int trial, int frame_from, int frame_to, int marker, bool forward);
		virtual ~MarkerTracking3D();
		void trackMarker();

		static bool isRunning()
		{
			return (nbInstances > 0);
		}

		static double s_searchArea;

		signals:
		void trackMarker_finished();

	private slots:
		void trackMarker_threadFinished();

	private:
		void trackMarker_thread();
		QFutureWatcher<void>* m_FutureWatcher;
		static int nbInstances;

		int m_trial;
		int m_frame_from;
		int m_frame_to;
		int m_marker;
		bool m_forward;

        cv::Point3d m_best3D;
        std::vector<cv::Point2d> m_best2D;
        std::vector<cv::Mat> m_templates;
	};
}
#endif // MARKERTRACKING3D_H
