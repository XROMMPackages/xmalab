#ifndef GLWIDGET_H_
#define GLWIDGET_H_

#include <QGLWidget>

namespace xma{
	class Camera;

	class GLCameraView : public QGLWidget
	{
		Q_OBJECT

	public:
		GLCameraView(QWidget *parent);
		void setCamera(Camera * _camera);

		void setMinimumWidthGL(bool set);
		void setAutoZoom(bool on);
		void setZoom(int value);

		void setDetailedView();

		void setScale(double value);
		void setBias(double value);
		void centerViewToPoint();

	protected:
		void paintGL();
		void initializeGL();
		void resizeGL(int w, int h);

		void mouseMoveEvent(QMouseEvent *e);
		void mousePressEvent(QMouseEvent *e);
		void wheelEvent(QWheelEvent *event);
		void mouseDoubleClickEvent(QMouseEvent * event);

		void setZoomToFit();
		void setZoomTo100();


	private:
		bool detailedView;
		Camera * camera;
		void clampXY();

		int window_width, window_height;
		int camera_width, camera_height;
		double x_offset, y_offset;
		double prev_x, prev_y;

		double zoomRatio;
		bool autozoom;

		void setZoomRatio(double newZoomRation, bool autozoom = false);

		void renderTextCentered(QString string);
		void renderPointText();
		void drawTexture();
		void drawQuad();

		double x_test, y_test;
		double bias;
		double scale;

	public:
	signals :

		void autozoomChanged(bool on);
			void zoomChanged(int zoom);
	};
}

#endif /* PROGRESSDIALOG_H_ */
