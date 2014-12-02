/*
 * OptionsVisualizationDialog.h
 *
 *  Created on: Nov 19, 2013
 *      Author: ben
 */

#ifndef CAMERAVIEWWIDGET_H
#define CAMERAVIEWWIDGET_H

#include <QWidget>
#include <QGLContext>
#include <QString>
#include "ui/State.h"

//forward declarations
namespace Ui {
	class CameraViewWidget;
}
class Camera;
class CalibrationInfoFrame;
class UndistortionInfoFrame;

class CameraViewWidget : public QWidget{

	Q_OBJECT

	public:
		explicit CameraViewWidget(Camera * camera, QWidget *parent = 0);
		~CameraViewWidget();

		Ui::CameraViewWidget *widget;

		void setSharedGLContext(const QGLContext * sharedContext);
		void setMinimumWidthGL(bool set);

		void draw();
		void updateInfo();

		void setCameraName(QString name);
		void setImageName(QString name);

	protected:

	public slots:
		void on_toolButtonFitZoom_clicked(bool checked);
		void on_spinBoxZoom_valueChanged(int value);
		void on_toolButtonInfo_clicked(bool checked);

		
		void autozoomChanged(bool on);
		void zoomChanged(int zoom);

		void workspaceChanged(work_state workspace);
		void activeFrameChanged(int activeFrame);
	private:

		UndistortionInfoFrame * undistortionFrame;
		CalibrationInfoFrame * calibrationFrame;

		Camera * camera;

};

#endif  // CAMERAVIEWWIDGET_H
