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

namespace xma{
	class Camera;
	class CalibrationInfoFrame;
	class UndistortionInfoFrame;

	class CameraViewWidget : public QWidget{

		Q_OBJECT

	public:
		explicit CameraViewWidget(Camera * camera, QWidget *parent = 0);
		virtual ~CameraViewWidget();
		
		void setSharedGLContext(const QGLContext * sharedContext);
		void setMinimumWidthGL(bool set);

		void draw();
		void updateInfo();

		void setCameraName(QString name);
		void setImageName(QString name);

	protected:
		bool eventFilter(QObject *obj, QEvent *event);

		public slots :
		void on_toolButtonFitZoom_clicked(bool checked);
		void on_spinBoxZoom_valueChanged(int value);
		void on_toolButtonInfo_clicked(bool checked);

		void autozoomChanged(bool on);
		void zoomChanged(int zoom);

		void workspaceChanged(work_state workspace);
		void activeFrameCalibrationChanged(int activeFrame);
		void activeCameraChanged(int activeCamera);

	private:

		

		UndistortionInfoFrame * undistortionFrame;
		CalibrationInfoFrame * calibrationFrame;

		Camera * camera;

		Ui::CameraViewWidget *widget;

	};
}

#endif  // CAMERAVIEWWIDGET_H
