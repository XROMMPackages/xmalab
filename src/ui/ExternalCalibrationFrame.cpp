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
///\file ExternalCalibrationFrame.cpp
///\author Benjamin Knorlein
///\date 05/08/2019

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/ExternalCalibrationFrame.h"
#include "ui_ExternalCalibrationFrame.h"
#include "core/Camera.h"
#include "core/CalibrationImage.h"
#include <QFileDialog>
#include "core/Settings.h"
#include "MainWindow.h"

using namespace xma;

ExternalCalibrationFrame::ExternalCalibrationFrame(QWidget* parent) :
	QFrame(parent),
	frame(new Ui::ExternalCalibrationFrame)
{
	frame->setupUi(this);
}

void ExternalCalibrationFrame::setCamera(Camera* cam)
{
	m_cam = cam;
	update();
}

void ExternalCalibrationFrame::update()
{
	if (!m_cam || m_cam->getCalibrationImages().size() <= 0)
		return;
	
	frame->spinBox_width->setValue(m_cam->getWidth());
	frame->spinBox_height->setValue(m_cam->getHeight());

	frame->doubleSpinBox_K11->setValue(m_cam->getCameraMatrix().at<double>(0, 0));
	frame->doubleSpinBox_K12->setValue(m_cam->getCameraMatrix().at<double>(0, 1));
	frame->doubleSpinBox_K13->setValue(m_cam->getCameraMatrix().at<double>(0, 2));

	frame->doubleSpinBox_K21->setValue(m_cam->getCameraMatrix().at<double>(1, 0));
	frame->doubleSpinBox_K22->setValue(m_cam->getCameraMatrix().at<double>(1, 1));
	frame->doubleSpinBox_K23->setValue(m_cam->getCameraMatrix().at<double>(1, 2));

	frame->doubleSpinBox_K31->setValue(m_cam->getCameraMatrix().at<double>(2, 0));
	frame->doubleSpinBox_K32->setValue(m_cam->getCameraMatrix().at<double>(2, 1));
	frame->doubleSpinBox_K33->setValue(m_cam->getCameraMatrix().at<double>(2, 2));

	cv::Mat rotationmatrix;
	rotationmatrix.create(3, 3, CV_64F);
	cv::Rodrigues(m_cam->getCalibrationImages()[0]->getRotationVector(), rotationmatrix);
	
	frame->doubleSpinBox_R11->setValue(rotationmatrix.at<double>(0, 0));
	frame->doubleSpinBox_R12->setValue(rotationmatrix.at<double>(0, 1));
	frame->doubleSpinBox_R13->setValue(rotationmatrix.at<double>(0, 2));
	
	frame->doubleSpinBox_R21->setValue(rotationmatrix.at<double>(1, 0));
	frame->doubleSpinBox_R22->setValue(rotationmatrix.at<double>(1, 1));
	frame->doubleSpinBox_R23->setValue(rotationmatrix.at<double>(1, 2));

	frame->doubleSpinBox_R31->setValue(rotationmatrix.at<double>(2, 0));
	frame->doubleSpinBox_R32->setValue(rotationmatrix.at<double>(2, 1));
	frame->doubleSpinBox_R33->setValue(rotationmatrix.at<double>(2, 2));

	frame->doubleSpinBox_TX->setValue(m_cam->getCalibrationImages()[0]->getTranslationVector().at<double>(0, 0));
	frame->doubleSpinBox_TY->setValue(m_cam->getCalibrationImages()[0]->getTranslationVector().at<double>(1, 0));
	frame->doubleSpinBox_TZ->setValue(m_cam->getCalibrationImages()[0]->getTranslationVector().at<double>(2, 0));

	frame->doubleSpinBox_k1->setValue(m_cam->getDistortionCoefficiants().at<double>(0, 0));
	frame->doubleSpinBox_k2->setValue(m_cam->getDistortionCoefficiants().at<double>(1, 0));
	frame->doubleSpinBox_p1->setValue(m_cam->getDistortionCoefficiants().at<double>(2, 0));
	frame->doubleSpinBox_p2->setValue(m_cam->getDistortionCoefficiants().at<double>(3, 0));
	frame->doubleSpinBox_k3->setValue(m_cam->getDistortionCoefficiants().at<double>(4, 0));
	frame->doubleSpinBox_k4->setValue(m_cam->getDistortionCoefficiants().at<double>(5, 0));
	frame->doubleSpinBox_k5->setValue(m_cam->getDistortionCoefficiants().at<double>(6, 0));
	frame->doubleSpinBox_k6->setValue(m_cam->getDistortionCoefficiants().at<double>(7, 0));

	if (m_cam->hasModelDistortion())
	{
		frame->checkBox_hasDistortion->setChecked(true);
	}
	else
	{
		frame->checkBox_hasDistortion->setChecked(false);

	}
	frame->frame_distortion->setVisible(frame->checkBox_hasDistortion->isChecked());
}

