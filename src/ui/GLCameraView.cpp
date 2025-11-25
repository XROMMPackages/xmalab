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
//  PROVIDED -AS IS-, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
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

#include "gl/MultisampleFrameBuffer.h"
#include "gl/DistortionShader.h"
#include "gl/BlendShader.h"
#include "gl/ShaderManager.h"
#include "gl/TexturedQuadShader.h"
#include "gl/MeshShader.h"
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
#include "core/CalibrationObject.h"
#include "core/Image.h"
#include "core/Trial.h"
#include "core/Project.h"
#include "core/Marker.h"
#include "core/RigidBody.h"
#include "core/Settings.h"
#include "core/CalibrationSequence.h"
#include "processing/MarkerDetection.h"


#include <QMouseEvent>
#include <QPainter>

#include <iostream> 
#include <math.h>
#include "MainWindow.h"


using namespace xma;

GLCameraView::GLCameraView(QWidget* parent)
	: QOpenGLWidget(parent)
	, m_quadVAO(0)
	, m_quadVBO(0)
	, m_quadInitialized(false)
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
	makeCurrent();
	
	if (blendShader)
		delete blendShader;

	if (rigidbodyBufferUndistorted)
		delete rigidbodyBufferUndistorted;

	if (distortionShader)
		delete distortionShader;
		
	if (m_quadInitialized) {
		glDeleteVertexArrays(1, &m_quadVAO);
		glDeleteBuffers(1, &m_quadVBO);
	}
	
	doneCurrent();
}

void GLCameraView::setCamera(Camera* _camera)
{
	camera = _camera;

	camera_width = camera->getWidth();
	camera_height = camera->getHeight();

	if (!detailedView){
		if (rigidbodyBufferUndistorted)
			delete rigidbodyBufferUndistorted;

			rigidbodyBufferUndistorted = new FrameBuffer(camera_width, camera_height);

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
	qreal devicePixelRatio = this->devicePixelRatio();
	double effectiveWidth = window_width * devicePixelRatio;
	double effectiveHeight = window_height * devicePixelRatio;
	
	if (camera_width < effectiveWidth * zoomRatio)
	{
		if (x_offset < -0.5 * (zoomRatio * effectiveWidth)) x_offset = -0.5 * (zoomRatio * effectiveWidth);
		if (x_offset > -camera_width + 0.5 * (zoomRatio * effectiveWidth)) x_offset = -camera_width + 0.5 * (zoomRatio * effectiveWidth);
	}
	else
	{
		if (x_offset > -0.5 * (zoomRatio * effectiveWidth)) x_offset = -0.5 * (zoomRatio * effectiveWidth);
		if (x_offset < -camera_width + 0.5 * (zoomRatio * effectiveWidth)) x_offset = -camera_width + 0.5 * (zoomRatio * effectiveWidth);
	}

	if (camera_height < effectiveHeight * zoomRatio)
	{
		if (y_offset < -0.5 * (zoomRatio * effectiveHeight)) y_offset = -0.5 * (zoomRatio * effectiveHeight);
		if (y_offset > -camera_height + 0.5 * (zoomRatio * effectiveHeight)) y_offset = -camera_height + 0.5 * (zoomRatio * effectiveHeight);
	}
	else
	{
		if (y_offset > -0.5 * (zoomRatio * effectiveHeight)) y_offset = -0.5 * (zoomRatio * effectiveHeight);
		if (y_offset < -camera_height + 0.5 * (zoomRatio * effectiveHeight)) y_offset = -camera_height + 0.5 * (zoomRatio * effectiveHeight);
	}
}

void GLCameraView::mouseMoveEvent(QMouseEvent* e)
{
	if (e->buttons() & Qt::RightButton)
	{
		qreal devicePixelRatio = this->devicePixelRatio();
		x_offset -= (prev_x - zoomRatio * e->pos().x() * devicePixelRatio);
		y_offset -= (prev_y - zoomRatio * e->pos().y() * devicePixelRatio);

		prev_y = zoomRatio * e->pos().y() * devicePixelRatio;
		prev_x = zoomRatio * e->pos().x() * devicePixelRatio;

		clampXY();
		update();
	}
}

void GLCameraView::mouseDoubleClickEvent(QMouseEvent* e)
{
	State::getInstance()->changeActiveCamera(this->camera->getID());
	if (e->buttons() & Qt::LeftButton)
	{
		if (State::getInstance()->getWorkspace() == CALIBRATION
			&& camera->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->isCalibrated() == 1)
		{
			qreal devicePixelRatio = this->devicePixelRatio();
			double x = zoomRatio * (e->pos().x() * devicePixelRatio) - ((window_width * devicePixelRatio) * zoomRatio * 0.5 + x_offset);
			double y = zoomRatio * (e->pos().y() * devicePixelRatio) - ((window_height * devicePixelRatio) * zoomRatio * 0.5 + y_offset);
			camera->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->toggleInlier(x, y, State::getInstance()->getCalibrationVisImage() == DISTORTEDCALIBIMAGE);
			update();
		}
	}
}

void GLCameraView::mousePressEvent(QMouseEvent* e)
{
	State::getInstance()->changeActiveCamera(this->camera->getID());
	qreal devicePixelRatio = this->devicePixelRatio();
	
	if (e->buttons() & Qt::RightButton)
	{
		prev_y = zoomRatio * e->pos().y() * devicePixelRatio;
		prev_x = zoomRatio * e->pos().x() * devicePixelRatio;
	}
	else if (e->buttons() & Qt::LeftButton)
	{
		double x = zoomRatio * (e->pos().x() * devicePixelRatio) - ((window_width * devicePixelRatio) * zoomRatio * 0.5 + x_offset) - 0.5;
		double y = zoomRatio * (e->pos().y() * devicePixelRatio) - ((window_height * devicePixelRatio) * zoomRatio * 0.5 + y_offset) - 0.5;
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
					cv::Point out = MarkerDetection::detectionPoint(camera->getCalibrationSequence()->getImage(State::getInstance()->getActiveFrameCalibration(), true), method, cv::Point2d(x, y), 40, 5);
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
				if (camera->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->isCalibrated() != 1)
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
				setTransparency(transparency + 1.0 / 2400.0 * e->angleDelta().y());
				emit transparencyChanged(transparency);
			}
		}
		else{
			State::getInstance()->changeActiveCamera(this->camera->getID());
			double zoom_prev = zoomRatio;

			setZoomRatio(zoomRatio * 1 - e->angleDelta().y() / 1000.0, false);

			QPoint coordinatesGlobal = e->globalPosition().toPoint();
			QPoint coordinates = this->mapFromGlobal(coordinatesGlobal);
			qreal devicePixelRatio = this->devicePixelRatio();

			y_offset += (zoom_prev - zoomRatio) * (0.5 * window_height * devicePixelRatio - coordinates.y() * devicePixelRatio);
			x_offset += (zoom_prev - zoomRatio) * (0.5 * window_width * devicePixelRatio - coordinates.x() * devicePixelRatio);

			update();
		}
	}
}

