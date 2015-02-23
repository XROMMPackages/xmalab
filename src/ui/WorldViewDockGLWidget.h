#ifndef WorldViewDockGLWidget_H_
#define WorldViewDockGLWidget_H_

#include <QGLWidget>

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

namespace xma{
	class Trial;

	class WorldViewDockGLWidget : public QGLWidget
	{
		Q_OBJECT

	public:
		WorldViewDockGLWidget(QWidget *parent);
		virtual ~WorldViewDockGLWidget();

		public slots:
		void animate();

	protected:
		void paintGL();
		void initializeGL();
		void resizeGL(int w, int h);

		void mouseMoveEvent(QMouseEvent *e);
		void mousePressEvent(QMouseEvent *e);
		void wheelEvent(QWheelEvent *event);

	private:
		int w, h;

		double eyedistance;
		double azimuth;
		double polar;
		double prev_azi;
		double prev_pol;

		void drawMarkers(Trial * trial, int frame);
		void drawRigidBodies(Trial * trial, int frame);

		void drawCalibrationCube();
		void drawCameras();

		GLUquadricObj *sphere_quadric;
		bool opengl_initialised;

	};
}


#endif /* PROGRESSDIALOG_H_ */
