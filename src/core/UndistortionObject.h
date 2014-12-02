/*
 * Calibration.h
 *
 *  Created on: Nov 18, 2013
 *      Author: ben
 */

#ifndef UNDISTORTIONOBJECT_H_
#define UNDISTORTIONOBJECT_H_

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <QString>

#include <opencv/cv.h>

class Camera;
class Image;

class UndistortionObject{

	public:

		UndistortionObject(Camera* _camera, QString imageFileName);
		~UndistortionObject();
		
		Image* getImage(){return image;}
		Image* getUndistortedImage(){return undistortedImage;}

		bool isComputed(){return computed;}
		void setComputed(bool value){computed = value;}

		bool undistort(Image* distorted, Image * undistorted);
		bool undistort(QString filenameIn, QString filenameOut);
		void setDetectedPoints(cv::vector <cv::Point2d> &points);
		void getDetectedPoints(cv::vector <cv::Point2d> &points);

		void setGridPoints(cv::vector <cv::Point2d> &points_distorted , cv::vector <cv::Point2d> &points_references, cv::vector<bool> &points_inlier);
		void getGridPoints(cv::vector <cv::Point2d> &points_distorted , cv::vector <cv::Point2d> &points_references, cv::vector<bool> &points_inlier);

		void setMaps(cv::Mat & map_x, cv::Mat & map_y);

		void setLWMMatrices(cv::Mat & A, cv::Mat & B, cv::Mat & radii, cv::Mat & points,
								cv::Mat & A_inverse, cv::Mat & B_inverse, cv::Mat & radii_inverse, cv::Mat & points_inverse);
		
		void undistortPoints();

		cv::Point2d transformPoint(cv::Point2d pt, bool undistort);

		void toggleOutlier(int vispoints, double x, double y);
		void setCenter(double x, double y);
		bool isCenterSet(){return centerSet;}
		cv::Point2d getCenter(){return center;}
		void setRecalibrationRequired(int value){requiresRecalibration = value;}
		int isRecalibrationRequired(){return requiresRecalibration;}
		
		void setUpdateInfoRequired(bool value){updateInfoRequired = value;}
		int isUpdateInfoRequired(){return updateInfoRequired;}
		
		QString getFilename();
		QString getFilenameBase();
		QString getFilenamePointsDetected();
		QString getFilenameGridPointsDistorted();
		QString getFilenameGridPointsReferences();
		QString getFilenameGridPointsInlier();

		void savePointsDetected( QString filename);
		void saveGridPointsDistorted( QString filename);
		void saveGridPointsReferences( QString filename);
		void saveGridPointsInlier( QString filename);
		void exportData(QString csvFileNameLUT,QString csvFileNameInPoints , QString csvFileNameBasePoints);

		void loadPointsDetected( QString filename);
		void loadGridPointsDistorted( QString filename);
		void loadGridPointsReferences( QString filename);
		void loadGridPointsInlier( QString filename);

		int getWidth(){return width;}
		int getHeight(){return height;}
		void loadTextures();

		void drawData(int type);
		void bindTexture(int type);

		cv::vector<bool> &getInlier(){return points_grid_inlier;}
		cv::vector<double> &getError(){return error;}

	private:
		//Camera
		Camera* camera;
		int height,width;
		QString imageFileName;

		//Images
		Image* image;
		Image* undistortedImage;

		Image* tmpImage;
		int tmpImageType;

		//UndistortionInput
		bool computed;

		//Points
		cv::vector <cv::Point2d> points_detected;
		cv::vector <cv::Point2d> points_grid_distorted;
		cv::vector <cv::Point2d> points_grid_undistorted;
		cv::vector<bool> points_grid_inlier;
		cv::vector <cv::Point2d> points_grid_references;
		cv::vector<double> error;

		cv::Mat undistortionMapX;
		cv::Mat undistortionMapY;

		cv::Mat distortionMatrixA;
		cv::Mat distortionMatrixB;
		cv::Mat distortionMatrixRadii;
		cv::Mat distortionPoints;

		cv::Mat undistortionMatrixA;
		cv::Mat undistortionMatrixB;
		cv::Mat undistortionMatrixRadii;
		cv::Mat undistortionPoints;

		std::istream& comma(std::istream& in);
		std::istream& getline(std::istream &is, std::string &s);
		void savePoints(std::vector <cv::Point2d> &points, QString filename);
		void loadPoints(cv::vector <cv::Point2d> &points, QString filename);
		void drawPoints(std::vector <cv::Point2d> &points);
		int findClosestPoint(std::vector <cv::Point2d> &points, double x, double y);
		void computeError();

		cv::Point2d transformLWM(cv::Point2d &ptIn, cv::Mat &controlPts, cv::Mat &A, cv::Mat &B, cv::Mat &radii);
		void computeTmpImage(int type);
		void getDisplacementScaledAngle(cv::Mat &imageOut);
		void getDisplacementAngle(cv::Mat &imageOut);
		void getDisplacementLength(cv::Mat &imageOut);
		void getDisplacementX(cv::Mat &imageOut);
		void getDisplacementY(cv::Mat &imageOut);

		cv::Point2d center;
		bool centerSet;
		int requiresRecalibration;

		bool updateInfoRequired;
};


#endif /* UNDISTORTIONOBJECT_H_ */
