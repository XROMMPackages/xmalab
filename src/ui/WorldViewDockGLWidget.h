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

#ifndef XMA_USE_PAINTER
#include <QtOpenGLWidgets/QOpenGLWidget>
#else
#include <QWidget>
#endif

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

namespace xma
{
	class Trial;

#ifdef XMA_USE_PAINTER
	class WorldViewDockGLWidget : public QWidget
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
		void paintEvent(QPaintEvent* event) override;
		void resizeEvent(QResizeEvent* event) override;
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
	};
#else
	class WorldViewDockGLWidget : public QOpenGLWidget
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

		GLUquadricObj* sphere_quadric;
		bool opengl_initialised;
	};
#endif
}


#endif /* PROGRESSDIALOG_H_ */

