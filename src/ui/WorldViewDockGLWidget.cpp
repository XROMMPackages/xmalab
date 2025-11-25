//  ----------------------------------
//  XMALab -- Copyright (c) 2015, Brown University, Providence, RI.
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
//  PROVIDED "AS IS", INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
//  FOR ANY PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR ANY 
//  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR FOR ANY DAMAGES WHATSOEVER RESULTING 
//  FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
//  OTHER TORTIOUS ACTION, OR ANY OTHER LEGAL THEORY, ARISING OUT OF OR IN CONNECTION 
//  WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
//  ----------------------------------
//  
///\file WorldViewDockGLWidget.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/State.h"
#include "ui/WorldViewDockGLWidget.h"

#include "core/Project.h"
#include "core/CalibrationImage.h"
#include "core/Camera.h"
#include "core/CalibrationObject.h"
#include "core/UndistortionObject.h"
#include "core/Image.h"
#include "core/Trial.h"
#include "core/Marker.h"
#include "core/RigidBody.h"
#include "core/CalibrationSequence.h"

#include "gl/ShaderManager.h"
#include "gl/SimpleColorShader.h"
#include "gl/LitColorShader.h"
#include "gl/TexturedQuadShader.h"

#include <QApplication>
#include <QMouseEvent>
#include "GLSharedWidget.h"

#include <cmath>

#ifndef _PI
#define _PI 3.141592653
#endif

using namespace xma;

WorldViewDockGLWidget::WorldViewDockGLWidget(QWidget* parent)
	: QOpenGLWidget(parent), useCustomTimeline(false), frame(0),
	  m_quadVAO(0), m_quadVBO(0), m_quadInitialized(false)
{
	eyedistance = 500.0;
	azimuth = 45.0;
	polar = -45.0;
	focal_plane_distance = 200;
	
	w = 50;
	h = 50;
	setMinimumSize(50, 50);
	setAutoFillBackground(false);
	opengl_initialised = false;
}

void WorldViewDockGLWidget::setFrame(int value)
{
	frame = value;
}

void WorldViewDockGLWidget::animate()
{
	//    repaint();
}

void WorldViewDockGLWidget::setFocalPlaneDistance(float distance)
{
	focal_plane_distance = distance;
}

WorldViewDockGLWidget::~WorldViewDockGLWidget()
{
	makeCurrent();
	if (m_quadInitialized) {
		glDeleteVertexArrays(1, &m_quadVAO);
		glDeleteBuffers(1, &m_quadVBO);
	}
	doneCurrent();
}

void WorldViewDockGLWidget::setUseCustomTimeline(bool value)
{
	useCustomTimeline = value;
}

void WorldViewDockGLWidget::mouseMoveEvent(QMouseEvent* e)
{
	if (e->buttons() & Qt::LeftButton)
	{
		azimuth -= prev_azi - e->pos().y();
		polar -= prev_pol - e->pos().x();

		azimuth = (azimuth > 180) ? 180.0 : azimuth;
		azimuth = (azimuth < 0) ? 0.0 : azimuth;

		while (polar > 360) polar = polar - 360.0;
		while (polar < 0) polar = polar + 360.0;

		prev_azi = e->pos().y();
		prev_pol = e->pos().x();
		update();
	}
}


void WorldViewDockGLWidget::mousePressEvent(QMouseEvent* e)
{
	if (e->buttons() & Qt::LeftButton)
	{
		prev_azi = e->pos().y();
		prev_pol = e->pos().x();
	}
}

void WorldViewDockGLWidget::wheelEvent(QWheelEvent* e)
{
	eyedistance += e->angleDelta().y() / 12.0;
	update();
}

void WorldViewDockGLWidget::initializeGL()
{
	initializeOpenGLFunctions();
	
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	
	// Initialize shader manager
	ShaderManager::getInstance()->initializeShaders();
	
	opengl_initialised = true;
}

