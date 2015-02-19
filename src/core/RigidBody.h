#ifndef RIGIDBODY_H_
#define RIGIDBODY_H_

#include <QString>
#include <vector>
#include <opencv/cv.h>


namespace xma{
	class Trial;
	class Camera;
	class Marker;

	class RigidBody{
	public:

		RigidBody(int size, Trial * trial);
		virtual ~RigidBody();

		void setDescription(QString _description);
		QString getDescription();

		const std::vector <int > &getPointsIdx();
		const std::vector <int> &getPoseComputed();
		const std::vector <QString> &getReferenceNames();

		Marker * getMarker(int idx);

		void clearPointIdx();
		void addPointIdx(int idx);
		void removePointIdx(int idx);
		void setPointIdx(int idx, int markerIdx);

		void setExpanded(bool _expanded);
		bool isExpanded();

		void computeCoordinateSystemAverage();
		void computeCoordinateSystem(int Frame);
		void computePose(int Frame);

		std::vector <cv::Point2d> projectToImage(Camera * cam, int Frame, bool with_center);
		void setMissingPoints(int Frame);

		int setReferenceFromFile(QString filename);
		double fitAndComputeError(std::vector<cv::Point3d> src, std::vector<cv::Point3d> dst);

		void save(QString filename_referenceNames, QString filename_points3D);
		void load(QString filename_referenceNames, QString filename_points3D);
		void saveTransformations(QString filename, bool inverse);

		bool isReferencesSet();
		void setReferencesSet(bool value);
		void resetReferences();

	private:
		void init(int size);
		void clear();

		QString description;
		bool expanded;
		bool initialised;
		bool referencesSet;

		Trial * trial;

		std::vector <int> pointsIdx;
		std::vector <cv::Point3d> points3D;
		std::vector <QString> referenceNames;
		//Markers

		//for each frame
		std::vector <int> poseComputed;
		std::vector <cv::Mat> rotationvectors;
		std::vector <cv::Mat> translationvectors;

		std::vector <double> errorMean2D;
		std::vector <double> errorSd2D;
		std::vector <double> errorMean3D;
		std::vector <double> errorSd3D;
	};
}

#endif 
