/*
 * Calibration.h
 *
 *  Created on: Nov 18, 2013
 *      Author: ben
 */

#ifndef CAMERA_H_
#define CAMERA_H_

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <QString>
#include <QStringList>
#include <vector>

#include <opencv/cv.h>

class Image;
class UndistortionObject;
class CalibrationImage;

class Camera{
	public:

		Camera(QString cameraName, int _id);

		void loadImages(QStringList fileNames);
		CalibrationImage* addImage(QString fileName);
		void loadUndistortionImage(QString undistortionImage);
		bool setResolutions();
		
		//Getter of private members
		UndistortionObject* getUndistortionObject(){return undistortionObject;}
		const std::vector<CalibrationImage*>& getCalibrationImages(){return calibrationImages;}
		bool hasUndistortion(){return undistortionObject!=NULL; }

		//Setter of private members
		void setIsLightCamera(bool value){lightCamera = value;}
		bool isLightCamera(){return lightCamera;}

		void setID(int value){id = value;}
		int getID(){return id;}

		QString getName(){return name;}
		~Camera();
		
		int getWidth(){return width;}
		int getHeight(){return height;}

		void save(QString folder);
		void loadTextures();

		void undistort();

		void setCalibrated(bool value){calibrated = value;}
		bool isCalibrated(){return calibrated;}

		void setCameraMatrix(cv::Mat & _cameramatrix);
		cv::Mat getCameraMatrix();

		void setRecalibrationRequired(int value){requiresRecalibration = value;}
		int isRecalibrationRequired(){return requiresRecalibration;}

		void setUpdateInfoRequired(bool value){updateInfoRequired = value;}
		int isUpdateInfoRequired(){return updateInfoRequired;}
		
		QString getFilenameCameraMatrix();
		void saveCameraMatrix( QString filename);
		void loadCameraMatrix( QString filename);

		void getMayaCam(double * out, int frame);
		void getDLT(double * out, int frame);

		void reset();
		void deleteFrame(int id);

	private:
		//InputData
		std::vector<CalibrationImage*> calibrationImages;
		UndistortionObject* undistortionObject;

		//Camera parameters
		int id;
		QString name;
		int height,width;
		bool lightCamera;
		
		//Calibrated Camera parameters
		bool calibrated;
		int requiresRecalibration;
		bool updateInfoRequired;


		cv::Mat cameramatrix;

		std::istream& comma(std::istream& in);
		std::istream& getline(std::istream &is, std::string &s);

		void paramsToMayaCam(double * out, cv::Mat _camera, cv::Mat _rotation, cv::Mat _translation);
};


#endif /* CALIBRATION_H_ */
