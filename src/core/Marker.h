#ifndef MARKER_H_
#define MARKER_H_

#include <QString>
#include <vector>
#include <opencv/cv.h>
#include <opencv2/video/tracking.hpp>

#define MIN_marker(a,b) (((a)<(b))?(a):(b))
namespace xma{

	enum markerStatus { DELETED = -2, LOST = -1, UNDEFINED = 0, PREDICTED_PREVIOUS = 1, PREDICTED_2FRAME = 2, PREDICTED_RIGIDBODY = 3, TRACKED = 4, SET = 5, MANUAL_REFINED = 6 };
	class Trial;

	class Marker{
	public:

		Marker(int nbCameras, int size, Trial* _trial);
		virtual ~Marker();

		void setDescription(QString _description);
		QString getDescription();

		const std::vector< std::vector <markerStatus> > &getStatus2D();
		std::vector< std::vector <cv::Point2d> > &getPoints2D();
		std::vector<std::vector <cv::Point2d> > &getPoints2D_projected();
		std::vector<std::vector <double> > &getError2D();

		const std::vector <markerStatus>  &getStatus3D();
		std::vector< cv::Point3d> &getPoints3D();

		void setReference3DPoint(double x, double y, double z);
		void loadReference3DPoint(QString filename);
		void saveReference3DPoint(QString filename);

		cv::Point3d getReference3DPoint();
		bool Reference3DPointSet();

		void setPoint(int camera, int activeFrame, double x, double y, markerStatus status);
		std::vector < cv::Point2d > getEpipolarLine(int cameraOrigin, int CameraDestination, int frame);
		void reconstruct3DPoint(int frame);
		bool getMarkerPrediction(int camera, int frame, double &x, double &y, bool forward);

		double getSize();
		double getSizeRange();
		void setSize(int camera, int frame, double size_value);
		void save(QString points_filename, QString status_filename = "", QString markersize_filename = "", QString pointsWorld_filename = "");
		void load(QString points_filename, QString status_filename, QString markersize_filename);
		void resetMultipleFrames(int camera, int frameStart, int frameEnd);

		void update();

		bool isValid(int camera, int frame);
		void reset(int camera, int frame);

		int getSizeOverride();
		void setSizeOverride(int value);

		int getThresholdOffset();
		void setThresholdOffset(int value);

		int getMaxPenalty();
		void setMaxPenalty(int value);

		int getMethod();
		void setMethod(int value);

	private:
		void init(int nbCameras, int size);
		void addFrame();
		void clear();
		void reprojectPoint(int frame);
		void updateError(int frame);

		Trial *trial;

		QString description;
		int trialIdx;
		std::vector< std::vector <cv::Point2d> > points2D;
		std::vector< std::vector <cv::Point2d> > points2D_projected;
		std::vector< std::vector <markerStatus> > status2D;
		std::vector< std::vector <double> > error2D;

		void updateMeanSize();
		std::vector< std::vector <double> > markerSize;
		double meanSize;
		double sizeRange;

		int thresholdOffset;
		int sizeOverride;
		int maxPenalty;
		int method;

		bool point3D_ref_set;
		cv::Point3d point3D_ref;
		std::vector <cv::Point3d> points3D;
		std::vector <markerStatus> status3D;
		std::vector <double> error3D;
	};
}

#endif 
