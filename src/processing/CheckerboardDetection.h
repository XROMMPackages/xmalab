#ifndef CHECKERBOARDDETECTION_H
#define CHECKERBOARDDETECTION_H

#include <QFutureWatcher>
#include <QObject>

#include <opencv/cv.h>

namespace xma{
	class CheckerboardDetection : public QObject{

		Q_OBJECT;

	public:
		CheckerboardDetection(int camera, int image);
		virtual ~CheckerboardDetection();
		void detectCorner();

		int m_camera;
		//if -1 undistortion;
		int m_image;
		cv::vector<cv::Point2d> tmpPoints;

		static bool isRunning(){
			return (nbInstances > 0);
		}

	signals:
		void detectCorner_finished();

		private slots:
		void detectCorner_threadFinished();

	private:
		void detectCorner_thread();
		QFutureWatcher<void>* m_FutureWatcher;
		static int nbInstances;
	};
}
#endif  // BLOBDETECTION_H