void WorldViewDockGLWidget::initQuadVAO()
{
	if (m_quadInitialized) return;
	
	glGenVertexArrays(1, &m_quadVAO);
	glGenBuffers(1, &m_quadVBO);
	
	glBindVertexArray(m_quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
	
	// Allocate space for dynamic quad data (position xyz + texcoord uv) * 6 vertices
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 5 * 6, nullptr, GL_DYNAMIC_DRAW);
	
	// Position attribute (location 0)
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	
	// TexCoord attribute (location 1)
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	m_quadInitialized = true;
}

void WorldViewDockGLWidget::drawTexturedQuad3D(float x1, float y1, float x2, float y2, float z,
                                                float tx1, float ty1, float tx2, float ty2)
{
	initQuadVAO();
	
	// Two triangles forming a quad
	float vertices[] = {
		// Position (x, y, z)      TexCoord (u, v)
		z * x1, z * y1, z,         tx1, ty2,   // bottom-left
		z * x2, z * y1, z,         tx2, ty2,   // bottom-right
		z * x2, z * y2, z,         tx2, ty1,   // top-right
		
		z * x1, z * y1, z,         tx1, ty2,   // bottom-left
		z * x2, z * y2, z,         tx2, ty1,   // top-right
		z * x1, z * y2, z,         tx1, ty1,   // top-left
	};
	
	glBindVertexArray(m_quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
	
	glDrawArrays(GL_TRIANGLES, 0, 6);
	
	glBindVertexArray(0);
}

void WorldViewDockGLWidget::resizeGL(int _w, int _h)
{
	w = _w;
	h = _h;

	// Handle high DPI displays by using device pixel ratio
	qreal devicePixelRatio = this->devicePixelRatio();
	glViewport(0, 0, w * devicePixelRatio, h * devicePixelRatio);
}

void WorldViewDockGLWidget::paintGL()
{
	// Set up projection matrix
	m_projection.setToIdentity();
	m_projection.perspective(25.0f, float(w) / float(h), 1.0f, 100000.0f);

	double e_z = eyedistance * cos(polar * _PI / 180.0) * sin(azimuth * _PI / 180.0);
	double e_x = eyedistance * sin(polar * _PI / 180.0) * sin(azimuth * _PI / 180.0);
	double e_y = eyedistance * cos(azimuth * _PI / 180.0);


	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// Set up view matrix
	m_view.setToIdentity();
	m_view.lookAt(QVector3D(e_x, e_y, e_z), QVector3D(0.0, 0.0, 0.0), QVector3D(0.0, 1.0, 0.0));
	m_view.rotate(-90.0f, 1.0f, 0.0f, 0.0f);
	m_view.rotate(180.0f, 0.0f, 0.0f, 1.0f);
	
	// Set up model matrix (identity for now)
	m_model.setToIdentity();

	glBindTexture(GL_TEXTURE_2D, 0);

	// Draw coordinate axes using SimpleColorShader
	SimpleColorShader* colorShader = ShaderManager::getInstance()->getSimpleColorShader();
	colorShader->bind();
	
	QMatrix4x4 mvp = m_projection * m_view * m_model;
	colorShader->setMVP(mvp);
	
	glLineWidth(2.5);
	
	// X axis (red)
	colorShader->setColor(1.0f, 0.0f, 0.0f);
	colorShader->drawLine(QVector3D(0.0f, 0.0f, 0.0f), QVector3D(100.0f, 0.0f, 0.0f));
	
	// Y axis (green)
	colorShader->setColor(0.0f, 1.0f, 0.0f);
	colorShader->drawLine(QVector3D(0.0f, 0.0f, 0.0f), QVector3D(0.0f, 100.0f, 0.0f));
	
	// Z axis (blue)
	colorShader->setColor(0.0f, 0.0f, 1.0f);
	colorShader->drawLine(QVector3D(0.0f, 0.0f, 0.0f), QVector3D(0.0f, 0.0f, 100.0f));
	
	colorShader->release();

	//////////DRAW
	if (this->isVisible())
	{
		if (State::getInstance()->getWorkspace() == CALIBRATION)
		{
			drawCalibrationCube();
			drawCameras();
		}
		else if (State::getInstance()->getWorkspace() == DIGITIZATION)
		{
			if (Project::getInstance()->getTrials().size() > 0 && State::getInstance()->getActiveTrial() >= 0 &&
				State::getInstance()->getActiveTrial() < (int) Project::getInstance()->getTrials().size())
			{
				Trial* trial = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()];
				if (trial->getStartFrame() - 1 <= State::getInstance()->getActiveFrameTrial()
					&& trial->getEndFrame() - 1 >= State::getInstance()->getActiveFrameTrial())
				{
					drawCameras();
					drawMarkers(trial, frame);
					drawRigidBodies(trial, frame);
				}
			}
		}
	}
	glFlush();
}

