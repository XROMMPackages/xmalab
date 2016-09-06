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
///\file GLCameraView.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <GL/glew.h>
#include "gl/MultisampleFrameBuffer.h"
#include "gl/DistortionShader.h"
#include "gl/BlendShader.h"
#include "ui/GLCameraView.h"
#include "ui/State.h"
#include "ui/ErrorDialog.h"
#include "ui/WizardDockWidget.h"
#include "ui/ConsoleDockWidget.h"
#include "ui/GLSharedWidget.h"
#include "ui/DetailViewDockWidget.h"

#include "core/Camera.h"
#include "core/UndistortionObject.h"
#include "core/CalibrationImage.h"
#include "core/Image.h"
#include "core/Trial.h"
#include "core/Project.h"
#include "core/Marker.h"
#include "core/RigidBody.h"
#include "core/Settings.h"
#include "processing/MarkerDetection.h"


#include <QMouseEvent>

#include <iostream> 
#include <math.h>
#include "MainWindow.h"

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

GLCameraView::GLCameraView(QWidget* parent)
	: QGLWidget(GLSharedWidget::getInstance()->format(), parent)
{
	camera = NULL;
	window_width = 50;
	window_height = 50;
	setMinimumSize(50, 50);
	setAutoFillBackground(false);
	x_offset = 0;
	y_offset = 0;
	this->setCursor(QCursor(Qt::CrossCursor));
	setZoomRatio(1.0, true);
	detailedView = false;
	bias = 0.0;
	scale = 1.0;
	transparency = 0.5;
	renderTransparentModels = true;
	showStatusColors = false;
	distortionShader = 0;
	blendShader = NULL; 
	rigidbodyBufferUndistorted = NULL;

	LightAmbient[0] = LightAmbient[1] = LightAmbient[2] = 0.1f;
	LightAmbient[3] = 1.0f;

	LightDiffuse[0] = LightDiffuse[1] = LightDiffuse[2] = 0.7f;
	LightDiffuse[3] = 1.0f;

	LightPosition_front[0] = LightPosition_front[1] = LightPosition_front[3] = 0.0f;
	LightPosition_front[2] = 1.0f;

	LightPosition_back[0] = LightPosition_back[1] = LightPosition_back[3] = 0.0f;
	LightPosition_back[2] = -1.0f;
}

GLCameraView::~GLCameraView()
{
	if (blendShader)
		delete blendShader;

	if (rigidbodyBufferUndistorted)
		delete rigidbodyBufferUndistorted;

	if (distortionShader)
		delete distortionShader;
}

void GLCameraView::setCamera(Camera* _camera)
{
	camera = _camera;

	camera_width = camera->getWidth();
	camera_height = camera->getHeight();

	if (!detailedView){
		if (rigidbodyBufferUndistorted)
			delete rigidbodyBufferUndistorted;
		if (GLSharedWidget::getInstance()->getVersion() >= 3.2){
			rigidbodyBufferUndistorted = new MultisampleFrameBuffer(camera_width, camera_height, 4);
		}
		else
		{
			rigidbodyBufferUndistorted = new FrameBuffer(camera_width, camera_height);
		}
		if (distortionShader)
			delete distortionShader;
		distortionShader = new DistortionShader(camera);

		if (blendShader)
			delete blendShader;
		blendShader = new BlendShader(); 
	}
}


void GLCameraView::clampXY()
{
	if (camera_width < window_width * zoomRatio)
	{
		if (x_offset < -0.5 * (zoomRatio * window_width)) x_offset = -0.5 * (zoomRatio * window_width);
		if (x_offset > -camera_width + 0.5 * (zoomRatio * window_width)) x_offset = -camera_width + 0.5 * (zoomRatio * window_width);
	}
	else
	{
		if (x_offset > -0.5 * (zoomRatio * window_width)) x_offset = -0.5 * (zoomRatio * window_width);
		if (x_offset < -camera_width + 0.5 * (zoomRatio * window_width)) x_offset = -camera_width + 0.5 * (zoomRatio * window_width);
	}

	if (camera_height < window_height * zoomRatio)
	{
		if (y_offset < -0.5 * (zoomRatio * window_height)) y_offset = -0.5 * (zoomRatio * window_height);
		if (y_offset > -camera_height + 0.5 * (zoomRatio * window_height)) y_offset = -camera_height + 0.5 * (zoomRatio * window_height);
	}
	else
	{
		if (y_offset > -0.5 * (zoomRatio * window_height)) y_offset = -0.5 * (zoomRatio * window_height);
		if (y_offset < -camera_height + 0.5 * (zoomRatio * window_height)) y_offset = -camera_height + 0.5 * (zoomRatio * window_height);
	}
}

