#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <QFutureWatcher>
#include <QObject>

#include <opencv/cv.h>

namespace xma{
	class Calibration : public QObject{

		Q_OBJECT;

	public:
		Calibration(int camera);
		virtual ~Calibration();

		void computeCameraPosesAndCam();
		static bool isRunning(){
			return (nbInstances > 0);
		}
	signals:
		void computeCameraPosesAndCam_finished();

		private slots:
		void computeCameraPosesAndCam_threadFinished();

	private:
		int m_camera;

		void computeCameraPosesAndCam_thread();
		void reproject();
		void setInitialByReferences();

		QFutureWatcher<void>* m_FutureWatcher;
		static int nbInstances;

		cv::vector<cv::Mat> rvecs;
		cv::vector<cv::Mat> tvecs;
		cv::Mat distortion_coeffs;
		cv::Mat intrinsic_matrix;
		std::vector<std::vector<cv::Point3f> > object_points;
		std::vector<std::vector<cv::Point2f> > image_points;

		cv::Mat rotationvector;
		cv::Mat translationvector;

		cv::vector<cv::Point2d> projectedPointsUndistorted;
		cv::vector<double> error;
	};
}
#endif  // CALIBRATION_H