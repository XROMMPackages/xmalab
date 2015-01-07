/*
 * ProgressDialog.cpp
 *
 *  Created on: Nov 19, 2013
 *      Author: ben
 */

#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include <GL/glew.h>

#include "ui/GLCameraView.h"
#include "ui/State.h"
#include "ui/ErrorDialog.h"
#include "ui/WizardDockWidget.h"
#include "ui/ConsoleDockWidget.h"
#include "ui/GLSharedWidget.h"

#include "core/Camera.h"
#include "core/UndistortionObject.h"
#include "core/CalibrationImage.h"
#include "core/Image.h"
#include "core/Trial.h"
#include "core/Project.h"
#include "core/Marker.h"

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



using namespace xma;

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
	this->setCursor( QCursor( Qt::CrossCursor ));
	setZoomRatio(1.0,true);
	detailedView = false;
	bias = 0.0;
	scale = 1.0;
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
		if(y_offset < -0.5 * (zoomRatio * window_height)) y_offset = -0.5 * (zoomRatio * window_height);
		if(y_offset > -camera_height + 0.5 * (zoomRatio * window_height)) y_offset = -camera_height + 0.5 * (zoomRatio * window_height);
	}else{
		if(y_offset > -0.5 * (zoomRatio * window_height)) y_offset = -0.5 * (zoomRatio * window_height);
		if(y_offset < -camera_height + 0.5 * (zoomRatio * window_height)) y_offset = -camera_height + 0.5 * (zoomRatio * window_height);
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
		if(State::getInstance()->getWorkspace() == CALIBRATION
			&& camera->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->isCalibrated() > 0){
				double x =  zoomRatio * (e->posF().x()) - ((window_width)  * zoomRatio * 0.5 + x_offset);
				double y =  zoomRatio * (e->posF().y()) - ((window_height) * zoomRatio * 0.5 + y_offset);
				camera->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->toggleInlier(x, y , State::getInstance()->getCalibrationVisImage() == DISTORTEDCALIBIMAGE);
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

		 double x =  zoomRatio * (e->posF().x()) - ((window_width ) * zoomRatio * 0.5 + x_offset) - 0.5;
		 double y =  zoomRatio * (e->posF().y()) - ((window_height) * zoomRatio * 0.5 + y_offset) - 0.5;
		 if(State::getInstance()->getWorkspace() == UNDISTORTION){
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
		 }else if(State::getInstance()->getWorkspace() == CALIBRATION){
			 if(camera->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->isCalibrated() <= 0){
				WizardDockWidget::getInstance()->addCalibrationReference(x, y);
			 }else{
				 if(e->modifiers().testFlag(Qt::ControlModifier)){
					camera->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->setPointManual(x, y , State::getInstance()->getCalibrationVisImage() == DISTORTEDCALIBIMAGE);
					updateGL();
				 }
			 }
		 }else if (State::getInstance()->getWorkspace() == DIGITIZATION){
			 if (e->modifiers().testFlag(Qt::ControlModifier)){
				 WizardDockWidget::getInstance()->addDigitizationPoint(camera->getID(), x, y);
			 }
			 else
			 {
				 WizardDockWidget::getInstance()->moveDigitizationPoint(camera->getID(), x, y);
			 }

			 updateGL();
		 }
		 updateGL();
	 }
}

