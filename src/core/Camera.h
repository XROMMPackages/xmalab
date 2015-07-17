#ifndef CAMERA_H_
#define CAMERA_H_

#include <QString>
#include <QStringList>
#include <vector>

#include <opencv/cv.h>

namespace xma{
	class Image;
	class UndistortionObject;
	class CalibrationImage;

	class Camera{
	public:

		Camera(QString cameraName, int _id);
		virtual ~Camera();

		void loadImages(QStringList fileNames);
		CalibrationImage* addImage(QString fileName);
		void loadUndistortionImage(QString undistortionImage);
		bool setResolutions();

		//Getter of private members
		UndistortionObject* getUndistortionObject(){ return undistortionObject; }
		const std::vector<CalibrationImage*>& getCalibrationImages(){ return calibrationImages; }
		bool hasUndistortion(){ return undistortionObject != NULL; }

		//Setter of private members
		void setIsLightCamera(bool value){ lightCamera = value; }
		bool isLightCamera(){ return lightCamera; }

		void setID(int value){ id = value; }
		int getID(){ return id; }

		QString getName(){ return name; }

		int getWidth(){ return width; }
		int getHeight(){ return height; }

		void save(QString folder);
		void loadTextures();

		void undistort();

		void setCalibrated(bool value);
		bool isCalibrated(){ return calibrated; }

		void setCameraMatrix(cv::Mat & _cameramatrix);
		cv::Mat getCameraMatrix();
		void setDistortionCoefficiants(cv::Mat & _distortion_coeff);
		cv::Mat getDistortionCoefficiants();
		bool hasModelDistortion();
		cv::Mat getProjectionMatrix(int referenceFrame);

		void setRecalibrationRequired(int value){ requiresRecalibration = value; }
		int isRecalibrationRequired(){ return requiresRecalibration; }

		void setUpdateInfoRequired(bool value){ updateInfoRequired = value; }
		int isUpdateInfoRequired(){ return updateInfoRequired; }

		QString getFilenameCameraMatrix();
		void saveCameraMatrix(QString filename);
		void loadCameraMatrix(QString filename);

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
		int height, width;
		bool lightCamera;

		//Calibrated Camera parameters
		bool calibrated;
		int requiresRecalibration;
		bool updateInfoRequired;

		cv::Mat cameramatrix;
		cv::Mat distortion_coeffs;
		bool model_distortion;

		void paramsToMayaCam(double * out, cv::Mat _camera, cv::Mat _rotation, cv::Mat _translation);
	};
}

#endif /* CALIBRATION_H_ */