void WorldViewDockGLWidget::drawCameras()
{
	SimpleColorShader* colorShader = ShaderManager::getInstance()->getSimpleColorShader();
	TexturedQuadShader* texShader = ShaderManager::getInstance()->getTexturedQuadShader();
	
	for (unsigned int cam = 0; cam < Project::getInstance()->getCameras().size(); cam++)
	{
		if (Project::getInstance()->getCameras()[cam]->isCalibrated()
			&& (Project::getInstance()->getCameras()[cam]->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->isCalibrated() > 0))
		{
			double m[16];
			//inversere Rotation = transposed rotation
			//and opengl requires transposed, so we set R
			cv::Mat transTmp;
			cv::Mat rotTmp;
			cv::Mat camTmp;
			camTmp.create(3, 3, CV_64F);
			rotTmp.create(3, 3, CV_64F);
			transTmp.create(3, 1, CV_64F);

			camTmp = Project::getInstance()->getCameras()[cam]->getCameraMatrix().clone();
			transTmp = Project::getInstance()->getCameras()[cam]->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->getTranslationVector();
			cv::Rodrigues(Project::getInstance()->getCameras()[cam]->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->getRotationVector(), rotTmp);

			//adjust y - inversion
			transTmp.at<double>(0, 0) = -transTmp.at<double>(0, 0);
			transTmp.at<double>(2, 0) = -transTmp.at<double>(2, 0);
			for (int i = 0; i < 3; i++)
			{
				rotTmp.at<double>(0, i) = -rotTmp.at<double>(0, i);
				rotTmp.at<double>(2, i) = -rotTmp.at<double>(2, i);
			}
			camTmp.at<double>(1, 2) = (Project::getInstance()->getCameras()[cam]->getHeight() - 1) - camTmp.at<double>(1, 2);

			for (unsigned int y = 0; y < 3; y++)
			{
				m[y * 4] = rotTmp.at<double>(y, 0);
				m[y * 4 + 1] = rotTmp.at<double>(y, 1);
				m[y * 4 + 2] = rotTmp.at<double>(y, 2);
				m[y * 4 + 3] = 0.0;
			}

			//inverse translation = translation rotated with inverse rotation/transposed rotation
			//R-1 * -t = R^tr * -t
			m[12] = m[0] * -transTmp.at<double>(0, 0)
				+ m[4] * -transTmp.at<double>(1, 0)
				+ m[8] * -transTmp.at<double>(2, 0);
			m[13] = m[1] * -transTmp.at<double>(0, 0)
				+ m[5] * -transTmp.at<double>(1, 0)
				+ m[9] * -transTmp.at<double>(2, 0);
			m[14] = m[2] * -transTmp.at<double>(0, 0)
				+ m[6] * -transTmp.at<double>(1, 0)
				+ m[10] * -transTmp.at<double>(2, 0);
			m[15] = 1.0;
			
			// Create camera transform matrix
			QMatrix4x4 camTransform(
				m[0], m[4], m[8], m[12],
				m[1], m[5], m[9], m[13],
				m[2], m[6], m[10], m[14],
				m[3], m[7], m[11], m[15]
			);
			
			QMatrix4x4 mvp = m_projection * m_view * m_model * camTransform;

			// Draw camera coordinate axes
			colorShader->bind();
			colorShader->setMVP(mvp);
			
			colorShader->setColor(1.0f, 0.0f, 0.0f);
			colorShader->drawLine(QVector3D(0.0f, 0.0f, 0.0f), QVector3D(10.0f, 0.0f, 0.0f));
			
			colorShader->setColor(0.0f, 1.0f, 0.0f);
			colorShader->drawLine(QVector3D(0.0f, 0.0f, 0.0f), QVector3D(0.0f, 10.0f, 0.0f));
			
			colorShader->setColor(0.0f, 0.0f, 1.0f);
			colorShader->drawLine(QVector3D(0.0f, 0.0f, 0.0f), QVector3D(0.0f, 0.0f, 10.0f));

			//Draw Boundaries
			double x_min = (0 - camTmp.at<double>(0, 2)) / camTmp.at<double>(0, 0);
			double x_max = (Project::getInstance()->getCameras()[cam]->getWidth() - camTmp.at<double>(0, 2)) / camTmp.at<double>(0, 0);
			double y_min = (0 - camTmp.at<double>(1, 2)) / camTmp.at<double>(1, 1);
			double y_max = (Project::getInstance()->getCameras()[cam]->getHeight() - camTmp.at<double>(1, 2)) / camTmp.at<double>(1, 1);

			double z = -focal_plane_distance;
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			colorShader->setColor(1.0f, 1.0f, 1.0f, 0.2f);
			
			// Draw frustum lines from origin to corners
			std::vector<QVector3D> frustumLines = {
				QVector3D(0.0f, 0.0f, 0.0f), QVector3D(z * x_min, z * y_min, z),
				QVector3D(0.0f, 0.0f, 0.0f), QVector3D(z * x_min, z * y_max, z),
				QVector3D(0.0f, 0.0f, 0.0f), QVector3D(z * x_max, z * y_min, z),
				QVector3D(0.0f, 0.0f, 0.0f), QVector3D(z * x_max, z * y_max, z)
			};
			colorShader->drawLines(frustumLines);
			
			// Draw rectangle at focal plane
			std::vector<QVector3D> rectPoints = {
				QVector3D(z * x_min, z * y_min, z),
				QVector3D(z * x_min, z * y_max, z),
				QVector3D(z * x_max, z * y_max, z),
				QVector3D(z * x_max, z * y_min, z)
			};
			colorShader->drawLineLoop(rectPoints);
			
			colorShader->release();
			glDisable(GL_BLEND);

			// Draw textured quad at focal plane
			if (State::getInstance()->getWorkspace() == CALIBRATION)
			{
				Project::getInstance()->getCameras()[cam]->getCalibrationSequence()->bindTexture(State::getInstance()->getActiveFrameCalibration(),State::getInstance()->getCalibrationVisImage());
			}
			else if (State::getInstance()->getWorkspace() == DIGITIZATION)
			{
				if (Project::getInstance()->getTrials().size() > 0 && State::getInstance()->getActiveTrial() >= 0 &&
					State::getInstance()->getActiveTrial() < (int) Project::getInstance()->getTrials().size())
				{
					if (!Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getIsDefault())
						if (!useCustomTimeline)
						Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getVideoStreams()[cam]->getImage()->bindTexture();
				}
			}

			// Use a simple textured quad shader with 3D support
			// We need to draw the quad in 3D space with texture
			texShader->bind();
			texShader->setMVP(mvp);
			texShader->setScaleBias(1.0f, 0.0f);
			
			// Draw the textured quad manually (need 3D version)
			drawTexturedQuad3D(x_min, y_min, x_max, y_max, z, 0.0f, 1.0f, 1.0f, 0.0f);
			
			texShader->release();
			glBindTexture(GL_TEXTURE_2D, 0);

			camTmp.release();
			rotTmp.release();
			transTmp.release();
		}
	}
}

