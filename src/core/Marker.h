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

		const std::vector <markerStatus>  &getStatus3D();
		std::vector< cv::Point3d> &getPoints3D();

		//void movePoint(int camera, int activeFrame, double x, double y);
		//void setTrackedPoint(int camera, int activeFrame, double x, double y);
		void setPoint(int camera, int activeFrame, double x, double y, markerStatus status);
		std::vector < cv::Point2d > getEpipolarLine(int cameraOrigin, int CameraDestination, int frame);
		void reconstruct3DPoint(int frame);
		bool getMarkerPrediction(int camera, int frame, double &x, double &y, bool forward);

		double getSize();
		double getSizeRange();
		void setSize(int camera, int frame, double size_value);
		void save(QString points_filename, QString status_filename, QString markersize_filename);
		void load(QString points_filename, QString status_filename, QString markersize_filename);
		void update();

		bool isValid(int camera, int frame);
		void reset(int camera, int frame);

	private:
		void init(int nbCameras, int size);
		void clear();
		void reprojectPoint(int frame);

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

		std::vector <cv::Point3d> points3D;
		std::vector <markerStatus> status3D;
		std::vector <double> error3D;

		std::istream &comma(std::istream& in);
		std::istream &getline(std::istream &is, std::string &s);
	};
}

#endif 
