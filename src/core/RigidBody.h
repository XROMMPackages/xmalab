#ifndef RIGIDBODY_H_
#define RIGIDBODY_H_

#include <QString>
#include <vector>
#include <opencv/cv.h>


namespace xma{
	class Trial;
	class Camera;

	class RigidBody{
	public:

		RigidBody(int size, Trial * trial);
		virtual ~RigidBody();

		void setDescription(QString _description);
		QString getDescription();

		const std::vector <int > &getPointsIdx();
		const std::vector <int> &getPoseComputed();

		void clearPointIdx();
		void addPointIdx(int idx);
		void removePointIdx(int idx);

		void setExpanded(bool _expanded);
		bool isExpanded();

		void computeRBCSMeanOfAllFrames();
		void computeRBCS(int Frame);
		void computeRBPoseInWorldCS(int Frame);
		std::vector <cv::Point2d> projectToImage(Camera * cam, int Frame, bool with_center);
		void setMissingPoints(int Frame);

	private:
		void init(int size);
		void clear();

		QString description;
		bool expanded;
		bool initialised;

		Trial * trial;

		std::vector <int> pointsIdx;
		std::vector <cv::Point3d> points3D;
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
