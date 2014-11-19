#ifndef BLOBDETECTION_H
#define BLOBDETECTION_H

#include <QFutureWatcher>
#include <QObject>

#include <opencv/cv.h>

class BlobDetection: public QObject{
	
	Q_OBJECT;

	public:
		BlobDetection(int camera, int image);
		~BlobDetection();
		void detectBlobs();

		int m_camera;
		//if -1 undistortion;
		int m_image;
		cv::vector<cv::Point2d> tmpPoints;

		static bool isRunning(){
			return (nbInstances > 0);
		}

	signals:
		void detectBlobs_finished();  

	private slots:
		void detectBlobs_threadFinished();

	private:
		void detectBlobs_thread();
		QFutureWatcher<void>* m_FutureWatcher;
		static int nbInstances;
};

#endif  // BLOBDETECTION_H