#ifndef BLOBDETECTION_H
#define BLOBDETECTION_H

#include "processing/ThreadedProcessing.h"

#include <QFutureWatcher>
#include <QObject>

#include <opencv/cv.h>

namespace xma{
	class BlobDetection : public ThreadedProcessing{

		Q_OBJECT;

	public:
		BlobDetection(int camera, int image);
		virtual ~BlobDetection();

	protected:
		void process() override;
		void process_finished() override;

	private:
		int m_camera;
		//if -1 undistortion;
		int m_image;
		cv::vector<cv::Point2d> tmpPoints;

	};
}
#endif  // BLOBDETECTION_H