void ExternalCalibrationFrame::on_pushButton_Apply_clicked()
{
	cv::Mat cameramatrix;
	cameramatrix.create(3, 3, CV_64F);
	cv::Mat distortion_coeffs = cv::Mat::zeros(8, 1, CV_64F);
	cv::Mat rotationvector;
	rotationvector.create(3, 1, CV_64F);
	cv::Mat rotationmatrix;
	rotationmatrix.create(3, 3, CV_64F);
	cv::Mat translationvector;
	translationvector.create(3, 1, CV_64F);

	cameramatrix.at<double>(0, 0) = frame->doubleSpinBox_K11->value();
	cameramatrix.at<double>(0, 1) = frame->doubleSpinBox_K12->value();
	cameramatrix.at<double>(0, 2) = frame->doubleSpinBox_K13->value();

	cameramatrix.at<double>(1, 0) = frame->doubleSpinBox_K21->value();
	cameramatrix.at<double>(1, 1) = frame->doubleSpinBox_K22->value();
	cameramatrix.at<double>(1, 2) = frame->doubleSpinBox_K23->value();

	cameramatrix.at<double>(2, 0) = frame->doubleSpinBox_K31->value();
	cameramatrix.at<double>(2, 1) = frame->doubleSpinBox_K32->value();
	cameramatrix.at<double>(2, 2) = frame->doubleSpinBox_K33->value();

	rotationmatrix.at<double>(0, 0) = frame->doubleSpinBox_R11->value();
	rotationmatrix.at<double>(0, 1) = frame->doubleSpinBox_R12->value();
	rotationmatrix.at<double>(0, 2) = frame->doubleSpinBox_R13->value();

	rotationmatrix.at<double>(1, 0) = frame->doubleSpinBox_R21->value();
	rotationmatrix.at<double>(1, 1) = frame->doubleSpinBox_R22->value();
	rotationmatrix.at<double>(1, 2) = frame->doubleSpinBox_R23->value();

	rotationmatrix.at<double>(2, 0) = frame->doubleSpinBox_R31->value();
	rotationmatrix.at<double>(2, 1) = frame->doubleSpinBox_R32->value();
	rotationmatrix.at<double>(2, 2) = frame->doubleSpinBox_R33->value();

	cv::Rodrigues(rotationmatrix, rotationvector);

	translationvector.at<double>(0, 0) = frame->doubleSpinBox_TX->value();
	translationvector.at<double>(1, 0) = frame->doubleSpinBox_TY->value();
	translationvector.at<double>(2, 0) = frame->doubleSpinBox_TZ->value();

	m_cam->setResolution(frame->spinBox_width->value(), frame->spinBox_height->value());
	m_cam->setCameraMatrix(cameramatrix);
	m_cam->getCalibrationImages()[0]->setMatrices(rotationvector, translationvector);

	distortion_coeffs.at<double>(0, 0) = frame->doubleSpinBox_k1->value();
	distortion_coeffs.at<double>(1, 0) = frame->doubleSpinBox_k2->value();
	distortion_coeffs.at<double>(2, 0) = frame->doubleSpinBox_p1->value();
	distortion_coeffs.at<double>(3, 0) = frame->doubleSpinBox_p2->value();
	distortion_coeffs.at<double>(4, 0) = frame->doubleSpinBox_k3->value();
	distortion_coeffs.at<double>(5, 0) = frame->doubleSpinBox_k4->value();
	distortion_coeffs.at<double>(6, 0) = frame->doubleSpinBox_k5->value();
	distortion_coeffs.at<double>(7, 0) = frame->doubleSpinBox_k6->value();

	if (frame->checkBox_hasDistortion->isChecked())
	{
		m_cam->setDistortionCoefficiants(distortion_coeffs);
	} 
	else
	{
		m_cam->resetDistortion();
	}
		
	m_cam->setRecalibrationRequired(0);

	MainWindow::getInstance()->updateCamera(m_cam->getID());
}

