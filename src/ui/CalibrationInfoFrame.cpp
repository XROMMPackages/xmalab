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
///\file CalibrationInfoFrame.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/CalibrationInfoFrame.h"
#include "ui_CalibrationInfoFrame.h"
#include "ui/State.h"

#include "core/Camera.h"
#include "core/CalibrationImage.h"
#include "ui/CameraViewWidget.h"

using namespace xma;

CalibrationInfoFrame::CalibrationInfoFrame(QWidget* parent) :
	QFrame(parent),
	frame(new Ui::CalibrationInfoFrame)
{
	frame->setupUi(this);
	cameraWidget = ((CameraViewWidget *)parent);
}

CalibrationInfoFrame::~CalibrationInfoFrame()
{
	delete frame;
}

void CalibrationInfoFrame::update(Camera* camera)
{
	if (camera->isCalibrated())
	{
		if (camera->isUpdateInfoRequired())
		{
			QString CameraCenter;
			QString FocalLength;
			QString Distortion;
			QString FramesCalibrated;
			QString ErrorAllDist;
			QString ErrorAllUndist;


			getCameraInfo(camera, CameraCenter, FocalLength, Distortion, FramesCalibrated, ErrorAllDist, ErrorAllUndist);
			frame->label_CameraCenter->setText(CameraCenter);
			frame->label_FocalLength->setText(FocalLength);
			frame->label_Distortion->setText(Distortion);
			frame->label_FramesCalibrated->setText(FramesCalibrated);
			frame->label_ErrorAllDist->setText(ErrorAllDist);
			frame->label_ErrorAllUndist->setText(ErrorAllUndist);

			updateFrame(camera);

			camera->setUpdateInfoRequired(false);
		}
	}
	else
	{
		frame->label_CameraCenter->setText("");
		frame->label_FocalLength->setText("");
		frame->label_Distortion->setText("");
		frame->label_FramesCalibrated->setText("");
		frame->label_ErrorAllDist->setText("");
		frame->label_ErrorAllUndist->setText("");
	}
}

void CalibrationInfoFrame::getCameraInfo(Camera* camera, QString& CameraCenter, QString& FocalLength, QString& Distortion, QString& FramesCalibrated, QString& ErrorAllDist, QString& ErrorAllUndist)
{
	FocalLength = QString::number(camera->getCameraMatrix().at<double>(0, 0), 'f', 2) + QString(" , ") +
		QString::number(camera->getCameraMatrix().at<double>(1, 1), 'f', 2);

	CameraCenter = QString::number(camera->getCameraMatrix().at<double>(0, 2), 'f', 2) + QString(" , ") +
		QString::number(camera->getCameraMatrix().at<double>(1, 2), 'f', 2);

	Distortion = QString::number(camera->getDistortionCoefficiants().at<double>(0, 0), 'f', 5) + QString(" , ") +
		QString::number(camera->getDistortionCoefficiants().at<double>(1, 0), 'f', 5) + QString(" , ") +
		QString::number(camera->getDistortionCoefficiants().at<double>(2, 0), 'f', 5) + QString(" , ") +
		QString::number(camera->getDistortionCoefficiants().at<double>(3, 0), 'f', 5) + QString(" , ") +
		QString::number(camera->getDistortionCoefficiants().at<double>(4, 0), 'f', 5) + QString(" , ") +
		QString::number(camera->getDistortionCoefficiants().at<double>(5, 0), 'f', 5) + QString(" , ") +
		QString::number(camera->getDistortionCoefficiants().at<double>(6, 0), 'f', 5) + QString(" , ") +
		QString::number(camera->getDistortionCoefficiants().at<double>(7, 0), 'f', 5);;

	int countFrames = 0;
	int countInlier = 0;
	double meanDist = 0;
	double meanUndist = 0;

	for (unsigned int f = 0; f < camera->getCalibrationImages().size(); f++)
	{
		if (camera->getCalibrationImages()[f]->isCalibrated() == 1)
		{
			countFrames++;
			for (unsigned int pt = 0; pt < camera->getCalibrationImages()[f]->getErrorDist().size(); pt++)
			{
				if (camera->getCalibrationImages()[f]->getInliers()[pt] > 0)
				{
					countInlier ++;

					meanDist += camera->getCalibrationImages()[f]->getErrorDist()[pt];
					meanUndist += camera->getCalibrationImages()[f]->getErrorUndist()[pt];
				}
			}
		}
	}


	if (countInlier > 0) meanDist = meanDist / countInlier;
	if (countInlier > 0) meanUndist = meanUndist / countInlier;

	countInlier = 0;
	double sdDist = 0;
	double sdUndist = 0;
	for (unsigned int f = 0; f < camera->getCalibrationImages().size(); f++)
	{
		if (camera->getCalibrationImages()[f]->isCalibrated() == 1)
		{
			for (unsigned int pt = 0; pt < camera->getCalibrationImages()[f]->getErrorDist().size(); pt++)
			{
				if (camera->getCalibrationImages()[f]->getInliers()[pt] > 0)
				{
					countInlier++;

					sdDist += pow(meanDist - camera->getCalibrationImages()[f]->getErrorDist()[pt], 2);
					sdUndist += pow(meanUndist - camera->getCalibrationImages()[f]->getErrorUndist()[pt], 2);
				}
			}
		}
	}

	if (countInlier > 1)
	{
		sdDist = sqrt(sdDist / (countInlier - 1));
		sdUndist = sqrt(sdUndist / (countInlier - 1));
	}

	FramesCalibrated = QString::number(countFrames) + " of " + QString::number(camera->getCalibrationImages().size());
	ErrorAllUndist = QString::number(meanUndist, 'f', 2) + " +/- " + QString::number(sdUndist, 'f', 2);
	ErrorAllDist = QString::number(meanDist, 'f', 2) + " +/- " + QString::number(sdDist, 'f', 2);
}

