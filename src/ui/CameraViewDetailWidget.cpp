#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/CameraViewDetailWidget.h"
#include "ui_CameraViewDetailWidget.h"

#include "core/Camera.h"

using namespace xma;

CameraViewDetailWidget::CameraViewDetailWidget(Camera * _camera, QWidget *parent) :
											QWidget(parent),
											widget(new Ui::CameraViewDetailWidget){
	
	camera = _camera;
	widget->setupUi(this);
	widget->glCameraView->setCamera(camera);
	widget->glCameraView->setDetailedView();
}

CameraViewDetailWidget::~CameraViewDetailWidget(){

}

void CameraViewDetailWidget::setSharedGLContext(const QGLContext * sharedContext){
	QGLContext* context = new QGLContext(sharedContext->format(), widget->glCameraView);
	context->create(sharedContext);
	widget->glCameraView->setContext(context,sharedContext,true);
}

void CameraViewDetailWidget::draw(){
	widget->glCameraView->update();
}

void CameraViewDetailWidget::setMinimumWidthGL(bool set){
	widget->glCameraView->setMinimumWidthGL(set);
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