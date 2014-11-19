/*
 * ProgressDialog.h
 *
 *  Created on: Nov 19, 2013
 *      Author: ben
 */

#ifndef GLWIDGET_H_
#define GLWIDGET_H_

#include <QGLWidget>

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

protected:
    void paintGL();
	void initializeGL();
    void resizeGL(int w, int h);

	void mouseMoveEvent(QMouseEvent *e);
	void mousePressEvent(QMouseEvent *e);
	void wheelEvent(QWheelEvent *event);
	void mouseDoubleClickEvent ( QMouseEvent * event );
	
	void setZoomToFit();
	void setZoomTo100();
	

private:
	Camera * camera;
	void clampXY();


	int window_width,window_height;
	int camera_width,camera_height;
	double x_offset,y_offset;
	double prev_x,prev_y;

	double zoomRatio;
	bool autozoom;

	void setZoomRatio(double newZoomRation, bool autozoom = false);

	void renderTextCentered(QString string);

	double x_test,y_test;

public: 
signals:
	
	void autozoomChanged(bool on);  
	void zoomChanged(int zoom); 
};



#endif /* PROGRESSDIALOG_H_ */