void CalibrationInfoFrame::updateFrame(Camera* camera)
{
	if (camera->isCalibrated() && (camera->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->isCalibrated() == 1))
	{
		QString ErrorCurrentDist;
		QString ErrorCurrentUndist;
		QString RotationVector;
		QString TranslationVector;

		getInfoFrame(camera, State::getInstance()->getActiveFrameCalibration(), ErrorCurrentDist, ErrorCurrentUndist, RotationVector, TranslationVector);

		frame->label_ErrorCurrentDist->setText(ErrorCurrentDist);
		frame->label_ErrorCurrentUndist->setText(ErrorCurrentUndist);
		frame->label_RotationVector->setText(RotationVector);
		frame->label_TranslationVector->setText(TranslationVector);

		frame->label_Inlier->setText(getInfoInlier(camera, State::getInstance()->getActiveFrameCalibration()));
	}
	else
	{
		frame->label_Inlier->setText(getInfoInlier(camera, State::getInstance()->getActiveFrameCalibration()));

		frame->label_ErrorCurrentDist->setText("");
		frame->label_ErrorCurrentUndist->setText("");
		frame->label_RotationVector->setText("");
		frame->label_TranslationVector->setText("");
	}
}

void CalibrationInfoFrame::getInfoFrame(Camera* camera, int frame, QString& ErrorCurrentDist, QString& ErrorCurrentUndist, QString& RotationVector, QString& TranslationVector)
{
	RotationVector = QString::number(camera->getCalibrationImages()[frame]->getRotationVector().at<double>(0, 0), 'f', 2) + QString(" , ") +
		QString::number(camera->getCalibrationImages()[frame]->getRotationVector().at<double>(1, 0), 'f', 2) + QString(" , ") +
		QString::number(camera->getCalibrationImages()[frame]->getRotationVector().at<double>(2, 0), 'f', 2);

	TranslationVector = QString::number(camera->getCalibrationImages()[frame]->getTranslationVector().at<double>(0, 0), 'f', 2) + QString(" , ") +
		QString::number(camera->getCalibrationImages()[frame]->getTranslationVector().at<double>(1, 0), 'f', 2) + QString(" , ") +
		QString::number(camera->getCalibrationImages()[frame]->getTranslationVector().at<double>(2, 0), 'f', 2);

	int countInlier = 0;
	double meanDist = 0;
	double meanUndist = 0;

	if (camera->getCalibrationImages()[frame]->isCalibrated() == 1)
	{
		for (unsigned int pt = 0; pt < camera->getCalibrationImages()[frame]->getErrorDist().size(); pt++)
		{
			if (camera->getCalibrationImages()[frame]->getInliers()[pt] > 0)
			{
				countInlier ++;

				meanDist += camera->getCalibrationImages()[frame]->getErrorDist()[pt];
				meanUndist += camera->getCalibrationImages()[frame]->getErrorUndist()[pt];
			}
		}
	}


	if (countInlier > 0) meanDist = meanDist / countInlier;
	if (countInlier > 0) meanUndist = meanUndist / countInlier;

	countInlier = 0;
	double sdDist = 0;
	double sdUndist = 0;
	if (camera->getCalibrationImages()[frame]->isCalibrated() == 1)
	{
		for (unsigned int pt = 0; pt < camera->getCalibrationImages()[frame]->getErrorDist().size(); pt++)
		{
			if (camera->getCalibrationImages()[frame]->getInliers()[pt] > 0)
			{
				countInlier ++;

				sdDist += pow(meanDist - camera->getCalibrationImages()[frame]->getErrorDist()[pt], 2);
				sdUndist += pow(meanUndist - camera->getCalibrationImages()[frame]->getErrorUndist()[pt], 2);
			}
		}
	}


	if (countInlier > 1) sdDist = sqrt(sdDist / (countInlier - 1));
	if (countInlier > 1) sdUndist = sqrt(sdUndist / (countInlier - 1));

	ErrorCurrentUndist = QString::number(meanUndist, 'f', 2) + " +/- " + QString::number(sdUndist, 'f', 2);
	ErrorCurrentDist = QString::number(meanDist, 'f', 2) + " +/- " + QString::number(sdDist, 'f', 2);
}


