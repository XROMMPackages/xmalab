/*
 * ProgressDialog.cpp
 *
 *  Created on: Nov 19, 2013
 *      Author: ben
 */

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/GLCameraView.h"
#include "ui/State.h"
#include "ui/ErrorDialog.h"
#include "ui/WizardDockWidget.h"
#include "core/Camera.h"
#include "core/UndistortionObject.h"
#include "core/CalibrationImage.h"
#include "core/Image.h"

#include <QMouseEvent>

#include <iostream> 
#include <math.h>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#ifdef _WIN32
  #include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#endif

GLCameraView::GLCameraView(QWidget *parent)
    : QGLWidget(QGLFormat(QGL::SampleBuffers), parent)
{
	camera = NULL;
	window_width=50;
	window_height=50;
    setMinimumSize(50, 50);
    setAutoFillBackground(false);
	x_offset = 0;
	y_offset = 0;

	setZoomRatio(1.0,true);
}

void GLCameraView::setCamera(Camera * _camera){
	camera = _camera;

	camera_width = camera->getWidth();
	camera_height = camera->getHeight();
}


void GLCameraView::clampXY(){
	if(camera_width < window_width * zoomRatio){
		if(x_offset < -0.5 * (zoomRatio * window_width)) x_offset = -0.5 * (zoomRatio * window_width);
		if(x_offset > -camera_width + 0.5 * (zoomRatio * window_width)) x_offset = -camera_width +0.5 * (zoomRatio * window_width);
	}else{
		if(x_offset > -0.5 * (zoomRatio * window_width)) x_offset = -0.5 * (zoomRatio * window_width);
		if(x_offset < -camera_width + 0.5 * (zoomRatio * window_width)) x_offset = -camera_width +0.5 * (zoomRatio * window_width);
	}

	if(camera_height < window_height * zoomRatio){
		if(y_offset > 0.5 * (zoomRatio * window_height)) y_offset = 0.5 * (zoomRatio * window_height);
		if(y_offset < camera_height - 0.5 * (zoomRatio * window_height)) y_offset = camera_height - 0.5 * (zoomRatio * window_height);
	}else{
		if(y_offset < 0.5 * (zoomRatio * window_height)) y_offset = 0.5 * (zoomRatio * window_height);
		if(y_offset > camera_height - 0.5 * (zoomRatio * window_height)) y_offset = camera_height - 0.5 * (zoomRatio * window_height);
	}
}

void GLCameraView::mouseMoveEvent(QMouseEvent *e)
{ 
     if(e->buttons() & Qt::RightButton)
	 {
		 y_offset -= (prev_y - zoomRatio * e->posF().y());
		 x_offset -= (prev_x - zoomRatio * e->posF().x());

		 prev_y = zoomRatio * e->posF().y();
		 prev_x = zoomRatio * e->posF().x();

		 clampXY();
         updateGL();
     }
}

void GLCameraView::mouseDoubleClickEvent ( QMouseEvent * e ){
	State::getInstance()->changeActiveCamera(this->camera->getID());
	if(e->buttons() & Qt::LeftButton){
		if(State::getInstance()->getWorkspace() == work_state::CALIBRATION
			&& camera->getCalibrationImages()[State::getInstance()->getActiveFrame()]->isCalibrated() > 0){
				double x =  zoomRatio * (e->posF().x()) - ((window_width ) * zoomRatio * 0.5 + x_offset);
				double y =  zoomRatio * ((window_height) - e->posF().y()) - ((window_height) * zoomRatio * 0.5 - y_offset) - 0.5;
				camera->getCalibrationImages()[State::getInstance()->getActiveFrame()]->toggleInlier(x, y , State::getInstance()->getCalibrationVisImage() == calibrationVisImage_state::DISTORTEDCALIBIMAGE);
			updateGL();
		}
	}
}

void GLCameraView::mousePressEvent(QMouseEvent *e)
{
	 State::getInstance()->changeActiveCamera(this->camera->getID());
     if(e->buttons() & Qt::RightButton)
     {
		 prev_y = zoomRatio * e->posF().y();
		 prev_x = zoomRatio * e->posF().x();
	 }else if(e->buttons() & Qt::LeftButton){

		 double x =  zoomRatio * (e->posF().x()) - ((window_width ) * zoomRatio * 0.5 + x_offset);
		 double y =  zoomRatio * ((window_height) - e->posF().y()) - ((window_height) * zoomRatio * 0.5 - y_offset) - 0.5;
		 if(State::getInstance()->getWorkspace() == work_state::UNDISTORTION){
			if(camera->hasUndistortion()){
				int clickmode = State::getInstance()->State::getInstance()->getUndistortionMouseMode();
				if(clickmode == 1)
				{
					int vismode = State::getInstance()->getUndistortionVisPoints();
					if(vismode == 2 || vismode == 3 || vismode == 4 ){
						camera->getUndistortionObject()->toggleOutlier(vismode, x, y);
					}else{
						ErrorDialog::getInstance()->showErrorDialog("You either have to display the grid points (distorted or undistorted) or the reference points to toggle outlier");
					}
				}else if (clickmode == 2){
					int vismode = State::getInstance()->getUndistortionVisImage();
					if(vismode == 0){
						camera->getUndistortionObject()->setCenter(x, y);
					}else{
						//Fix also allow undistorted selection
						ErrorDialog::getInstance()->showErrorDialog("You have to display the distorted image to set the center");
					}
				
				}
			}
		 }else if(State::getInstance()->getWorkspace() == work_state::CALIBRATION){
			 if(camera->getCalibrationImages()[State::getInstance()->getActiveFrame()]->isCalibrated() <= 0){
				WizardDockWidget::getInstance()->addCalibrationReference(x, y);
			 }else{
				 if(e->modifiers().testFlag(Qt::ControlModifier)){
					camera->getCalibrationImages()[State::getInstance()->getActiveFrame()]->setPointManual(x, y , State::getInstance()->getCalibrationVisImage() == calibrationVisImage_state::DISTORTEDCALIBIMAGE);
					updateGL();
				 }
			 }
		 }
		 updateGL();
	 }
}

