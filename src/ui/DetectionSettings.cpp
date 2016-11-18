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
///\file DetectionSettings.cpp
///\author Benjamin Knorlein
///\date 11/8/2016

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "core/Marker.h"
#include "core/Project.h"
#include "core/Trial.h"
#include "processing/MarkerDetection.h"
#include "ui/DetectionSettings.h"
#include "ui/MainWindow.h"
#include "ui/PointsDockWidget.h"
#include "ui_DetectionSettings.h"

using namespace xma;

DetectionSettings* DetectionSettings::instance = NULL;

DetectionSettings::DetectionSettings(QWidget* parent) :
	QDialog(parent),
	diag(new Ui::DetectionSettings)
{
	m_lastCam = -1;
	diag->setupUi(this);
	connect(PointsDockWidget::getInstance(), SIGNAL(activePointChanged(int)), this, SLOT(activePointChanged(int)));
	if (State::getInstance()->getActiveTrial() >= 0 && State::getInstance()->getActiveTrial() < Project::getInstance()->getTrials().size())
		setMarker(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarker());

	setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
}

void DetectionSettings::on_comboBox_currentIndexChanged(int index)
{
	if(m_images.size() > 0)
		diag->label_Image->setPixmap(getPixmap(m_images[diag->comboBox->currentIndex()]));
}

DetectionSettings::~DetectionSettings()
{
	delete diag;
	instance = NULL;
}

DetectionSettings* DetectionSettings::getInstance()
{
	if (!instance)
	{
		instance = new DetectionSettings(MainWindow::getInstance());
	}
	return instance;
}

void DetectionSettings::setMarker(Marker* marker)
{
	m_marker = marker;
	if (m_marker == NULL || (m_marker->getMethod() != 0 && m_marker->getMethod() != 2 && m_marker->getMethod() != 5))
	{
		m_marker = NULL;
		diag->spinBox_ThresholdOffset->setValue(0);
	}
	else{
		diag->spinBox_ThresholdOffset->setValue(marker->getThresholdOffset());
	}
}


void DetectionSettings::update(int camera, cv::Point2d center)
{
	if (m_marker != NULL){
		double m_input_size = (m_marker->getSizeOverride() > 0) ? m_marker->getSizeOverride() : (m_marker->getSize() > 0) ? m_marker->getSize() : 5;

		int trial = State::getInstance()->getActiveTrial();
		MarkerDetection::detectionPoint(Project::getInstance()->getTrials()[trial]->getVideoStreams()[camera]->getImage(), m_marker->getMethod(), center, 30.5, m_input_size, m_marker->getThresholdOffset(), NULL, &m_images);

		diag->label_Image->setPixmap(getPixmap(m_images[diag->comboBox->currentIndex()]));

		m_lastCam = camera;
		m_lastCenter = center;
	}
}

void DetectionSettings::closeEvent(QCloseEvent* e)
{
	setMarker(NULL);
	MainWindow::getInstance()->on_actionDetectionSettings_triggered(false);
}

void DetectionSettings::showEvent(QShowEvent* event)
{
	setMarker(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarker());
	diag->label_Image->setPixmap(QPixmap());
	m_lastCam = -1;
	m_images.clear();
}

QPixmap DetectionSettings::getPixmap(cv::Mat inMat)
{
	cv::Mat tmp;
	inMat.copyTo(tmp);
	cv::resize(inMat, tmp, cv::Size(0, 0), 3, 3, cv::INTER_NEAREST);

	QVector<QRgb>  sColorTable(256);
	for (int i = 0; i < 256; ++i)
	{
		sColorTable[i] = qRgb(i, i, i);
	}

	QImage image(tmp.data,
		tmp.cols, tmp.rows,
		static_cast<int>(tmp.step),
		QImage::Format_Indexed8);

	image.setColorTable(sColorTable);

	return QPixmap::fromImage(image);
}

void DetectionSettings::activePointChanged(int idx)
{
	if (this->isVisible()){
		setMarker(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarker());
		diag->label_Image->setPixmap(QPixmap());
		m_lastCam = -1;
		m_images.clear();
	}
}

void DetectionSettings::on_spinBox_ThresholdOffset_valueChanged(int value)
{
	if (m_marker != NULL)
	{
		m_marker->setThresholdOffset(diag->spinBox_ThresholdOffset->value());
		if (m_lastCam >= 0) update(m_lastCam, m_lastCenter);
	}
}