QString CalibrationInfoFrame::getInfoInlier(Camera* camera, int frame)
{
	int countInlierAll = 0;
	int countInlier = 0;
	for (unsigned int f = 0; f < camera->getCalibrationImages().size(); f++)
	{
		if (camera->getCalibrationImages()[f]->isCalibrated() == 1)
		{
			for (unsigned int pt = 0; pt < camera->getCalibrationImages()[f]->getInliers().size(); pt++)
			{
				if (camera->getCalibrationImages()[f]->getInliers()[pt] > 0)
				{
					countInlierAll ++;
					if (frame == f) countInlier++;
				}
			}
		}
	}
	return QString::number(countInlier) + " / " + QString::number(countInlierAll);
}

void CalibrationInfoFrame::reset()
{
	frame->horizontalSliderBias->setValue(0.0);
	frame->horizontalSliderScale->setValue(100.0);
	frame->doubleSpinBoxBias->setValue(0.0);
	frame->doubleSpinBoxScale->setValue(1.0);
	cameraWidget->setScale(1.0);
	cameraWidget->setBias(0.0);
}

void CalibrationInfoFrame::on_doubleSpinBoxBias_valueChanged(double value)
{
	if (frame->horizontalSliderBias->value() != value * 100)
	{
		frame->horizontalSliderBias->setValue(value * 100);
		cameraWidget->setBias(value);
	}
}

void CalibrationInfoFrame::on_horizontalSliderBias_valueChanged(int value)
{
	if (frame->doubleSpinBoxBias->value() * 100 != value)
	{
		frame->doubleSpinBoxBias->setValue(0.01 * value);
		cameraWidget->setBias(0.01 * value);
	}
}

void CalibrationInfoFrame::on_doubleSpinBoxScale_valueChanged(double value)
{
	if (frame->horizontalSliderScale->value() != value * 100)
	{
		frame->horizontalSliderScale->setValue(value * 100);
		cameraWidget->setScale(value);
	}
}

void CalibrationInfoFrame::on_horizontalSliderScale_valueChanged(int value)
{
	if (frame->doubleSpinBoxScale->value() * 100 != value)
	{
		frame->doubleSpinBoxScale->setValue(0.01 * value);
		cameraWidget->setScale(0.01 * value);
	}
}