void WorldViewDockGLWidget::drawMarkers(Trial* trial, int frame)
{
	LitColorShader* litShader = ShaderManager::getInstance()->getLitColorShader();
	litShader->bind();
	litShader->setProjectionMatrix(m_projection);
	litShader->setViewMatrix(m_view);
	litShader->setLightPosition(QVector3D(0.0f, 10.0f, 0.0f));
	litShader->setAmbientColor(QVector3D(0.3f, 0.3f, 0.3f));
	litShader->setDiffuseColor(QVector3D(0.5f, 0.5f, 0.5f));

	for (unsigned int i = 0; i < trial->getMarkers().size(); i++)
	{
		if (trial->getMarkers()[i]->getStatus3D()[frame] > 0){
			if (i == trial->getActiveMarkerIdx())
			{
				litShader->setColor(1.0f, 0.0f, 0.0f);
			}
			else
			{
				litShader->setColor(0.0f, 1.0f, 0.0f);
			}

			QVector3D pos(trial->getMarkers()[i]->getPoints3D()[frame].x,
				trial->getMarkers()[i]->getPoints3D()[frame].y,
				trial->getMarkers()[i]->getPoints3D()[frame].z);

			litShader->drawSphere(pos, 0.3f);
		}
	}
	litShader->release();
}

