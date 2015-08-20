#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <QFutureWatcher>
#include <QObject>

#include <opencv/cv.h>

namespace xma{
	class Calibration : public QObject{

		Q_OBJECT;

	public:
		Calibration(int camera, bool planar = false);
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
		bool m_planar;
		void computeCameraPosesAndCam_thread();
		void setInitialByReferences();

		QFutureWatcher<void>* m_FutureWatcher;
		static int nbInstances;

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