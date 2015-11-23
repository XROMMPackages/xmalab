//  ----------------------------------
//  XMA Lab -- Copyright © 2015, Brown University, Providence, RI.
//  
//  All Rights Reserved
//   
//  Use of the XMA Lab software is provided under the terms of the GNU General Public License version 3 
//  as published by the Free Software Foundation at http://www.gnu.org/licenses/gpl-3.0.html, provided 
//  that this copyright notice appear in all copies and that the name of Brown University not be used in 
//  advertising or publicity pertaining to the use or distribution of the software without specific written 
//  prior permission from Brown University.
//  
//  See license.txt for further information.
//  
//  BROWN UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE WHICH IS 
//  PROVIDED “AS IS”, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
//  FOR ANY PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR ANY 
//  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR FOR ANY DAMAGES WHATSOEVER RESULTING 
//  FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
//  OTHER TORTIOUS ACTION, OR ANY OTHER LEGAL THEORY, ARISING OUT OF OR IN CONNECTION 
//  WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
//  ----------------------------------
//  
///\file Camera.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef CAMERA_H_
#define CAMERA_H_

#include <QString>
#include <QStringList>
#include <vector>

#include <opencv/cv.h>

namespace xma
{
	class Image;
	class UndistortionObject;
	class CalibrationImage;

	class Camera
	{
	public:

		Camera(QString cameraName, int _id);
		virtual ~Camera();

		void loadImages(QStringList fileNames);
		CalibrationImage* addImage(QString fileName);
		void loadUndistortionImage(QString undistortionImage);
		bool setResolutions();

		//Getter of private members
		UndistortionObject* getUndistortionObject()
		{
			return undistortionObject;
		}

		const std::vector<CalibrationImage*>& getCalibrationImages()
		{
			return calibrationImages;
		}

		bool hasUndistortion()
		{
			return undistortionObject != NULL;
		}

		//Setter of private members
		void setIsLightCamera(bool value)
		{
			lightCamera = value;
		}

		bool isLightCamera()
		{
			return lightCamera;
		}

		void setID(int value)
		{
			id = value;
		}

		int getID()
		{
			return id;
		}

		QString getName()
		{
			return name;
		}

		int getWidth()
		{
			return width;
		}

		int getHeight()
		{
			return height;
		}

		void save(QString folder);
		void loadTextures();

		void undistort();
		cv::Point2d undistortPoint(cv::Point2d pt, bool undistort, bool withModel = true);
		cv::Point2d applyModelDistortion(cv::Point2d pt, bool undistort);

		void setCalibrated(bool value);

		bool isCalibrated()
		{
			return calibrated;
		}

		void setCameraMatrix(cv::Mat& _cameramatrix);
		cv::Mat getCameraMatrix();

		void setDistortionCoefficiants(cv::Mat& _distortion_coeff);
		void resetDistortion();

		cv::Mat getDistortionCoefficiants();
		bool hasModelDistortion();

		cv::Mat getProjectionMatrix(int referenceFrame);

		void setRecalibrationRequired(int value)
		{
			requiresRecalibration = value;
		}

		int isRecalibrationRequired()
		{
			return requiresRecalibration;
		}

		void setUpdateInfoRequired(bool value)
		{
			updateInfoRequired = value;
		}

		int isUpdateInfoRequired()
		{
			return updateInfoRequired;
		}

		bool isOptimized()
		{
			return optimized;
		}

		void setOptimized(bool value)
		{
			optimized = value;
		}

		QString getFilenameCameraMatrix();
		void saveCameraMatrix(QString filename);
		void loadCameraMatrix(QString filename);

		QString getFilenameUndistortionParam();
		void saveUndistortionParam(QString filename);
		void loadUndistortionParam(QString filename);

		void saveMayaCamVersion2(int ImageId, QString filename);
		void getMayaCam(double* out, int frame);
		void getDLT(double* out, int frame);

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
		bool optimized;

		cv::Mat cameramatrix;
		cv::Mat distortion_coeffs;
		bool model_distortion;

		cv::Mat undistortionMapX;
		cv::Mat undistortionMapY;

		void paramsToMayaCam(double* out, cv::Mat _camera, cv::Mat _rotation, cv::Mat _translation);
	};
}

#endif /* CALIBRATION_H_ */