void GLCameraView::initializeGL()
{
	initializeOpenGLFunctions();
	
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0f);
	glDisable(GL_DEPTH_TEST);
	
	// Initialize shader manager
	ShaderManager::getInstance()->initializeShaders();
}

void GLCameraView::initQuadVAO()
{
	if (m_quadInitialized) return;
	
	glGenVertexArrays(1, &m_quadVAO);
	glGenBuffers(1, &m_quadVBO);
	
	glBindVertexArray(m_quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
	
	// Allocate space for quad data (position xy + texcoord uv) * 6 vertices
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 6, nullptr, GL_DYNAMIC_DRAW);
	
	// Position attribute (location 0)
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	
	// TexCoord attribute (location 1)
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	m_quadInitialized = true;
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
	qreal devicePixelRatio = this->devicePixelRatio();
	double effectiveWidth = window_width * devicePixelRatio;
	double effectiveHeight = window_height * devicePixelRatio;
	
	setZoomRatio((((double) camera_width) / effectiveWidth > ((double) camera_height) / effectiveHeight) ? ((double) camera_width) / effectiveWidth : ((double) camera_height) / effectiveHeight, autozoom);

	x_offset = -0.5 * (zoomRatio * effectiveWidth);
	y_offset = -0.5 * (zoomRatio * effectiveHeight);

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
		qreal devicePixelRatio = this->devicePixelRatio();
		this->setMinimumWidth(ratio * this->size().height() / devicePixelRatio);
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

	// Handle high DPI displays by using device pixel ratio
	qreal devicePixelRatio = this->devicePixelRatio();
	glViewport(0, 0, window_width * devicePixelRatio, window_height * devicePixelRatio);
	if (autozoom)setZoomToFit();
}

