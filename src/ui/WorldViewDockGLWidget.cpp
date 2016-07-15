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

#include <QtGui/QApplication>
#include <QMouseEvent>

#ifndef _PI
#define _PI 3.141592653
#endif

using namespace xma;

GLfloat LightAmbient[] = {0.3f, 0.3f, 0.3f, 1.0f}; // Ambient Light Values
GLfloat LightDiffuse[] = {0.5f, 0.5f, 0.5f, 1.0f}; // Diffuse Light Values
GLfloat LightPosition[] = {0.0f, 10.0f, 0.0f, 1.0f}; // Light Position

WorldViewDockGLWidget::WorldViewDockGLWidget(QWidget* parent)
	: QGLWidget(QGLFormat(QGL::SampleBuffers), parent)
{
	eyedistance = 500.0;
	azimuth = 45.0;
	polar = -45.0;

	w = 50;
	h = 50;
	setMinimumSize(50, 50);
	setAutoFillBackground(false);
	opengl_initialised = false;
}

void WorldViewDockGLWidget::animate()
{
	//    repaint();
}

WorldViewDockGLWidget::~WorldViewDockGLWidget()
{
	if (opengl_initialised)gluDeleteQuadric(sphere_quadric);
}

void WorldViewDockGLWidget::mouseMoveEvent(QMouseEvent* e)
{
	if (e->buttons() & Qt::LeftButton)
	{
		azimuth -= prev_azi - e->posF().y();
		polar -= prev_pol - e->posF().x();

		azimuth = (azimuth > 180) ? 180.0 : azimuth;
		azimuth = (azimuth < 0) ? 0.0 : azimuth;

		while (polar > 360) polar = polar - 360.0;
		while (polar < 0) polar = polar + 360.0;

		prev_azi = e->posF().y();
		prev_pol = e->posF().x();
		updateGL();
	}
}


void WorldViewDockGLWidget::mousePressEvent(QMouseEvent* e)
{
	if (e->buttons() & Qt::LeftButton)
	{
		prev_azi = e->posF().y();
		prev_pol = e->posF().x();
	}
}

void WorldViewDockGLWidget::wheelEvent(QWheelEvent* e)
{
	eyedistance += e->delta() / 12.0;
	updateGL();
}

void WorldViewDockGLWidget::initializeGL()
{
	glShadeModel(GL_SMOOTH); // Enable Smooth Shading
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Black Background
	glClearDepth(1.0f); // Depth Buffer Setup
	glEnable(GL_DEPTH_TEST); // Enables Depth Testing
	glDepthFunc(GL_LEQUAL); // The Type Of Depth Testing To Do
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST); // Really Nice Perspective Calculations

	glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmbient); // Setup The Ambient Light
	glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuse); // Setup The Diffuse Light
	glLightfv(GL_LIGHT1, GL_POSITION, LightPosition); // Position The Light
	glEnable(GL_LIGHT1);
	glEnable(GL_LIGHTING);

	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT,GL_AMBIENT_AND_DIFFUSE);
}

void WorldViewDockGLWidget::resizeGL(int _w, int _h)
{
	w = _w;
	h = _h;

	glViewport(0, 0, w, h);
}