void WorldViewDockGLWidget::drawRigidBodies(Trial* trial, int frame)
{
	for (unsigned int i = 0; i < trial->getRigidBodies().size(); i++)
	{
		trial->getRigidBodies()[i]->draw3D(frame, m_projection, m_view);

		if (trial->getRigidBodies()[i]->getDrawMeshModel())
			trial->getRigidBodies()[i]->drawMesh(frame, m_projection, m_view);
	}
}

void WorldViewDockGLWidget::drawCalibrationCube()
{
	if (CalibrationObject::getInstance()->isInitialised() && !CalibrationObject::getInstance()->isCheckerboard())
	{
		LitColorShader* litShader = ShaderManager::getInstance()->getLitColorShader();
		litShader->bind();
		litShader->setProjectionMatrix(m_projection);
		litShader->setViewMatrix(m_view);
		litShader->setLightPosition(QVector3D(0.0f, 10.0f, 0.0f));
		litShader->setAmbientColor(QVector3D(0.3f, 0.3f, 0.3f));
		litShader->setDiffuseColor(QVector3D(0.5f, 0.5f, 0.5f));
		
		for (unsigned int i = 0; i < CalibrationObject::getInstance()->getFrameSpecifications().size(); i++)
		{
			if (i == CalibrationObject::getInstance()->getReferenceIDs()[0])
			{
				litShader->setColor(1.0f, 0.0f, 0.0f);
			}
			else if (i == CalibrationObject::getInstance()->getReferenceIDs()[1])
			{
				litShader->setColor(0.0f, 1.0f, 0.0f);
			}
			else if (i == CalibrationObject::getInstance()->getReferenceIDs()[2])
			{
				litShader->setColor(0.0f, 0.0f, 1.0f);
			}
			else if (i == CalibrationObject::getInstance()->getReferenceIDs()[3])
			{
				litShader->setColor(1.0f, 1.0f, 0.0f);
			}
			else
			{
				litShader->setColor(1.0f, 1.0f, 1.0f);
			}

			QVector3D pos(CalibrationObject::getInstance()->getFrameSpecifications()[i].x,
			              CalibrationObject::getInstance()->getFrameSpecifications()[i].y,
			              CalibrationObject::getInstance()->getFrameSpecifications()[i].z);

			litShader->drawSphere(pos, 0.3f);
		}
		litShader->release();
	}
}

