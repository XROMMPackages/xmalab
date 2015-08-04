#ifndef MULTICAMERACALIBRATION_H
#define MULTICAMERACALIBRATION_H

#include <QFutureWatcher>
#include <QObject>

#include <opencv/cv.h>

namespace xma{
	class MultiCameraCalibration : public QObject{

		Q_OBJECT;

	public:
		MultiCameraCalibration(int method, int iterations, double initial);
		virtual ~MultiCameraCalibration();

		void optimizeCameraSetup();
		static bool isRunning(){
			return (nbInstances > 0);
		}

		int nbCameras;
		int nbFrames;
		int nbPoints;
		int nbParams;

		int nbCamParams;


		double * p;
		double * x;
		void projError(int n, double * p,  double * x);
		void projErrorJac(int n, int m, double * p, double * Jac);
		int getStep();

		static void reproject(int c);

	signals:
		void optimizeCameraSetup_finished();

		private slots:
		void optimizeCameraSetup_threadFinished();

	private:
		
		int m_iterations;
		int m_method;
		double m_initial;

		bool withDistortion;
		bool seperateDimensions;

		std::vector<int> frameIdx;
		std::vector<int> camIdx;
		std::vector<int> pts3DIdx;
		std::vector<cv::Point2d> Pts2D;
		std::vector<cv::Point3d> Pts3D;
		std::vector<cv::Mat> cameraRotationVector;
		std::vector<cv::Mat> cameraTranslationVector;
		std::vector<cv::Mat> cameraMatrix;
		std::vector<cv::Mat> distortion;
		std::vector<cv::Mat> chessRotationVector;
		std::vector<cv::Mat> chessTranslationVector;

		void optimizeCameraSetup_thread();
		cv::Mat getTransformationMatrix(cv::Mat rotationvector, cv::Mat translationvector);
		cv::Mat getTranslation(cv::Mat trans);
		cv::Mat getRotationVector(cv::Mat trans);
		int refIdx;

		
		QFutureWatcher<void>* m_FutureWatcher;
		static int nbInstances;
	};
}
#endif  // MULTICAMERACALIBRATION_H