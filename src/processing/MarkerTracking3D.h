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

		signals:
		void trackMarker_finished();

	private slots:
		void trackMarker_threadFinished();

	private:
		struct Peak
		{
			cv::Point2d pt;
			float score;
		};

		struct CameraResult
		{
			cv::Mat ncc_map;
			cv::Point2d offset;
			std::vector<Peak> peaks;
		};

		void trackMarker_thread();

		bool triangulatePair(const cv::Point2d& pt1, int cam1,
		                     const cv::Point2d& pt2, int cam2,
		                     cv::Point3d& result) const;

		double evaluate3D(const cv::Point3d& p3d, const cv::Point3d& pred3D,
		                  const std::vector<CameraResult>& cam_results,
		                  int& valid_cams) const;

		static std::vector<Peak> extractPeaks(const cv::Mat& ncc_map, int max_peaks, double min_dist);

		QFutureWatcher<void>* m_FutureWatcher;
		static int nbInstances;

		int m_trial;
		int m_frame_from;
		int m_frame_to;
		int m_marker;
		bool m_forward;

		std::vector<cv::Mat> m_templates;

		cv::Point3d m_best3D;
		std::vector<cv::Point2d> m_best2D;
	};
}
#endif
