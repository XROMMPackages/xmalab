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
///\file CameraViewWidget.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/CameraViewWidget.h"
#include "ui_CameraViewWidget.h"
#include "ui/UndistortionInfoFrame.h"
#include "ui/CalibrationInfoFrame.h"
#include "ui/DigitizationInfoFrame.h"

#include "core/Camera.h"

using namespace xma;

CameraViewWidget::CameraViewWidget(Camera* _camera, QWidget* parent) :
QWidget(parent),
widget(new Ui::CameraViewWidget),
m_visible(true)
{
	camera = _camera;

	widget->setupUi(this);
	cameraName = camera->getName();
	widget->glCameraView->setCamera(camera);
	widget->frameInfo->hide();

	undistortionFrame = new UndistortionInfoFrame(this);
	calibrationFrame = new CalibrationInfoFrame(this);
	digitizationFrame = new DigitizationInfoFrame(this);

	widget->gridLayout_3->addWidget(undistortionFrame, 0, 0, 1, 1);
	widget->gridLayout_3->addWidget(calibrationFrame, 1, 0, 1, 1);
	widget->gridLayout_3->addWidget(digitizationFrame, 2, 0, 1, 1);

	connect(widget->glCameraView, SIGNAL(autozoomChanged(bool)), this, SLOT(autozoomChanged(bool)));
	connect(widget->glCameraView, SIGNAL(zoomChanged(int)), this, SLOT(zoomChanged(int)));
	connect(widget->glCameraView, SIGNAL(transparencyChanged(double)), this, SLOT(transparencyChanged(double)));

	connect(State::getInstance(), SIGNAL(workspaceChanged(work_state)), this, SLOT(workspaceChanged(work_state)));
	connect(State::getInstance(), SIGNAL(activeFrameCalibrationChanged(int)), this, SLOT(activeFrameCalibrationChanged(int)));
	connect(State::getInstance(), SIGNAL(activeCameraChanged(int)), this, SLOT(activeCameraChanged(int)));

	this->installEventFilter(this);
	widget->toolButtonInfo->installEventFilter(this);
	widget->spinBoxZoom->installEventFilter(this);
	widget->toolButtonFitZoom->installEventFilter(this);
	widget->frameInfo->installEventFilter(this);
	widget->glCameraView->installEventFilter(this);

	calibrationFrame->installEventFilter(this);
	undistortionFrame->installEventFilter(this);
	digitizationFrame->installEventFilter(this);

	if (camera->getID() == State::getInstance()->getActiveCamera())activeCameraChanged(State::getInstance()->getActiveCamera());
}

CameraViewWidget::~CameraViewWidget()
{
}

void CameraViewWidget::setImageName(QString name)
{
	widget->imageTitleLabel->setText(cameraName + " - " + name);
}

void CameraViewWidget::setBias(double value)
{
	if (widget != NULL && widget->glCameraView != NULL) 
		widget->glCameraView->setBias(value);
}

void CameraViewWidget::setScale(double value)
{
	if (widget != NULL && widget->glCameraView != NULL) 
		widget->glCameraView->setScale(value);
}

void CameraViewWidget::setTransparency(double value)
{
	if (widget != NULL && widget->glCameraView != NULL)
		widget->glCameraView->setTransparency(value);
}

void CameraViewWidget::setRenderTransparentModels(bool value)
{
	if (widget != NULL && widget->glCameraView != NULL)
		widget->glCameraView->setRenderTransparentModels(value);
}

void CameraViewWidget::centerViewToPoint()
{
	widget->glCameraView->centerViewToPoint(false);
	if (m_visible)widget->glCameraView->update();
}

const bool& CameraViewWidget::isVisible()
{
	return m_visible;
}

void CameraViewWidget::setIsVisible(bool value)
{
	m_visible = value;
}

void CameraViewWidget::setSharedGLContext(const QGLContext* sharedContext)
{
	QGLContext* context = new QGLContext(sharedContext->format(), widget->glCameraView);
	context->create(sharedContext);
	widget->glCameraView->setContext(context, sharedContext, true);
}

void CameraViewWidget::draw()
{
	if (m_visible)widget->glCameraView->update();
}

void CameraViewWidget::updateInfo()
{
	if (m_visible){
		calibrationFrame->update(camera);
		undistortionFrame->update(camera);
	}
}


void CameraViewWidget::setMinimumWidthGL(bool set)
{
	widget->glCameraView->setMinimumWidthGL(set);
}

void CameraViewWidget::on_toolButtonFitZoom_clicked(bool checked)
{
	widget->glCameraView->setAutoZoom(checked);
	if (m_visible) widget->glCameraView->update();
}

void CameraViewWidget::on_toolButtonInfo_clicked(bool checked)
{
	if (checked)
	{
		widget->frameInfo->show();
	}
	else
	{
		widget->frameInfo->hide();
	}
}


void CameraViewWidget::on_spinBoxZoom_valueChanged(int value)
{
	widget->glCameraView->setZoom(value);
	if (m_visible) widget->glCameraView->update();
}

void CameraViewWidget::autozoomChanged(bool on)
{
	widget->toolButtonFitZoom->setChecked(on);
}

void CameraViewWidget::zoomChanged(int zoom)
{
	if (zoom != widget->spinBoxZoom->value())
		widget->spinBoxZoom->setValue(zoom);
}

void CameraViewWidget::transparencyChanged(double value)
{
	if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		digitizationFrame->setTransparency(value);
	}
}

void CameraViewWidget::workspaceChanged(work_state workspace)
{
	if (workspace == UNDISTORTION)
	{
		undistortionFrame->show();
		calibrationFrame->hide();
		digitizationFrame->hide();
		digitizationFrame->reset();
	}
	else if (workspace == CALIBRATION)
	{
		undistortionFrame->hide();
		calibrationFrame->show();
		digitizationFrame->hide();
		digitizationFrame->reset();
	}
	else if (workspace == DIGITIZATION)
	{
		undistortionFrame->hide();
		calibrationFrame->hide();
		digitizationFrame->show();
		digitizationFrame->reset();
	}
}

void CameraViewWidget::activeFrameCalibrationChanged(int activeFrame)
{
	if (m_visible) calibrationFrame->updateFrame(camera);
}

bool CameraViewWidget::eventFilter(QObject* obj, QEvent* event)
{
	if (event->type() == QEvent::MouseButtonPress) State::getInstance()->changeActiveCamera(camera->getID());

	if (obj == this)
	{
		return QWidget::eventFilter(obj, event);
	}
	else
	{
		return false;
	}
}

void CameraViewWidget::activeCameraChanged(int activeCamera)
{
	if (activeCamera == camera->getID())
	{
		QPalette pal(palette());
		QBrush brush1(QColor(215, 215, 215, 255));
		brush1.setStyle(Qt::SolidPattern);
		pal.setBrush(QPalette::Active, QPalette::Window, brush1);
		pal.setBrush(QPalette::Inactive, QPalette::Window, brush1);
		setPalette(pal);
	}
	else
	{
		QPalette pal(palette());
		QBrush brush1(QColor(255, 255, 255, 0));
		brush1.setStyle(Qt::SolidPattern);
		pal.setBrush(QPalette::Active, QPalette::Window, brush1);
		pal.setBrush(QPalette::Inactive, QPalette::Window, brush1);
		setPalette(pal);
	}
}

