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
//  PROVIDED �AS IS�, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
//  FOR ANY PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR ANY 
//  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR FOR ANY DAMAGES WHATSOEVER RESULTING 
//  FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
//  OTHER TORTIOUS ACTION, OR ANY OTHER LEGAL THEORY, ARISING OUT OF OR IN CONNECTION 
//  WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
//  ----------------------------------
//  
///\file WorldViewDockGLWidget.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef WorldViewDockGLWidget_H_
#define WorldViewDockGLWidget_H_

#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QOpenGLFunctions_4_1_Core>
#include <QMatrix4x4>

namespace xma
{
	class Trial;
	class SimpleColorShader;
	class LitColorShader;
	class TexturedQuadShader;
	class GLSphere;

	class WorldViewDockGLWidget : public QOpenGLWidget, protected QOpenGLFunctions_4_1_Core
	{
		Q_OBJECT

	public:
		explicit WorldViewDockGLWidget(QWidget* parent = nullptr);
		~WorldViewDockGLWidget() override;

		void setUseCustomTimeline(bool value);
		void setFrame(int value);

	public slots:
		void animate();
		void setFocalPlaneDistance(float distance);
	protected:
		void paintGL() override;
		void initializeGL() override;
		void resizeGL(int w, int h) override;

		void mouseMoveEvent(QMouseEvent* e) override;
		void mousePressEvent(QMouseEvent* e) override;
		void wheelEvent(QWheelEvent* event) override;

	private:
		int w, h;

		double eyedistance;
		double azimuth;
		double polar;
		double prev_azi;
		double prev_pol;
		double focal_plane_distance;
		
		int frame;
		bool useCustomTimeline;

		void drawMarkers(Trial* trial, int frame);
		void drawRigidBodies(Trial* trial, int frame);

		void drawCalibrationCube();
		void drawCameras();

		// Modern OpenGL state
		bool opengl_initialised;
		
		// Transform matrices
		QMatrix4x4 m_projection;
		QMatrix4x4 m_view;
		QMatrix4x4 m_model;
		
		// Textured quad VAO/VBO for camera viewports
		GLuint m_quadVAO;
		GLuint m_quadVBO;
		bool m_quadInitialized;
		void initQuadVAO();
		void drawTexturedQuad3D(float x1, float y1, float x2, float y2, float z,
		                        float tx1, float ty1, float tx2, float ty2);
	};
}

#endif /* WORLDVIEWDOCKGLWIDGET_H_ */

