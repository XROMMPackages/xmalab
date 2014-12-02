/*
 * Calibration.h
 *
 *  Created on: Nov 18, 2013
 *      Author: ben
 */

#ifndef CALIBRATIONIMAGE_H_
#define CALIBRATIONIMAGE_H_

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <QString>

#include <opencv/cv.h>

class Camera;
class Image;

class CalibrationImage{
	public:

		CalibrationImage(Camera* _camera,QString imageFileName);
		~CalibrationImage();

		int getWidth(){return width;}
		int getHeight(){return height;}
		QString getFilename();
		QString getFilenameBase();

		Image* getImage(){return image;}
		Image* getUndistortedImage(){return undistortedImage;}

		int isCalibrated(){return calibrated;}
		void setCalibrated(int value){calibrated = value;}

		QString getImageFilename();
		void loadTextures();

		void setDetectedPoints(cv::vector <cv::Point2d> &points);
		cv::vector <cv::Point2d>& getDetectedPointsAll(){return detectedPoints_ALL;}
		cv::vector <int>& getInliers(){return Inlier;}
		cv::vector <double>& getErrorDist(){return error;}
		cv::vector <double>& getErrorUndist(){return errorUndistorted;}
		cv::vector <cv::Point2d>& getDetectedPointsUndistorted(){return detectedPointsUndistorted;}

		void setPointsUndistorted(cv::vector <cv::Point2d> & _detectedPoints,cv::vector <cv::Point2d> & _projectedPoints,cv::vector <bool> & _Inlier);
		void setPointsProjectedUndistorted(cv::vector <cv::Point2d> & _projectedPoints);
		void undistortPoints();

		void setMatrices(cv::Mat & _rotationvector,cv::Mat & _translationvector);
		cv::Mat getRotationVector ();
		cv::Mat getTranslationVector ();

		void draw(int type);
		void getDrawTextData(int type, bool distorted, std::vector<double>& x, std::vector<double>& y, std::vector<QString>& text, std::vector<bool>& inlier);
		void bindTexture(int type);

		cv::Mat getTransformationMatrix();

		void toggleInlier(double x, double y, bool isDistortedView);
		void setPointManual(double x, double y, bool isDistortedView);

		QString getFilenamePointsInlier();
		QString getFilenamePointsDetected();
		QString getFilenamePointsDetectedAll();
		QString getFilenameRotationMatrix();
		QString getFilenameTranslationVector();

		void savePointsInlier( QString filename);
		void savePointsDetected( QString filename);
		void savePointsDetectedAll( QString filename);
		void saveRotationMatrix( QString filename);
		void saveTranslationVector( QString filename);

		void loadPointsInlier( QString filename);
		void loadPointsDetected( QString filename);
		void loadPointsDetectedAll( QString filename);
		void loadRotationMatrix( QString filename);
		void loadTranslationVector( QString filename);

		void reset();

	private:	
		std::istream& comma(std::istream& in);
		std::istream& getline(std::istream &is, std::string &s);
		void savePoints(std::vector <cv::Point2d> &points, QString filename);
		void loadPoints(cv::vector <cv::Point2d> &points, QString filename);
		void drawPoints(std::vector <cv::Point2d> &points, bool drawAllPoints = false);
		void computeError();
		int calibrated;

		//Camera
		Camera* camera;
		int height,width;
		QString imageFileName;

		//Images
		Image* image;
		Image* undistortedImage;

		//Pose
		cv::Mat rotationvector;
		cv::Mat translationvector;

		//CalibrationInput
		cv::vector<cv::Point2d> detectedPoints_ALL;
		cv::vector<cv::Point2d> detectedPoints;
		cv::vector<cv::Point2d> projectedPoints;
		cv::vector<cv::Point2d> detectedPointsUndistorted;
		cv::vector<cv::Point2d> projectedPointsUndistorted;
		cv::vector<int> Inlier;
		cv::vector<double> error;
		cv::vector<double> errorUndistorted;
		int nbInlier;
};


#endif /* CALIBRATIONIMAGE_H_ */
