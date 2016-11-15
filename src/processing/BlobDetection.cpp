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
///\file BlobDetection.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "processing/BlobDetection.h" 

#include "ui/ProgressDialog.h"
#include "ui/MainWindow.h"

#include "core/Project.h"
#include "core/Camera.h"
#include "core/Image.h"
#include "core/CalibrationImage.h"
#include "core/CalibrationObject.h"
#include "core/UndistortionObject.h"
#include "core/Settings.h"
#include "core/CalibrationSequence.h"

#include <QtCore>

using namespace xma;

BlobDetection::BlobDetection(int camera, int image): ThreadedProcessing("Detect Points")
{
	m_camera = camera;
	m_image = image;
}

BlobDetection::~BlobDetection()
{
}

void BlobDetection::process()
{
	tmpPoints.clear();
	cv::Mat image;
	if (m_image < 0)
	{
		Project::getInstance()->getCameras()[m_camera]->getUndistortionObject()->getImage()->getImage(image);
	}
	else
	{
		Project::getInstance()->getCameras()[m_camera]->getCalibrationSequence()->getImage(m_image,true)->getImage(image);
	}

	cv::SimpleBlobDetector::Params paramsBlob;

	paramsBlob.thresholdStep = Settings::getInstance()->getFloatSetting("BlobDetectorThresholdStep");
	paramsBlob.minThreshold = Settings::getInstance()->getFloatSetting("BlobDetectorMinThreshold");
	paramsBlob.maxThreshold = Settings::getInstance()->getFloatSetting("BlobDetectorMaxThreshold");
	paramsBlob.minRepeatability = Settings::getInstance()->getIntSetting("BlobDetectorMinRepeatability");
	paramsBlob.minDistBetweenBlobs = Settings::getInstance()->getFloatSetting("BlobDetectorMinDistBetweenBlobs");

	paramsBlob.filterByColor = Settings::getInstance()->getBoolSetting("BlobDetectorFilterByColor");
	if (m_image < 0 || CalibrationObject::getInstance()->hasWhiteBlobs())
	{
		paramsBlob.blobColor = Settings::getInstance()->getIntSetting("BlobDetectorBlobColor");
	}
	else
	{
		paramsBlob.blobColor = 255 - Settings::getInstance()->getIntSetting("BlobDetectorBlobColor");
	}

	paramsBlob.filterByArea = Settings::getInstance()->getBoolSetting("BlobDetectorFilterByArea");
	paramsBlob.minArea = Settings::getInstance()->getFloatSetting("BlobDetectorMinArea");
	paramsBlob.maxArea = Settings::getInstance()->getFloatSetting("BlobDetectorMaxArea");

	paramsBlob.filterByCircularity = Settings::getInstance()->getBoolSetting("BlobDetectorFilterByCircularity");
	paramsBlob.minCircularity = Settings::getInstance()->getFloatSetting("BlobDetectorMinCircularity");
	paramsBlob.maxCircularity = Settings::getInstance()->getFloatSetting("BlobDetectorMaxCircularity");

	paramsBlob.filterByInertia = Settings::getInstance()->getBoolSetting("BlobDetectorFilterByInertia");
	paramsBlob.minInertiaRatio = Settings::getInstance()->getFloatSetting("BlobDetectorMinInertiaRatio");
	paramsBlob.maxInertiaRatio = Settings::getInstance()->getFloatSetting("BlobDetectorMaxInertiaRatio");

	paramsBlob.filterByConvexity = Settings::getInstance()->getBoolSetting("BlobDetectorFilterByConvexity");
	paramsBlob.minConvexity = Settings::getInstance()->getFloatSetting("BlobDetectorMinConvexity");
	paramsBlob.maxConvexity = Settings::getInstance()->getFloatSetting("BlobDetectorMaxConvexity");

	cv::FeatureDetector* detector = new cv::SimpleBlobDetector(paramsBlob);
	cv::vector<cv::KeyPoint> keypoints;

	detector->detect(image, keypoints);

	for (unsigned int i = 0; i < keypoints.size(); i++)
	{
		tmpPoints.push_back(cv::Point2d(keypoints[i].pt.x, keypoints[i].pt.y));
	}
	image.release();
}

void BlobDetection::process_finished()
{
	if (m_image < 0)
	{
		Project::getInstance()->getCameras()[m_camera]->getUndistortionObject()->setDetectedPoints(tmpPoints);
	}
	else
	{
		Project::getInstance()->getCameras()[m_camera]->getCalibrationImages()[m_image]->setDetectedPoints(tmpPoints);
	}

	tmpPoints.clear();
}

