//  ----------------------------------
//  XMALab -- Copyright � 2015, Brown University, Providence, RI.
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
//  PROVIDED �AS IS�, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
//  FOR ANY PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR ANY 
//  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR FOR ANY DAMAGES WHATSOEVER RESULTING 
//  FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
//  OTHER TORTIOUS ACTION, OR ANY OTHER LEGAL THEORY, ARISING OUT OF OR IN CONNECTION 
//  WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
//  ----------------------------------
//  
///\file CameraViewDetailWidget.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/CameraViewDetailWidget.h"
#include "ui_CameraViewDetailWidget.h"
#include "ui/PointsDockWidget.h"
#include "core/Camera.h"


using namespace xma;

CameraViewDetailWidget::CameraViewDetailWidget(Camera* _camera, QWidget* parent) :
	QWidget(parent),
	widget(new Ui::CameraViewDetailWidget),
	m_visible(true)
{
	camera = _camera;
	widget->setupUi(this);
	widget->glCameraView->setCamera(camera);
	widget->glCameraView->setDetailedView();

	connect(State::getInstance(), SIGNAL(activeTrialChanged(int)), this, SLOT(activeTrialChanged(int)));
	connect(State::getInstance(), SIGNAL(activeFrameTrialChanged(int)), this, SLOT(activeFrameTrialChanged(int)));
	connect(State::getInstance(), SIGNAL(workspaceChanged(work_state)), this, SLOT(workspaceChanged(work_state)));
	connect(PointsDockWidget::getInstance(), SIGNAL(activePointChanged(int)), this, SLOT(activePointChanged(int)));
}

CameraViewDetailWidget::~CameraViewDetailWidget()
{
}

void CameraViewDetailWidget::updateCamera()
{
	widget->glCameraView->setCamera(camera);
}

void CameraViewDetailWidget::draw()
{
	if (m_visible) widget->glCameraView->update();
}

void CameraViewDetailWidget::centerViews()
{
	widget->glCameraView->centerViewToPoint();
	if (m_visible) widget->glCameraView->update();
}

void CameraViewDetailWidget::setMinimumWidthGL(bool set)
{
	widget->glCameraView->setMinimumWidthGL(set);
}


bool CameraViewDetailWidget::getIsVisible() const
{
	return m_visible;
}

void CameraViewDetailWidget::setIsVisible(bool value)
{
	m_visible = value;
}

void CameraViewDetailWidget::on_doubleSpinBoxBias_valueChanged(double value)
{
	if (widget->horizontalSliderBias->value() != value * 100)
	{
		widget->horizontalSliderBias->setValue(value * 100);
		widget->glCameraView->setBias(value);
	}
}

void CameraViewDetailWidget::on_horizontalSliderBias_valueChanged(int value)
{
	if (widget->doubleSpinBoxBias->value() * 100 != value)
	{
		widget->doubleSpinBoxBias->setValue(0.01 * value);
		widget->glCameraView->setBias(0.01 * value);
	}
}

void CameraViewDetailWidget::on_doubleSpinBoxScale_valueChanged(double value)
{
	if (widget->horizontalSliderScale->value() != value * 100)
	{
		widget->horizontalSliderScale->setValue(value * 100);
		widget->glCameraView->setScale(value);
	}
}

void CameraViewDetailWidget::on_horizontalSliderScale_valueChanged(int value)
{
	if (widget->doubleSpinBoxScale->value() * 100 != value)
	{
		widget->doubleSpinBoxScale->setValue(0.01 * value);
		widget->glCameraView->setScale(0.01 * value);
	}
}


void CameraViewDetailWidget::workspaceChanged(work_state workspace)
{
	widget->glCameraView->centerViewToPoint(true);
	if (m_visible) widget->glCameraView->update();
}

void CameraViewDetailWidget::activeFrameTrialChanged(int)
{
	if (!State::getInstance()->getDisableDraw())
	{
		widget->glCameraView->centerViewToPoint();
		if (m_visible) widget->glCameraView->update();
	}
}

void CameraViewDetailWidget::activeTrialChanged(int)
{
	widget->glCameraView->centerViewToPoint(true);
	if (m_visible) widget->glCameraView->update();
}

void CameraViewDetailWidget::activePointChanged(int)
{
	widget->glCameraView->centerViewToPoint(true);
	if (m_visible) widget->glCameraView->update();
}