void GLCameraView::mouseMoveEvent(QMouseEvent* e)
{
	if (e->buttons() & Qt::RightButton)
	{
		y_offset -= (prev_y - zoomRatio * e->posF().y());
		x_offset -= (prev_x - zoomRatio * e->posF().x());

		prev_y = zoomRatio * e->posF().y();
		prev_x = zoomRatio * e->posF().x();

		clampXY();
		updateGL();
	}
}

void GLCameraView::mouseDoubleClickEvent(QMouseEvent* e)
{
	State::getInstance()->changeActiveCamera(this->camera->getID());
	if (e->buttons() & Qt::LeftButton)
	{
		if (State::getInstance()->getWorkspace() == CALIBRATION
			&& camera->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->isCalibrated() > 0)
		{
			double x = zoomRatio * (e->posF().x()) - ((window_width) * zoomRatio * 0.5 + x_offset);
			double y = zoomRatio * (e->posF().y()) - ((window_height) * zoomRatio * 0.5 + y_offset);
			camera->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->toggleInlier(x, y, State::getInstance()->getCalibrationVisImage() == DISTORTEDCALIBIMAGE);
			updateGL();
		}
	}
}

void GLCameraView::mousePressEvent(QMouseEvent* e)
{
	State::getInstance()->changeActiveCamera(this->camera->getID());
	if (e->buttons() & Qt::RightButton)
	{
		prev_y = zoomRatio * e->posF().y();
		prev_x = zoomRatio * e->posF().x();
	}
	else if (e->buttons() & Qt::LeftButton)
	{
		double x = zoomRatio * (e->posF().x()) - ((window_width) * zoomRatio * 0.5 + x_offset) - 0.5;
		double y = zoomRatio * (e->posF().y()) - ((window_height) * zoomRatio * 0.5 + y_offset) - 0.5;
		if (State::getInstance()->getWorkspace() == UNDISTORTION)
		{
			if (camera->hasUndistortion())
			{
				int clickmode = State::getInstance()->State::getInstance()->getUndistortionMouseMode();
				if (clickmode == 1)
				{
					int vismode = State::getInstance()->getUndistortionVisPoints();
					if (vismode == 2 || vismode == 3 || vismode == 4)
					{
						camera->getUndistortionObject()->toggleOutlier(vismode, x, y);
					}
					else
					{
						ErrorDialog::getInstance()->showErrorDialog("You either have to display the grid points (distorted or undistorted) or the reference points to toggle outlier");
					}
				}
				else if (clickmode == 2)
				{
					int vismode = State::getInstance()->getUndistortionVisImage();
					if (vismode == 0)
					{
						camera->getUndistortionObject()->setCenter(x, y);
					}
					else
					{
						//Fix also allow undistorted selection
						ErrorDialog::getInstance()->showErrorDialog("You have to display the distorted image to set the center");
					}
				}
			}
		}
		else if (State::getInstance()->getWorkspace() == CALIBRATION)
		{
			if (e->modifiers().testFlag(Qt::AltModifier))
			{
				int method = Settings::getInstance()->getIntSetting("DetectionMethodForCalibration");
				if (method > 0)
				{
					if (method == 5)
					{
						method = 6;// white blobs
					}
					else
					{
						method = method - 1; // different ordering than normal detection
					}
					cv::Point out = MarkerDetection::detectionPoint(camera->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->getImage(), method, cv::Point2d(x, y), 40, 5);
					x = out.x;
					y = out.y;
				}
			}

			if (WizardDockWidget::getInstance()->manualCalibrationRunning())
			{
				WizardDockWidget::getInstance()->addCalibrationReference(x, y);
				//updateGL();
			}
			else
			{
				if (camera->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->isCalibrated() <= 0)
				{
					WizardDockWidget::getInstance()->addCalibrationReference(x, y);
				}
				else
				{
					if (e->modifiers().testFlag(Qt::ControlModifier))
					{
						camera->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->setPointManual(x, y, State::getInstance()->getCalibrationVisImage() == DISTORTEDCALIBIMAGE);
						//updateGL();
					}
				}
			}
		}
		else if (State::getInstance()->getWorkspace() == DIGITIZATION)
		{
			if (e->modifiers().testFlag(Qt::ControlModifier) && !detailedView)
			{
				WizardDockWidget::getInstance()->addDigitizationPoint(camera->getID(), x, y);
			}
			else if (e->modifiers().testFlag(Qt::ShiftModifier))
			{
				WizardDockWidget::getInstance()->selectDigitizationPoint(camera->getID(), x, y);
			}
			else
			{
				WizardDockWidget::getInstance()->moveDigitizationPoint(camera->getID(), x, y, detailedView);
			}

			if (!detailedView) DetailViewDockWidget::getInstance()->centerViews();

			WizardDockWidget::getInstance()->updateDialog();
		}
		MainWindow::getInstance()->redrawGL();
	}
}

