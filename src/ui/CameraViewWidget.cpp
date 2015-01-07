#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/CameraViewWidget.h"
#include "ui_CameraViewWidget.h"
#include "ui/UndistortionInfoFrame.h"
#include "ui/CalibrationInfoFrame.h"

#include "core/Camera.h"

using namespace xma;

CameraViewWidget::CameraViewWidget(Camera * _camera, QWidget *parent) :
											QWidget(parent),
												widget(new Ui::CameraViewWidget){
	
	camera = _camera;

	widget->setupUi(this);
	setCameraName(camera->getName());
	widget->glCameraView->setCamera(camera);
	widget->frameInfo->hide();

	undistortionFrame = new UndistortionInfoFrame(this);
	calibrationFrame = new CalibrationInfoFrame(this);
	widget->gridLayout_3->addWidget(undistortionFrame, 0, 0, 1, 1);
	widget->gridLayout_3->addWidget(calibrationFrame, 1, 0, 1, 1);


	connect(widget->glCameraView, SIGNAL(autozoomChanged(bool)), this, SLOT(autozoomChanged(bool)));
	connect(widget->glCameraView, SIGNAL(zoomChanged(int)), this, SLOT(zoomChanged(int)));

	connect(State::getInstance(), SIGNAL(workspaceChanged(work_state)), this, SLOT(workspaceChanged(work_state)));
	connect(State::getInstance(), SIGNAL(activeFrameCalibrationChanged(int)), this, SLOT(activeFrameCalibrationChanged(int)));
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

void CameraViewWidget::updateInfo(){
	calibrationFrame->update(camera);
	undistortionFrame->update(camera);
}


void CameraViewWidget::setMinimumWidthGL(bool set){
	widget->glCameraView->setMinimumWidthGL(set);
}

void CameraViewWidget::on_toolButtonFitZoom_clicked(bool checked){
	widget->glCameraView->setAutoZoom(checked);
	widget->glCameraView->update();
}

void CameraViewWidget::on_toolButtonInfo_clicked(bool checked){
	if(checked)
	{
		widget->frameInfo->show();
	}else{
		widget->frameInfo->hide();
	} 
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

void CameraViewWidget::workspaceChanged(work_state workspace){
	if(workspace == UNDISTORTION){
		undistortionFrame->show();
		calibrationFrame->hide();
	}else if(workspace == CALIBRATION){
		undistortionFrame->hide();
		calibrationFrame->show();
	}
}

void CameraViewWidget::activeFrameCalibrationChanged(int activeFrame){
	calibrationFrame->updateFrame(camera);
}