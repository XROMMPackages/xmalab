#ifndef LOCALUNDISTORTION_H
#define LOCALUNDISTORTION_H

#include <QFutureWatcher>
#include <QObject>

#include <opencv/cv.h>

namespace xma{
	class LocalUndistortion : public QObject{

		Q_OBJECT;

	public:
		LocalUndistortion(int camera);
		virtual ~LocalUndistortion();

		void computeUndistortion(bool recompute = false);
		static bool isRunning(){
			return (nbInstances > 0);
		}
	signals:
		void localUndistortion_finished();

		private slots:
		void localUndistortion_threadFinished();

	private:
		int m_camera;
		bool hasReferences;

		cv::vector<cv::Point2d> detectedPoints;
		cv::vector<cv::Point2d> tmpPoints_distorted;
		cv::vector<cv::Point2d> tmpPoints_references;

		cv::vector<bool> tmpPoints_inlier;
		cv::Mat map_x;
		cv::Mat map_y;

		cv::Mat _A;
		cv::Mat _B;
		cv::Mat _radii;

		cv::Mat A_inverse;
		cv::Mat B_inverse;
		cv::Mat radii_inverse;

		cv::Mat controlPts;
		cv::Mat controlPts_inverse;

		void localUndistortion_thread();
		QFutureWatcher<void>* m_FutureWatcher;
		static int nbInstances;

		void setupCorrespondances();
		int computeLWM(cv::Mat &detectedPointsPtsInlier, cv::Mat &controlPtsInlier, cv::Mat &A, cv::Mat &B, cv::Mat &radii);
		void setPointsByInlier(cv::vector<cv::Point2d> &pts, cv::Mat &ptsInlier);
		void createLookupTable(cv::Mat &controlPts, cv::Mat &A, cv::Mat &B, cv::Mat &radii, cv::Mat &outMat_x, cv::Mat &outMat_y, int gridSize);

		bool findNClosestPoint(int numberPoints, cv::Point2d pt, cv::vector<cv::Point2d> &closest_pts, cv::vector<cv::Point2d> all_pts, bool skipfirst);
		double getHexagonalGridOrientation(cv::Point2d center, cv::vector<cv::Point2d> all_pts);
		double getHexagonalGridSize(cv::Point2d center);
		cv::vector<cv::Point2d> get6adjCells(cv::Point2d center, cv::vector<cv::Point2d> all_pts);
		bool addNeighbours(cv::Point2d centercont, cv::Point2d centerdet, double dY, double dX, double dOffY, double dYdist, cv::vector<double> &dy_vec);
		void setupHexagonalGrid(cv::Point2d center, double dY);
	};

}



	

#endif  // LOCALUNDISTORTION_H