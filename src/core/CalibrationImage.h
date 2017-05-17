//  ----------------------------------
//  XMALab -- Copyright © 2015, Brown University, Providence, RI.
//  
//  All Rights Reserved
//   
//  Use of the XMALab software is provided under the terms of the GNU General Public License version 3 
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
///\file CalibrationImage.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef CALIBRATIONIMAGE_H_
#define CALIBRATIONIMAGE_H_

#include <QString>

#include <opencv/cv.h>

namespace xma
{
	class Camera;
	class Image;

	class CalibrationImage
	{
	public:

		CalibrationImage(Camera* _camera, QString imageFileName, bool createImages = true);

		virtual ~CalibrationImage();

		int getWidth()
		{
			return width;
		}

		int getHeight()
		{
			return height;
		}

		QString getFilename();
		QString getFilenameBase();

		Image* getImage()
		{ 
			return image;
		}

		Image* getUndistortedImage()
		{
			return undistortedImage;
		}

		int isCalibrated()
		{
			return calibrated;
		}

		void setCalibrated(int value)
		{
			calibrated = value;
		}

		void loadTextures();
		void init(int nbPoints);

		void setDetectedPoints(cv::vector<cv::Point2d>& points);

		cv::vector<cv::Point2d>& getDetectedPointsAll()
		{
			return detectedPoints_ALL;
		}

		cv::vector<cv::Point2d>& getDetectedPoints()
		{
			return detectedPoints;
		}

		cv::vector<int>& getInliers()
		{
			return Inlier;
		}

		cv::vector<double>& getErrorDist()
		{
			return error;
		}

		cv::vector<double>& getErrorUndist()
		{
			return errorUndistorted;
		}

		cv::vector<cv::Point2d>& getDetectedPointsUndistorted()
		{
			return detectedPointsUndistorted;
		}

		void setPointsUndistorted(cv::vector<cv::Point2d>& _detectedPoints, cv::vector<cv::Point2d>& _projectedPoints, cv::vector<bool>& _Inlier);
		void setPointsProjectedUndistorted(cv::vector<cv::Point2d>& _projectedPoints);
		void undistortPoints();

		void setMatrices(cv::Mat& _rotationvector, cv::Mat& _translationvector);
		cv::Mat getRotationVector();
		cv::Mat getTranslationVector();

		void draw(int type);
		void getDrawTextData(int type, bool distorted, std::vector<double>& x, std::vector<double>& y, std::vector<QString>& text, std::vector<bool>& inlier);
		void bindTexture(int type);

		cv::Mat getTransformationMatrix();

		void toggleInlier(double x, double y, bool isDistortedView);
		void setInlier(int id, bool value);
		void setPoint(int idx, double x, double y, bool distorted);
		void setPointManual(double x, double y, bool isDistortedView);

		QString getFilenamePointsInlier();
		QString getFilenamePointsDetected();
		QString getFilenamePointsDetectedAll();
		QString getFilenameRotationMatrix();
		QString getFilenameTranslationVector();

		void savePointsInlier(QString filename);
		void savePointsDetected(QString filename);
		void savePointsDetectedAll(QString filename);
		void saveRotationMatrix(QString filename);
		void saveTranslationVector(QString filename);

		void loadPointsInlier(QString filename);
		void loadPointsDetected(QString filename);
		void loadPointsDetectedAll(QString filename);
		void loadRotationMatrix(QString filename);
		void loadTranslationVector(QString filename);

		void reset();
		void sortGridByReference(double x, double y);

		int getCalibrationNbInlier();
		double getCalibrationError();

	private:
		void savePoints(std::vector<cv::Point2d>& points, QString filename);
		void loadPoints(cv::vector<cv::Point2d>& points, QString filename);
		void drawPoints(std::vector<cv::Point2d>& points, bool drawAllPoints = false);
		void computeError();
		int calibrated;

		//Camera
		Camera* camera;
		int height, width;
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
}

#endif /* CALIBRATIONIMAGE_H_ */