void WorldViewDockGLWidget::paintGL()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(25.0, (double(w)) / h, 1.0, 100000.0);

	double e_z = eyedistance * cos(polar * _PI / 180.0) * sin(azimuth * _PI / 180.0);
	double e_x = eyedistance * sin(polar * _PI / 180.0) * sin(azimuth * _PI / 180.0);
	double e_y = eyedistance * cos(azimuth * _PI / 180.0);

	gluLookAt(e_x, e_y, e_z, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
	glRotated(-90.0, 1.0, 0.0, 0.0);
	glRotated(180.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	glLineWidth(2.5);
	glColor3f(1.0, 0.0, 0.0);
	glBegin(GL_LINES);
	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(100.0, 0, 0);
	glEnd();

	glColor3f(0.0, 1.0, 0.0);
	glBegin(GL_LINES);
	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(0, 100.0, 0);
	glEnd();

	glColor3f(0.0, 0.0, 1.0);
	glBegin(GL_LINES);
	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(0, 0, 100.0);
	glEnd();

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
					int frame = State::getInstance()->getActiveFrameTrial();

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
	for (unsigned int cam = 0; cam < Project::getInstance()->getCameras().size(); cam++)
	{
		if (Project::getInstance()->getCameras()[cam]->isCalibrated()
			&& Project::getInstance()->getCameras()[cam]->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->isCalibrated())
		{
			glPushMatrix();
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
			transTmp.at<double>(0, 2) = -transTmp.at<double>(0, 2);
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
			glMultMatrixd(m);

			//Draw CO
			glColor3f(1.0, 0.0, 0.0);
			glBegin(GL_LINES);
			glVertex3f(0.0, 0.0, 0.0);
			glVertex3f(10.0, 0, 0);
			glEnd();

			glColor3f(0.0, 1.0, 0.0);
			glBegin(GL_LINES);
			glVertex3f(0.0, 0.0, 0.0);
			glVertex3f(0, 10.0, 0);
			glEnd();

			glColor3f(0.0, 0.0, 1.0);
			glBegin(GL_LINES);
			glVertex3f(0.0, 0.0, 0.0);
			glVertex3f(0, 0, 10.0);
			glEnd();


			//Draw Boundaries
			double x_min = (0 - camTmp.at<double>(0, 2)) / camTmp.at<double>(0, 0);
			double x_max = (Project::getInstance()->getCameras()[cam]->getWidth() - camTmp.at<double>(0, 2)) / camTmp.at<double>(0, 0);
			double y_min = (0 - camTmp.at<double>(1, 2)) / camTmp.at<double>(1, 1);
			double y_max = (Project::getInstance()->getCameras()[cam]->getHeight() - camTmp.at<double>(1, 2)) / camTmp.at<double>(1, 1);

			double z = -200.0;
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			glColor4f(1.0f, 1.0f, 1.0f, 0.2f);
			glBegin(GL_LINES);
			glVertex3f(0.0f, 0.0f, 0.0f);
			glVertex3f(z * x_min, z * y_min, z);

			glVertex3f(0.0f, 0.0f, 0.0f);
			glVertex3f(z * x_min, z * y_max, z);

			glVertex3f(0.0f, 0.0f, 0.0f);
			glVertex3f(z * x_max, z * y_min, z);

			glVertex3f(0.0f, 0.0f, 0.0f);
			glVertex3f(z * x_max, z * y_max, z);
			glEnd();

			glBegin(GL_LINE_LOOP);
			glVertex3f(z * x_min, z * y_min, z);
			glVertex3f(z * x_min, z * y_max, z);
			glVertex3f(z * x_max, z * y_max, z);
			glVertex3f(z * x_max, z * y_min, z);
			glEnd();
			glDisable(GL_BLEND);

			glEnable(GL_TEXTURE_2D);


			if (State::getInstance()->getWorkspace() == CALIBRATION)
			{
				Project::getInstance()->getCameras()[cam]->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->bindTexture(State::getInstance()->getCalibrationVisImage());
			}
			else if (State::getInstance()->getWorkspace() == DIGITIZATION)
			{
				if (Project::getInstance()->getTrials().size() > 0 && State::getInstance()->getActiveTrial() >= 0 &&
					State::getInstance()->getActiveTrial() < (int) Project::getInstance()->getTrials().size())
				{
					Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getVideoStreams()[cam]->getImage()->bindTexture();
				}
			}

			glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3f(z * x_min, z * y_min, z); // bottom left
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(z * x_max, z * y_min, z); // bottom right
			glTexCoord2f(1.0f, 0.0f);
			glVertex3f(z * x_max, z * y_max, z);// top right
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(z * x_min, z * y_max, z); // top left
			glEnd();
			glBindTexture(GL_TEXTURE_2D, 0);
			glDisable(GL_TEXTURE_2D);

			glPopMatrix();

			camTmp.release();
			rotTmp.release();
			transTmp.release();

			glDisable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}
}

void WorldViewDockGLWidget::drawMarkers(Trial* trial, int frame)
{
	if (!opengl_initialised)
	{
		sphere_quadric = gluNewQuadric(); // Create A Pointer To The Quadric Object ( NEW )
		gluQuadricNormals(sphere_quadric, GLU_SMOOTH); // Create Smooth Normals ( NEW )
		gluQuadricTexture(sphere_quadric, GL_TRUE); // Create Texture Coords ( NEW )
	}

	for (unsigned int i = 0; i < trial->getMarkers().size(); i++)
	{
		if (trial->getMarkers()[i]->getStatus3D()[frame] > 0){
			glPushMatrix();
			if (i == trial->getActiveMarkerIdx())
			{
				glColor3f(1.0, 0.0, 0.0);
			}
			else
			{
				glColor3f(0.0, 1.0, 0.0);
			}

			glTranslated(trial->getMarkers()[i]->getPoints3D()[frame].x,
				trial->getMarkers()[i]->getPoints3D()[frame].y,
				trial->getMarkers()[i]->getPoints3D()[frame].z);

			gluSphere(sphere_quadric, 0.3f, 32, 32);

			glPopMatrix();
		}
	}
}

void WorldViewDockGLWidget::drawRigidBodies(Trial* trial, int frame)
{
	for (unsigned int i = 0; i < trial->getRigidBodies().size(); i++)
	{
		trial->getRigidBodies()[i]->draw3D(frame);
	}
}

void WorldViewDockGLWidget::drawCalibrationCube()
{
	if (!opengl_initialised)
	{
		sphere_quadric = gluNewQuadric(); // Create A Pointer To The Quadric Object ( NEW )
		gluQuadricNormals(sphere_quadric, GLU_SMOOTH); // Create Smooth Normals ( NEW )
		gluQuadricTexture(sphere_quadric, GL_TRUE); // Create Texture Coords ( NEW )
	}

	if (CalibrationObject::getInstance()->isInitialised() && !CalibrationObject::getInstance()->isCheckerboard())
	{
		for (unsigned int i = 0; i < CalibrationObject::getInstance()->getFrameSpecifications().size(); i++)
		{
			glPushMatrix();
			if (i == CalibrationObject::getInstance()->getReferenceIDs()[0])
			{
				glColor3f(1.0, 0.0, 0.0);
			}
			else if (i == CalibrationObject::getInstance()->getReferenceIDs()[1])
			{
				glColor3f(0.0, 1.0, 0.0);
			}
			else if (i == CalibrationObject::getInstance()->getReferenceIDs()[2])
			{
				glColor3f(0.0, 0.0, 1.0);
			}
			else if (i == CalibrationObject::getInstance()->getReferenceIDs()[3])
			{
				glColor3f(1.0, 1.0, 0.0);
			}
			else
			{
				glColor3f(1.0, 1.0, 1.0);
			}

			glTranslated(CalibrationObject::getInstance()->getFrameSpecifications()[i].x,
			             CalibrationObject::getInstance()->getFrameSpecifications()[i].y,
			             CalibrationObject::getInstance()->getFrameSpecifications()[i].z);

			gluSphere(sphere_quadric, 0.3f, 32, 32);

			glPopMatrix();
		}
	}
}