bool GLCameraView::projectTextPos(double objx, double objy, double objz,
	const QMatrix4x4& mvp,
	const int viewport[4],
	double* winx, double* winy, double* winz)
{
	QVector4D in(objx, objy, objz, 1.0);
	QVector4D out = mvp * in;

	if (out.w() == 0.0)
		return false;

	out /= out.w();

	*winx = viewport[0] + (1 + out.x()) * viewport[2] / 2;
	*winy = viewport[1] + (1 + out.y()) * viewport[3] / 2;

	*winz = (1 + out.z()) / 2;
	return true;
}

void GLCameraView::renderText(double x, double y, double z, const QString &str, QColor fontColor , const QFont & font) {
	int width = this->width();
	int height = this->height();

	int view[4];
	glGetIntegerv(GL_VIEWPORT, view);
	
	QMatrix4x4 mvp = m_projection * m_view;
	double textPosX = 0, textPosY = 0, textPosZ = 0;

	if (!projectTextPos(x, y, z, mvp, view, &textPosX, &textPosY, &textPosZ))
		return;

	// Handle high DPI displays: convert from device coordinates to widget coordinates
	qreal devicePixelRatio = this->devicePixelRatio();
	textPosX /= devicePixelRatio;
	textPosY /= devicePixelRatio;
	
	textPosY = height - textPosY; // y is inverted

	QPainter painter(this);
	painter.setPen(fontColor);
	painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
	painter.drawText(textPosX, textPosY, str); // z = pointT4.z + distOverOp / 4
	painter.end();
}

void GLCameraView::renderTextCentered(QString string)
{
	setFont(QFont(this->font().family(), 24));
	QFontMetrics fm(this->font());
	renderText(-zoomRatio * fm.horizontalAdvance(string) * 0.5 - x_offset,
               -zoomRatio * fm.height() * 0.5 - y_offset,
               0.0, string, QColor(255, 0, 0));
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
			((!camera->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->isCalibrated() == 1)
				&& WizardDockWidget::getInstance()->manualCalibrationRunning()) ? IDCALIBTEXT : State::getInstance()->getCalibrationVisText(), State::getInstance()->getCalibrationVisImage() == DISTORTEDCALIBIMAGE, x, y, text, inlier);
		setFont(QFont(this->font().family(), 15.0));
		QFontMetrics fm(this->font());

		if (State::getInstance()->getCalibrationVisText() == 4) {
			for (unsigned int i = 0; i < x.size(); i++)
			{
				text[i] = CalibrationObject::getInstance()->getMarkerNames()[i];
			}
		}

		if (State::getInstance()->getCalibrationVisText() != 3)
		{
			for (unsigned int i = 0; i < x.size(); i++)
			{
				QColor color;
				if (inlier[i])
				{
					color =  QColor(0, 255, 0);
				}
				else
				{
					color = QColor(255, 0, 0);
				}
				renderText(x[i] + 3, y[i], 0.0, text[i], color);
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
				renderText(x[i] + 3, y[i], 0.0, text[i], QColor(255.0 * val / max, 255.0 * (1.0 - val / max), 0));
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
			QColor color;
			if (Settings::getInstance()->getBoolSetting("ShowColoredMarkerIDs"))
			{
				color = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getMarkers()[i]->getStatusColor(this->camera->getID(), State::getInstance()->getActiveFrameTrial());
			}
			else{
				if (active == i)
				{
					color = QColor(255, 0, 0);
				}
				else
				{
					color = QColor(0, 255, 0);
				}
			}
			renderText(x[i] + 3, y[i], 0.0, text[i], color);
		}
	}

	x.clear();
	y.clear();
	text.clear();
}

void GLCameraView::drawTexture()
{
	bool drawImage = true;
	// glEnable(GL_TEXTURE_2D) not needed in modern OpenGL - texturing is shader-based
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
	else if ((State::getInstance()->getWorkspace() == CALIBRATION) && (Project::getInstance()->getCalibration() == INTERNAL))
	{
		camera->getCalibrationSequence()->bindTexture(State::getInstance()->getActiveFrameCalibration(),State::getInstance()->getCalibrationVisImage());
	}
	else if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		if ((int)Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0)
		{
			if (!Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getIsDefault())
				Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getVideoStreams()[camera->getID()]->bindTexture();
		}
	}
	if (drawImage)
	{
		if (State::getInstance()->getWorkspace() != DIGITIZATION ||( State::getInstance()->getActiveTrial() != -1 && !Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getIsDefault()))
			drawQuad();
	}
	glBindTexture(GL_TEXTURE_2D, 0);
}