void GLCameraView::wheelEvent(QWheelEvent *e){
	State::getInstance()->changeActiveCamera(this->camera->getID());
	double zoom_prev = zoomRatio;
	
	setZoomRatio(zoomRatio * 1 + e->delta()/1000.0,false);

	QPoint coordinatesGlobal = e->globalPos();
	QPoint coordinates =  this->mapFromGlobal(coordinatesGlobal);
			
	y_offset += (zoom_prev - zoomRatio) * (0.5 * window_height - coordinates.y());
	x_offset += (zoom_prev - zoomRatio) * (0.5 * window_width - coordinates.x());

	updateGL();
}

void GLCameraView::initializeGL(){
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);				// Black Background
	glClearDepth(1.0f);									// Depth Buffer Setup
	glDisable(GL_DEPTH_TEST);	
}

void GLCameraView::setAutoZoom(bool on){ 
	if(on){
		setZoomRatio(zoomRatio,true);
		setZoomToFit();
	}else{
		setZoomRatio(zoomRatio,false);
	}

}
void GLCameraView::setZoom(int value){
	setZoomRatio(100.0/value,autozoom);
	update();
}


void GLCameraView::setZoomToFit(){
	setZoomRatio((((double) camera_width) / window_width > ((double) camera_height) / window_height ) ? ((double) camera_width) / window_width : ((double) camera_height) / window_height,autozoom);

	x_offset = -0.5 * (zoomRatio * window_width);
	y_offset =  0.5 * (zoomRatio * window_height);

	update();
}

void GLCameraView::setZoomTo100(){
	setZoomRatio(1.0,autozoom);
	update();
}

void GLCameraView::setMinimumWidthGL(bool set){
	if(set){
		double ratio = camera->getWidth() /  camera->getHeight();
		this->setMinimumWidth( ratio * this->size().height() );
	}else{
		this->setMinimumWidth(0);
	}
}

void GLCameraView::resizeGL(int _w, int _h){
	window_width = _w;
	window_height = _h;
	
	glViewport(0,0,window_width,window_height);	

	if(autozoom)setZoomToFit();
}

void GLCameraView::renderTextCentered(QString string){
	setFont(QFont(this->font().family(), 24));
	qglColor(QColor(255, 0, 0));
	QFontMetrics fm(this->font());
	renderText(-zoomRatio * fm.width(string) * 0.5 - x_offset, y_offset - zoomRatio * fm.height() * 0.5 , 0.0, string);
}

void GLCameraView::paintGL()
{
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity();

	glOrtho(-0.5 * (zoomRatio * window_width) - x_offset - 0.5, 
			 0.5 * (zoomRatio * window_width) - x_offset - 0.5,
			-0.5 * (zoomRatio * window_height) + y_offset - 0.5,
			 0.5 * (zoomRatio * window_height) + y_offset - 0.5,
			-1000,1000);

	gluLookAt (0, 0,1, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity();

	bool drawImage = true;
	bool noDraw = false;
	glEnable(GL_TEXTURE_2D);
	if(State::getInstance()->getWorkspace() == work_state::UNDISTORTION){
		if(camera->hasUndistortion()){
			camera->getUndistortionObject()->bindTexture(State::getInstance()->getUndistortionVisImage());
		}else{
			drawImage = false;
			renderTextCentered("No undistortion grid loaded");
		}
	}else if(State::getInstance()->getWorkspace() == work_state::CALIBRATION){
		camera->getCalibrationImages()[State::getInstance()->getActiveFrame()]->bindTexture(State::getInstance()->getCalibrationVisImage());
	}

	if(drawImage){
		glDisable(GL_LIGHTING);

		glColor3f(1.0,1.0,1.0);
		glBegin(GL_QUADS);
		glTexCoord2f(0,0);
		glVertex2f(-0.5,-0.5);
		glTexCoord2f(0,1);
		glVertex2f(-0.5, camera_height - 0.5);
		glTexCoord2f(1,1);
		glVertex2f(camera_width - 0.5, camera_height - 0.5);
		glTexCoord2f(1,0);
		glVertex2f(camera_width - 0.5, -0.5);
		glEnd();
	}
	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	if(State::getInstance()->getWorkspace() == work_state::UNDISTORTION){
		if(camera->hasUndistortion()){
			camera->getUndistortionObject()->drawData(State::getInstance()->getUndistortionVisPoints());
		}
	}else{
		camera->getCalibrationImages()[State::getInstance()->getActiveFrame()]->draw(State::getInstance()->getCalibrationVisPoints());
	}

	if(State::getInstance()->getActiveCamera() == this->camera->getID()){
		WizardDockWidget::getInstance()->draw();
	}

	glFlush ();
}

void GLCameraView::setZoomRatio(double newZoomRation, bool newAutozoom){
	newZoomRation = (newZoomRation > 100.0/999.0) ? newZoomRation : 100.0/999.0;
	if(zoomRatio != newZoomRation){
		zoomRatio = newZoomRation;
		int newZoom = floor(100.0/zoomRatio + 0.5);
		emit zoomChanged(newZoom);
	}
	if(autozoom != newAutozoom){
		autozoom = newAutozoom;
		emit autozoomChanged(autozoom);
	}
}