void ExternalCalibrationFrame::on_pushButton_MayaCam_clicked()
{
	QString filename = QFileDialog::getOpenFileName(this,
		tr("Open MayaCam 2.0 file"), Settings::getInstance()->getLastUsedDirectory(), tr("Text file (*.txt)"));

	if (!filename.isEmpty())
	{
		
		FILE * pfile = fopen(filename.toStdString().c_str(), "r");
		fscanf(pfile, "image size\n");
		uint w, h;
		fscanf(pfile, "%d,%d\n",&w,&h);
		std::cerr << w << " " << h << std::endl;
		fscanf(pfile, "\ncamera matrix\n");
		float C11,C12,C13,C21,C22,C23,C31,C32,C33;
		fscanf(pfile, "%f,%f,%f\n", &C11, &C12, &C13);
		fscanf(pfile, "%f,%f,%f\n", &C21, &C22, &C23);
		fscanf(pfile, "%f,%f,%f\n", &C31, &C32, &C33);

		fscanf(pfile, "\nrotation\n");
		float R11,R12,R13,R21,R22,R23,R31,R32,R33;
		fscanf(pfile, "%f,%f,%f\n", &R11, &R12, &R13);
		fscanf(pfile, "%f,%f,%f\n", &R21, &R22, &R23);
		fscanf(pfile, "%f,%f,%f\n", &R31, &R32, &R33);

		fscanf(pfile, "\ntranslation\n");
		float tx, ty, tz;
		fscanf(pfile, "%f\n", &tx);
		fscanf(pfile, "%f\n", &ty);
		fscanf(pfile, "%f\n", &tz);
		
		frame->spinBox_width->setValue(w);
		frame->spinBox_height->setValue(h);

		frame->doubleSpinBox_K11->setValue(C11);
		frame->doubleSpinBox_K12->setValue(C12);
		frame->doubleSpinBox_K13->setValue(C13);

		frame->doubleSpinBox_K21->setValue(C21);
		frame->doubleSpinBox_K22->setValue(C22);
		frame->doubleSpinBox_K23->setValue(C23);

		frame->doubleSpinBox_K31->setValue(C31);
		frame->doubleSpinBox_K32->setValue(C32);
		frame->doubleSpinBox_K33->setValue(C33);

		frame->doubleSpinBox_R11->setValue(R11);
		frame->doubleSpinBox_R12->setValue(R12);
		frame->doubleSpinBox_R13->setValue(R13);

		frame->doubleSpinBox_R21->setValue(R21);
		frame->doubleSpinBox_R22->setValue(R22);
		frame->doubleSpinBox_R23->setValue(R23);

		frame->doubleSpinBox_R31->setValue(R31);
		frame->doubleSpinBox_R32->setValue(R32);
		frame->doubleSpinBox_R33->setValue(R33);

		frame->doubleSpinBox_TX->setValue(tx);
		frame->doubleSpinBox_TY->setValue(ty);
		frame->doubleSpinBox_TZ->setValue(tz);

		if (fscanf(pfile, "\nundistortion\n") < 0)
		{
			frame->checkBox_hasDistortion->setChecked(false);		
			frame->doubleSpinBox_k1->setValue(0);
			frame->doubleSpinBox_k2->setValue(0);
			frame->doubleSpinBox_p1->setValue(0);
			frame->doubleSpinBox_p2->setValue(0);
			frame->doubleSpinBox_k3->setValue(0);
			frame->doubleSpinBox_k4->setValue(0);
			frame->doubleSpinBox_k5->setValue(0);
			frame->doubleSpinBox_k6->setValue(0);
		}
		else
		{
			frame->checkBox_hasDistortion->setChecked(true);
			float k1, k2, p1, p2, k3, k4, k5, k6;
			fscanf(pfile, "%f,%f,%f,%f,%f,%f,%f,%f\n", &k1, &k2, &p1,&p2,&k3,&k4,&k5,&k6);
			frame->doubleSpinBox_k1->setValue(k1);
			frame->doubleSpinBox_k2->setValue(k2);
			frame->doubleSpinBox_p1->setValue(p1);
			frame->doubleSpinBox_p2->setValue(p2);
			frame->doubleSpinBox_k3->setValue(k3);
			frame->doubleSpinBox_k4->setValue(k4);
			frame->doubleSpinBox_k5->setValue(k5);
			frame->doubleSpinBox_k6->setValue(k6);
		}
	}
}

void ExternalCalibrationFrame::on_checkBox_hasDistortion_stateChanged(int state)
{
	frame->frame_distortion->setVisible(frame->checkBox_hasDistortion->isChecked());
}

ExternalCalibrationFrame::~ExternalCalibrationFrame()
{
	delete frame;
}