void GLCameraView::drawQuad()
{
	initQuadVAO();
	
	// Using TexturedQuadShader
	TexturedQuadShader* texShader = ShaderManager::getInstance()->getTexturedQuadShader();
	texShader->bind();
	
	QMatrix4x4 mvp = m_projection * m_view;
	texShader->setMVP(mvp);
	texShader->setScaleBias(1.0f, 0.0f);
	
	// Draw quad using the shader's method
	texShader->drawQuadWithTexCoords(-0.5f, -0.5f, camera_width - 0.5f, camera_height - 0.5f,
	                                  0.0f, 0.0f, 1.0f, 1.0f);
	
	texShader->release();
}

void GLCameraView::centerViewToPoint(bool resetZoom)
{
	if (State::getInstance()->getWorkspace() == DIGITIZATION)
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
			if (!detailedView && Settings::getInstance()->getBoolSetting("TrialDrawRigidBodyMeshmodels") && !Settings::getInstance()->getBoolSetting("TrialDrawHideAll"))
			{
				renderMeshes = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->renderMeshes() && 
					!Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getIsDefault();
			}
		}

		renderMeshes = renderMeshes && (Project::getInstance()->getCalibration() != NO_CALIBRATION);

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
						glEnable(GL_DEPTH_TEST);
						
						if (renderTransparentModels){
							glDepthFunc(GL_ALWAYS);
							glEnable(GL_BLEND);
							glBlendFunc(GL_ZERO, GL_SRC_COLOR);
						}
						else
						{
							glDepthFunc(GL_LEQUAL);
						}
						
						// Set up mesh shader with lighting
						MeshShader* meshShader = ShaderManager::getInstance()->getMeshShader();
						meshShader->bind();
						meshShader->setLight1Position(QVector3D(LightPosition_front[0], LightPosition_front[1], LightPosition_front[2]));
						meshShader->setLight1Ambient(QVector3D(LightAmbient[0], LightAmbient[1], LightAmbient[2]));
						meshShader->setLight1Diffuse(QVector3D(LightDiffuse[0], LightDiffuse[1], LightDiffuse[2]));
						meshShader->setLight2Position(QVector3D(LightPosition_back[0], LightPosition_back[1], LightPosition_back[2]));
						meshShader->setLight2Ambient(QVector3D(LightAmbient[0], LightAmbient[1], LightAmbient[2]));
						meshShader->setLight2Diffuse(QVector3D(LightDiffuse[0], LightDiffuse[1], LightDiffuse[2]));

						glViewport(0, 0, rigidbodyBufferUndistorted->getWidth(), rigidbodyBufferUndistorted->getHeight());
						double proj[16];
						double model[16];

						camera->getGLTransformations(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getReferenceCalibrationImage(), &proj[0], &model[0]);

						// Convert double arrays to QMatrix4x4 (column-major)
						QMatrix4x4 projMatrix(
							proj[0], proj[4], proj[8], proj[12],
							proj[1], proj[5], proj[9], proj[13],
							proj[2], proj[6], proj[10], proj[14],
							proj[3], proj[7], proj[11], proj[15]
						);
						QMatrix4x4 modelMatrix(
							model[0], model[4], model[8], model[12],
							model[1], model[5], model[9], model[13],
							model[2], model[6], model[10], model[14],
							model[3], model[7], model[11], model[15]
						);
						
						meshShader->setProjectionMatrix(projMatrix);
						meshShader->setViewMatrix(modelMatrix);
						meshShader->setModelMatrix(QMatrix4x4()); // Identity for now

						Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->drawRigidBodiesMesh(projMatrix, modelMatrix);
						
						meshShader->release();

						glDisable(GL_BLEND);
						glDisable(GL_DEPTH_TEST);
						rigidbodyBufferUndistorted->unbindFrameBuffer();
					}

					distortionShader->draw(rigidbodyBufferUndistorted->getTextureID(), rigidbodyBufferUndistorted->getDepthTextureID(), transparency);
					doDistortion = distortionShader->canRender();
				}
			}
		}
	}
	qreal devicePixelRatio = this->devicePixelRatio();
	glViewport(0, 0, window_width * devicePixelRatio, window_height * devicePixelRatio);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClearDepth(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDisable(GL_DEPTH_TEST);
	
	if (detailedView && Settings::getInstance()->getBoolSetting("CenterDetailView")) centerViewToPoint();

	double effectiveWidth = window_width * devicePixelRatio;
	double effectiveHeight = window_height * devicePixelRatio;
	
	// Set up orthographic projection
	m_projection.setToIdentity();
	m_projection.ortho(-0.5f * (zoomRatio * effectiveWidth) - x_offset - 0.5f,
	                    0.5f * (zoomRatio * effectiveWidth) - x_offset - 0.5f,
	                    0.5f * (zoomRatio * effectiveHeight) - y_offset - 0.5f,
	                   -0.5f * (zoomRatio * effectiveHeight) - y_offset - 0.5f,
	                   -1000.0f, 1000.0f);

	// Set up view matrix
	m_view.setToIdentity();
	m_view.lookAt(QVector3D(0, 0, 1), QVector3D(0, 0, 0), QVector3D(0, 1, 0));

	drawTexture();

	if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		if (Settings::getInstance()->getBoolSetting("TrialDrawFiltered") && (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getCutoffFrequency() <= 0 || Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getRecordingSpeed() <= 0)){
		 setFont(QFont(this->font().family(), 12));
		 QFontMetrics fm(this->font());
		 QString string("Rendering of filtered data is enabled, but framrate and cutoff are not set correctly");
		 renderText(-zoomRatio * fm.horizontalAdvance(string) * 0.5 - x_offset,
           -zoomRatio * (fm.height() - window_height) * 0.5 - y_offset,
           0.0, string, QColor(255, 0, 0));
		}

		if (renderMeshes){
			if ((int)Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0)
			{
				if (!detailedView)
				{
					if (!doDistortion){
						setFont(QFont(this->font().family(), 12));
						QFontMetrics fm(this->font());
						QString string("Rigid body models are not distorted! XMALab is currently computing the distortion!");
						renderText(-zoomRatio * fm.horizontalAdvance(string) * 0.5 - x_offset,
           -zoomRatio * (fm.height() - window_height) * 0.5 - y_offset,
           0.0, string, QColor(255, 0, 0));
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

	// Scale/bias is applied in the shader now for the main texture
	// The old blending approach is replaced by shader-based scale/bias in drawQuad()
	// For backward compatibility, we still support the blending approach here
	// but in OpenGL 4.1 core, these blend equations are standard
	if ((bias != 0.0 || scale != 1.0))
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
			// In OpenGL 4.1 core, glBlendEquation is standard
			glBlendEquation(GL_FUNC_ADD);
			glBlendFunc(GL_DST_COLOR, GL_ONE);
			if (remainingScale > 2.0)
			{
				/* Clever cascading approach.  Example: if the
				scaling factor was 9.5, do 3 "doubling" blends
				(8x), then scale by the remaining 1.1875. */
				// TODO: Update scale/bias rendering to use shader
				while (remainingScale > 2.0)
				{
					drawQuad();
					remainingScale /= 2.0;
				}
			}
			drawQuad();
			glBlendFunc(GL_ONE, GL_ONE);
			if (bias != 0)
			{
				if (bias < 0)
				{
					glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
				}
				drawQuad();
			}
		}
		else
		{
			if (bias > 0)
			{
				glBlendEquation(GL_FUNC_ADD);
			}
			else
			{
				glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
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
			QMatrix4x4 mvp = m_projection * m_view;
			camera->getUndistortionObject()->drawData(State::getInstance()->getUndistortionVisPoints(), mvp);
		}
	}
	else if ((State::getInstance()->getWorkspace() == CALIBRATION && Project::getInstance()->getCalibration() == INTERNAL))
	{
		QMatrix4x4 mvp = m_projection * m_view;
		camera->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->draw(State::getInstance()->getCalibrationVisPoints(), mvp);
		if (State::getInstance()->getCalibrationVisText() > 0 ||
			((!camera->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->isCalibrated() == 1)
				&& WizardDockWidget::getInstance()->manualCalibrationRunning()))
		{
			renderPointText(true);
		}
	}
	else if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		if (!Settings::getInstance()->getBoolSetting("TrialDrawHideAll")){
			if ((int)Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0)
			{
				QMatrix4x4 mvp = m_projection * m_view;
				Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->drawPoints(this->camera->getID(), detailedView, mvp);

				if (!detailedView)
				{
					if (Settings::getInstance()->getBoolSetting("TrialDrawRigidBodyConstellation"))
						Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->drawRigidBodies(this->camera, mvp);
				}
				if (!detailedView || Settings::getInstance()->getBoolSetting("ShowIDsInDetail"))
				{
					if (Settings::getInstance()->getBoolSetting("TrialDrawMarkerIds"))
					{
						renderPointText(false);
					}
				}
			}
		}
	}
	if (State::getInstance()->getActiveCamera() == this->camera->getID())
	{
		QMatrix4x4 mvp = m_projection * m_view;
		WizardDockWidget::getInstance()->draw(mvp);
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

