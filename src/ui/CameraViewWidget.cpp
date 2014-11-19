/*
 * Image_subwindow.cpp
 *
 *  Created on: Nov 19, 2013
 *      Author: ben
 */

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "core/Camera.h"
#include "ui/CameraViewWidget.h"
#include "ui_CameraViewWidget.h"

CameraViewWidget::CameraViewWidget(Camera * camera, QWidget *parent) :
											QWidget(parent),
												widget(new Ui::CameraViewWidget){
	widget->setupUi(this);
	setCameraName(camera->getName());
	widget->glCameraView->setCamera(camera);

	connect(widget->glCameraView, SIGNAL(autozoomChanged(bool)), this, SLOT(autozoomChanged(bool)));
	connect(widget->glCameraView, SIGNAL(zoomChanged(int)), this, SLOT(zoomChanged(int)));
}

CameraViewWidget::~CameraViewWidget(){

}

void CameraViewWidget::setCameraName(QString name){
	widget->cameraTitleLabel->setText(name);
}

void CameraViewWidget::setImageName(QString name){
	widget->imageTitleLabel->setText(name);
}

void CameraViewWidget::setSharedGLContext(const QGLContext * sharedContext){
	QGLContext* context = new QGLContext(sharedContext->format(), widget->glCameraView);
	context->create(sharedContext);
	widget->glCameraView->setContext(context,sharedContext,true);
}

void CameraViewWidget::draw(){
	widget->glCameraView->update();
}

void CameraViewWidget::setMinimumWidthGL(bool set){
	widget->glCameraView->setMinimumWidthGL(set);
}

void CameraViewWidget::on_toolButtonFitZoom_clicked(bool checked){
	widget->glCameraView->setAutoZoom(checked);
	widget->glCameraView->update();
}

void CameraViewWidget::on_spinBoxZoom_valueChanged(int value){
	widget->glCameraView->setZoom(value);
	widget->glCameraView->update();
}

void CameraViewWidget::autozoomChanged(bool on){
	widget->toolButtonFitZoom->setChecked(on);
}
void CameraViewWidget::zoomChanged(int zoom){
	if(zoom != widget->spinBoxZoom->value())
		widget->spinBoxZoom->setValue(zoom);
}