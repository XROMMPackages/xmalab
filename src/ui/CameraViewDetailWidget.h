#ifndef CAMERAVIEWDETAILWIDGET_H
#define CAMERAVIEWDETAILWIDGET_H

#include <QWidget>
#include <QGLContext>
#include <QString>
#include "ui/State.h"

//forward declarations
namespace Ui {
	class CameraViewDetailWidget;
}

namespace xma{
	class Camera;

	class CameraViewDetailWidget : public QWidget{

		Q_OBJECT

	public:
		explicit CameraViewDetailWidget(Camera * camera, QWidget *parent = 0);
		virtual ~CameraViewDetailWidget();

		
		void setSharedGLContext(const QGLContext * sharedContext);
		void setMinimumWidthGL(bool set);
		void draw();

	public slots:
			void on_doubleSpinBoxBias_valueChanged(double value);
			void on_horizontalSliderBias_valueChanged(int value);
			void on_doubleSpinBoxScale_valueChanged(double value);
			void on_horizontalSliderScale_valueChanged(int value);

			void workspaceChanged(work_state workspace);
			void activeFrameTrialChanged(int);
			void activeTrialChanged(int);
			void activePointChanged(int);

	private:
		Camera * camera;

		Ui::CameraViewDetailWidget *widget;

	};
}

#endif  // CAMERAVIEWDETAILWIDGET_H