void GLCameraView::wheelEvent(QWheelEvent* e)
{
	if (!detailedView || !Settings::getInstance()->getBoolSetting("CenterDetailView"))
	{
		if (e->modifiers().testFlag(Qt::ControlModifier))
		{
			if (State::getInstance()->getWorkspace() == DIGITIZATION){
				setTransparency(transparency + 1.0 / 2400.0 * e->delta());
				emit transparencyChanged(transparency);
			}
		}
		else{
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
}

void GLCameraView::initializeGL()
{
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Black Background
	glClearDepth(1.0f); // Depth Buffer Setup
	glDisable(GL_DEPTH_TEST);
}

void GLCameraView::setAutoZoom(bool on)
{
	if (on)
	{
		setZoomRatio(zoomRatio, true);
		setZoomToFit();
	}
	else
	{
		setZoomRatio(zoomRatio, false);
	}
}

void GLCameraView::setZoom(int value)
{
	setZoomRatio(100.0 / value, autozoom);
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

void GLCameraView::setTransparency(double value)
{
	
	transparency = value;

	if (transparency < 0.0)  transparency = 0.0;

	if (transparency > 1.0)  transparency = 1.0;

	update();
}

void GLCameraView::setRenderTransparentModels(bool value)
{
	renderTransparentModels = value;
	update();
}

void GLCameraView::setZoomToFit()
{
	setZoomRatio((((double) camera_width) / window_width > ((double) camera_height) / window_height) ? ((double) camera_width) / window_width : ((double) camera_height) / window_height, autozoom);

	x_offset = -0.5 * (zoomRatio * window_width);
	y_offset = -0.5 * (zoomRatio * window_height);

	update();
}

void GLCameraView::setZoomTo100()
{
	setZoomRatio(1.0, autozoom);
	update();
}

void GLCameraView::setMinimumWidthGL(bool set)
{
	if (set)
	{
		double ratio = camera->getWidth() / camera->getHeight();
		this->setMinimumWidth(ratio * this->size().height());
	}
	else
	{
		this->setMinimumWidth(0);
	}
}

void GLCameraView::resizeGL(int _w, int _h)
{
	window_width = _w;
	window_height = _h;

	glViewport(0, 0, window_width, window_height);
	if (autozoom)setZoomToFit();
}

void GLCameraView::renderTextCentered(QString string)
{
	setFont(QFont(this->font().family(), 24));
	qglColor(QColor(255, 0, 0));
	QFontMetrics fm(this->font());
	renderText(-zoomRatio * fm.width(string) * 0.5 - x_offset, - zoomRatio * fm.height() * 0.5 - y_offset, 0.0, string);
}

void GLCameraView::renderPointText(bool calibration)
{
	std::vector<double> x;
	std::vector<double> y;
	std::vector<QString> text;
	if (calibration)
	{
		std::vector<bool> inlier;
		camera->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->getDrawTextData(
			(!camera->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->isCalibrated() && WizardDockWidget::getInstance()->manualCalibrationRunning()) ? IDCALIBTEXT : State::getInstance()->getCalibrationVisText(), State::getInstance()->getCalibrationVisImage() == DISTORTEDCALIBIMAGE, x, y, text, inlier);
		setFont(QFont(this->font().family(), 15.0));
		QFontMetrics fm(this->font());
		if (State::getInstance()->getCalibrationVisText() < 3)
		{
			for (unsigned int i = 0; i < x.size(); i++)
			{
				if (inlier[i])
				{
					qglColor(QColor(0, 255, 0));
				}
				else
				{
					qglColor(QColor(255, 0, 0));
				}
				renderText(x[i] + 3, y[i], 0.0, text[i]);
			}
		}
		else
		{
			double max = 0;
			for (unsigned int i = 0; i < text.size(); i++)
			{
				if (text[i].toDouble() > max) max = text[i].toDouble();
			}
			for (unsigned int i = 0; i < x.size(); i++)
			{
				double val = text[i].toDouble();
				qglColor(QColor(255.0 * val / max, 255.0 * (1.0 - val / max), 0));
				renderText(x[i] + 3, y[i], 0.0, text[i]);
			}
		}
		inlier.clear();
	}
	else
	{
		Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getDrawTextData(this->camera->getID(), State::getInstance()->getActiveFrameTrial(), x, y, text);
		setFont(QFont(this->font().family(), 15.0));
		QFontMetrics fm(this->font());
		int active = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarkerIdx();
		for (unsigned int i = 0; i < x.size(); i++)
		{
			if (Settings::getInstance()->getBoolSetting("ShowColoredMarkerIDs"))
			{
				qglColor(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[i]->getStatusColor(this->camera->getID(), State::getInstance()->getActiveFrameTrial()));
			}
			else{
				if (active == i)
				{
					qglColor(QColor(255, 0, 0));
				}
				else
				{
					qglColor(QColor(0, 255, 0));
				}
			}
			renderText(x[i] + 3, y[i], 0.0, text[i]);
		}
	}

	x.clear();
	y.clear();
	text.clear();
}

void GLCameraView::drawTexture()
{
	bool drawImage = true;
	glEnable(GL_TEXTURE_2D);
	if (State::getInstance()->getWorkspace() == UNDISTORTION)
	{
		if (camera->hasUndistortion())
		{
			camera->getUndistortionObject()->bindTexture(State::getInstance()->getUndistortionVisImage());
		}
		else
		{
			drawImage = false;
			renderTextCentered("No undistortion grid loaded");
		}
	}
	else if (State::getInstance()->getWorkspace() == CALIBRATION)
	{
		camera->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->bindTexture(State::getInstance()->getCalibrationVisImage());
	}
	else if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		if ((int)Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0)
		{
			Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getVideoStreams()[camera->getID()]->bindTexture();
		}
	}
	if (drawImage)
	{
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

void GLCameraView::centerViewToPoint(bool resetZoom)
{
	if (detailedView && State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		if ((int)Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0)
		{
			Marker* activeMarker = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarker();
			if (activeMarker != NULL)
			{
				double x, y;
				if (activeMarker->getStatus2D()[camera->getID()][State::getInstance()->getActiveFrameTrial()] > 0)
				{
					x = activeMarker->getPoints2D()[camera->getID()][State::getInstance()->getActiveFrameTrial()].x;
					y = activeMarker->getPoints2D()[camera->getID()][State::getInstance()->getActiveFrameTrial()].y;
					x_offset = -x;
					y_offset = -y;
				}
				else if (activeMarker->getMarkerPrediction(camera->getID(), State::getInstance()->getActiveFrameTrial(), x, y, true))
				{
					x_offset = -x;
					y_offset = -y;
				}
				else if (activeMarker->getMarkerPrediction(camera->getID(), State::getInstance()->getActiveFrameTrial(), x, y, false))
				{
					x_offset = -x;
					y_offset = -y;
				}
			}
		}
		if (resetZoom) setZoomRatio(0.2, false);
	}
}

void GLCameraView::UseStatusColors(bool value)
{
	showStatusColors = value;
}


void GLCameraView::paintGL()
{
	doDistortion = false;
	renderMeshes = false;

	if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		if ((int)Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0)
		{
			if (!detailedView && Settings::getInstance()->getBoolSetting("TrialDrawRigidBodyMeshmodels"))
			{
				renderMeshes = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->renderMeshes();
			}
		}
		if (renderMeshes){
			if ((int)Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0)
			{
				if (!detailedView)
				{
					if (rigidbodyBufferUndistorted)
					{
						rigidbodyBufferUndistorted->bindFrameBuffer();

						glClearColor(1.0, 1.0, 1.0, 0.0);
						glClearDepth(1.0f);
						glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
						glEnable(GL_DEPTH_TEST); // Enables Depth Testing
						glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST); // Really Nice Perspective Calculations
						if (renderTransparentModels){
							glDepthFunc(GL_ALWAYS); // The Type Of Depth Testing To Do
							glEnable(GL_BLEND);
							glBlendFunc(GL_ZERO, GL_SRC_COLOR);
						}
						else
						{
							glDepthFunc(GL_LEQUAL);
						}
						glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmbient); // Setup The Ambient Light
						glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuse); // Setup The Diffuse Light
						glLightfv(GL_LIGHT1, GL_POSITION, LightPosition_front); // Position The Light
						glEnable(GL_LIGHT1);
						glLightfv(GL_LIGHT2, GL_AMBIENT, LightAmbient); // Setup The Ambient Light
						glLightfv(GL_LIGHT2, GL_DIFFUSE, LightDiffuse); // Setup The Diffuse Light
						glLightfv(GL_LIGHT2, GL_POSITION, LightPosition_back); // Position The Light
						glEnable(GL_LIGHT2);

						glEnable(GL_LIGHTING);

						glEnable(GL_COLOR_MATERIAL);
						glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

						glViewport(0, 0, rigidbodyBufferUndistorted->getWidth(), rigidbodyBufferUndistorted->getHeight());
						double proj[16];
						double model[16];

						camera->getGLTransformations(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getReferenceCalibrationImage(), &proj[0], &model[0]);

						glMatrixMode(GL_PROJECTION);
						glLoadMatrixd(&proj[0]);

						glMatrixMode(GL_MODELVIEW);
						glLoadMatrixd(&model[0]);
						if (GLSharedWidget::getInstance()->getVersion() > 3.2){
							glEnable(GL_MULTISAMPLE);
						}

						Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->drawRigidBodiesMesh();

						if (GLSharedWidget::getInstance()->getVersion() > 3.2){
							glDisable(GL_MULTISAMPLE);
						}

						glDisable(GL_BLEND);
						glDisable(GL_LIGHTING);
						glDisable(GL_DEPTH_TEST);
						rigidbodyBufferUndistorted->unbindFrameBuffer();
					}

					rigidbodyBufferUndistorted->blitFramebuffer();

					distortionShader->draw(rigidbodyBufferUndistorted->getTextureID(), rigidbodyBufferUndistorted->getDepthTextureID(), transparency);
					doDistortion = distortionShader->canRender();
				}
			}
		}
	}

	glViewport(0, 0, window_width, window_height);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClearDepth(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	if (detailedView && Settings::getInstance()->getBoolSetting("CenterDetailView")) centerViewToPoint();

	glOrtho(-0.5 * (zoomRatio * window_width) - x_offset - 0.5,
	        0.5 * (zoomRatio * window_width) - x_offset - 0.5,
	        0.5 * (zoomRatio * window_height) - y_offset - 0.5,
	        -0.5 * (zoomRatio * window_height) - y_offset - 0.5,
	        -1000, 1000);

	gluLookAt(0, 0, 1, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glColor3f(1.0, 1.0, 1.0);
	drawTexture();

	if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		if (renderMeshes){
			if ((int)Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0)
			{
				if (!detailedView)
				{
					if (!doDistortion){
						setFont(QFont(this->font().family(), 12));
						qglColor(QColor(255, 0, 0));
						QFontMetrics fm(this->font());
						QString string("Rigid body models are not distorted! XMALab is currently computing the distortion!");
						renderText(-zoomRatio * fm.width(string) * 0.5 - x_offset, -zoomRatio * (fm.height() - window_height)* 0.5 - y_offset, 0.0, string);
						blendShader->draw(camera_width, camera_height, transparency, rigidbodyBufferUndistorted->getTextureID(), rigidbodyBufferUndistorted->getDepthTextureID(), true);
					}
					else
					{
						blendShader->draw(camera_width, camera_height, transparency, distortionShader->getTextureID(), distortionShader->getDepthTextureID(), false);
					}
				}
			}
		}
	}

	if ((bias != 0.0 || scale != 1.0) && GLSharedWidget::getInstance()->getHasBlendSubtract())
	{
		/* NOTE: The blending approach does not allow negative
		scales.  The blending approach also fails if the
		partial scaling or biasing results leave the 0.0 to
		1.0 range (example, scale=5.47, bias=-1.2). */

		glEnable(GL_BLEND);
		if (scale > 1.0)
		{
			float remainingScale;

			remainingScale = scale;
			if (GLSharedWidget::getInstance()->getHasBlendExt())
			{
				glBlendEquationEXT(GL_FUNC_ADD_EXT);
			}
			glBlendFunc(GL_DST_COLOR, GL_ONE);
			if (remainingScale > 2.0)
			{
				/* Clever cascading approach.  Example: if the
				scaling factor was 9.5, do 3 "doubling" blends
				(8x), then scale by the remaining 1.1875. */
				glColor4f(1, 1, 1, 1);
				while (remainingScale > 2.0)
				{
					drawQuad();
					remainingScale /= 2.0;
				}
			}
			glColor4f(remainingScale - 1, remainingScale - 1, remainingScale - 1, 1);
			drawQuad();
			glBlendFunc(GL_ONE, GL_ONE);
			if (bias != 0)
			{
				if (bias > 0)
				{
					glColor4f(bias, bias, bias, 0.0);
				}
				else
				{
					if (GLSharedWidget::getInstance()->getHasBlendSubtract())
					{
						glBlendEquationEXT(GL_FUNC_REVERSE_SUBTRACT_EXT);
					}
					glColor4f(-bias, -bias, -bias, 0.0);
				}
				drawQuad();
			}
		}
		else
		{
			if (bias > 0)
			{
				if (GLSharedWidget::getInstance()->getHasBlendExt())
				{
					glBlendEquationEXT(GL_FUNC_ADD_EXT);
				}
				glColor4f(bias, bias, bias, scale);
			}
			else
			{
				if (GLSharedWidget::getInstance()->getHasBlendSubtract())
				{
					glBlendEquationEXT(GL_FUNC_REVERSE_SUBTRACT_EXT);
				}
				glColor4f(-bias, -bias, -bias, scale);
			}
			glBlendFunc(GL_ONE, GL_SRC_ALPHA);
			drawQuad();
		}
		glDisable(GL_BLEND);
	}
	if (State::getInstance()->getWorkspace() == UNDISTORTION)
	{
		if (camera->hasUndistortion())
		{
			camera->getUndistortionObject()->drawData(State::getInstance()->getUndistortionVisPoints());
		}
	}
	else if (State::getInstance()->getWorkspace() == CALIBRATION)
	{
		camera->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->draw(State::getInstance()->getCalibrationVisPoints());
		if (State::getInstance()->getCalibrationVisText() > 0 ||
			(!camera->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->isCalibrated() && WizardDockWidget::getInstance()->manualCalibrationRunning()))
		{
			renderPointText(true);
		}
	}
	else if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		if ((int)Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0)
		{
			Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->drawPoints(this->camera->getID(), detailedView);

			if (!detailedView)
			{
				if (Settings::getInstance()->getBoolSetting("TrialDrawRigidBodyConstellation"))
					Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->drawRigidBodies(this->camera);

				if (Settings::getInstance()->getBoolSetting("TrialDrawMarkerIds"))
				{
					renderPointText(false);
				}
			}
		}
	}
	if (State::getInstance()->getActiveCamera() == this->camera->getID())
	{
		WizardDockWidget::getInstance()->draw();
	}

	glFlush();
}

void GLCameraView::setZoomRatio(double newZoomRation, bool newAutozoom)
{
	newZoomRation = (newZoomRation > 100.0 / 999.0) ? newZoomRation : 100.0 / 999.0;
	if (zoomRatio != newZoomRation)
	{
		zoomRatio = newZoomRation;
		int newZoom = floor(100.0 / zoomRatio + 0.5);
		emit zoomChanged(newZoom);
	}
	if (autozoom != newAutozoom)
	{
		autozoom = newAutozoom;
		emit autozoomChanged(autozoom);
	}
}