void GLCameraView::wheelEvent(QWheelEvent *e){
	if (!detailedView){
		State::getInstance()->changeActiveCamera(this->camera->getID());
		double zoom_prev = zoomRatio;

		setZoomRatio(zoomRatio * 1 + e->delta() / 1000.0, false);

		QPoint coordinatesGlobal = e->globalPos();
		QPoint coordinates = this->mapFromGlobal(coordinatesGlobal);

		y_offset += (zoom_prev - zoomRatio) * (0.5 * window_height - coordinates.y());
		x_offset += (zoom_prev - zoomRatio) * (0.5 * window_width - coordinates.x());

		updateGL();
	}
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

void GLCameraView::setDetailedView()
{
	detailedView = true;
	setZoomRatio(0.2, false);
}

void GLCameraView::setScale(double value)
{
	scale = value;
	update();
}

void GLCameraView::setBias(double value)
{
	bias = value;
	update();
}

void GLCameraView::setZoomToFit(){
	setZoomRatio((((double) camera_width) / window_width > ((double) camera_height) / window_height ) ? ((double) camera_width) / window_width : ((double) camera_height) / window_height,autozoom);

	x_offset =  -0.5 * (zoomRatio * window_width);
	y_offset =  -0.5 * (zoomRatio * window_height);

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
	renderText(-zoomRatio * fm.width(string) * 0.5 - x_offset, - zoomRatio * fm.height() * 0.5 - y_offset, 0.0, string);
}

void GLCameraView::renderPointText(){
	std::vector<double> x;
	std::vector<double> y;
	std::vector<QString> text;
	std::vector<bool> inlier;

	camera->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->getDrawTextData(State::getInstance()->getCalibrationVisText(), State::getInstance()->getCalibrationVisImage() == DISTORTEDCALIBIMAGE, x, y, text, inlier);
	setFont(QFont(this->font().family(), 15.0 ));
	QFontMetrics fm(this->font());
	if(State::getInstance()->getCalibrationVisText() < 3){
		for(int i = 0 ; i < x.size(); i++){
			if(inlier[i]) {
				qglColor(QColor(0, 255, 0));
			}else{
				qglColor(QColor(255, 0, 0));
			}
			renderText(x[i] + 3, y[i], 0.0, text[i]);
		}
	}else{
		double max = 0;
		for(int i = 0 ; i < text.size(); i++){
			if(text[i].toDouble() > max) max = text[i].toDouble();
		}
		for(int i = 0 ; i < x.size(); i++){
			double val = text[i].toDouble();
			qglColor(QColor(255.0 * val / max, 255.0 * (1.0 - val / max), 0));
			renderText(x[i] + 3, y[i], 0.0, text[i]);
		}
	}

	x.clear();
	y.clear();
	inlier.clear();
	text.clear();
}

void GLCameraView::drawTexture()
{
	bool drawImage = true;
	bool noDraw = false;
	glEnable(GL_TEXTURE_2D);
	if (State::getInstance()->getWorkspace() == UNDISTORTION){
		if (camera->hasUndistortion()){
			camera->getUndistortionObject()->bindTexture(State::getInstance()->getUndistortionVisImage());
		}
		else{
			drawImage = false;
			renderTextCentered("No undistortion grid loaded");
		}
	}
	else if (State::getInstance()->getWorkspace() == CALIBRATION){
		camera->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->bindTexture(State::getInstance()->getCalibrationVisImage());
	}
	else if (State::getInstance()->getWorkspace() == DIGITIZATION){
		if (Project::getInstance()->getTrials().size() > 0)
		{
			Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getImage(camera->getID())->bindTexture();
		}
	}
	if (drawImage){
		glDisable(GL_LIGHTING);
		drawQuad();
	}
	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void GLCameraView::drawQuad()
{
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex2f(-0.5, -0.5);
	glTexCoord2f(0, 1);
	glVertex2f(-0.5, camera_height - 0.5);
	glTexCoord2f(1, 1);
	glVertex2f(camera_width - 0.5, camera_height - 0.5);
	glTexCoord2f(1, 0);
	glVertex2f(camera_width - 0.5, -0.5);
	glEnd();
}



void GLCameraView::paintGL()
{	
	glDisable(GL_DEPTH_TEST);	
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity();

	if (detailedView && State::getInstance()->getWorkspace() == DIGITIZATION)
	{

		Marker * activeMarker = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarker();
		if (activeMarker != NULL){
			double x = activeMarker->getPoints2D()[camera->getID()][State::getInstance()->getActiveFrameTrial()].x;
			double y = activeMarker->getPoints2D()[camera->getID()][State::getInstance()->getActiveFrameTrial()].y;
			x_offset = - x;
			y_offset = - y;
		}
	}

	glOrtho(-0.5 * (zoomRatio * window_width) - x_offset - 0.5, 
			 0.5 * (zoomRatio * window_width) - x_offset - 0.5,
			 0.5 * (zoomRatio * window_height) - y_offset - 0.5,
			 -0.5 * (zoomRatio * window_height) - y_offset - 0.5,
			-1000,1000);

	gluLookAt (0, 0,1, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity();

	glColor3f(1.0, 1.0, 1.0);
	drawTexture();

	if ((bias != 0.0 || scale != 1.0) && GLSharedWidget::getInstance()->getHasBlendSubtract()){
			/* NOTE: The blending approach does not allow negative
			scales.  The blending approach also fails if the
			partial scaling or biasing results leave the 0.0 to
			1.0 range (example, scale=5.47, bias=-1.2). */

			glEnable(GL_BLEND);
			if (scale > 1.0) {
				float remainingScale;

				remainingScale = scale;
				if (GLSharedWidget::getInstance()->getHasBlendExt()){
					glBlendEquationEXT(GL_FUNC_ADD_EXT);
				}
				glBlendFunc(GL_DST_COLOR, GL_ONE);
				if (remainingScale > 2.0) {
					/* Clever cascading approach.  Example: if the
					scaling factor was 9.5, do 3 "doubling" blends
					(8x), then scale by the remaining 1.1875. */
					glColor4f(1, 1, 1, 1);
					while (remainingScale > 2.0) {
						drawQuad();
						remainingScale /= 2.0;
					}
				}
				glColor4f(remainingScale - 1,remainingScale - 1, remainingScale - 1, 1);
				drawQuad();
				glBlendFunc(GL_ONE, GL_ONE);
				if (bias != 0) {
					if (bias > 0) {
						glColor4f(bias, bias, bias, 0.0);
					}
					else {
						if (GLSharedWidget::getInstance()->getHasBlendSubtract()){
							glBlendEquationEXT(GL_FUNC_REVERSE_SUBTRACT_EXT);
						}
						glColor4f(-bias, -bias, -bias, 0.0);
					}
					drawQuad();
				}
			}
			else {
				if (bias > 0) {
					if (GLSharedWidget::getInstance()->getHasBlendExt()){
						glBlendEquationEXT(GL_FUNC_ADD_EXT);
					}
					glColor4f(bias, bias, bias, scale);
				}
				else {
					if (GLSharedWidget::getInstance()->getHasBlendSubtract()){
						glBlendEquationEXT(GL_FUNC_REVERSE_SUBTRACT_EXT);
					}
					glColor4f(-bias, -bias, -bias, scale);
				}
				glBlendFunc(GL_ONE, GL_SRC_ALPHA);
				drawQuad();
			}
			glDisable(GL_BLEND);
	}
	if(State::getInstance()->getWorkspace() == UNDISTORTION){
		if(camera->hasUndistortion()){
			camera->getUndistortionObject()->drawData(State::getInstance()->getUndistortionVisPoints());
		}
	}
	else if (State::getInstance()->getWorkspace() == CALIBRATION)
	{
		camera->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->draw(State::getInstance()->getCalibrationVisPoints());
		if(State::getInstance()->getCalibrationVisText() > 0){
			renderPointText();
		}
	}
	else if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->drawPoints(this->camera->getID(), detailedView);
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
