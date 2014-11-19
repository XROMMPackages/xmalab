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

//forward declarations
namespace Ui {
	class CameraViewWidget;
}
class Camera;

class CameraViewWidget : public QWidget{

	Q_OBJECT

	public:
		explicit CameraViewWidget(Camera * camera, QWidget *parent = 0);
		~CameraViewWidget();

		Ui::CameraViewWidget *widget;

		void setSharedGLContext(const QGLContext * sharedContext);
		void setMinimumWidthGL(bool set);

		void draw();

		void setCameraName(QString name);
		void setImageName(QString name);

	protected:

	public slots:
		void on_toolButtonFitZoom_clicked(bool checked);
		void on_spinBoxZoom_valueChanged(int value);

		void autozoomChanged(bool on);
		void zoomChanged(int zoom);
	private:
};

#endif  // CAMERAVIEWWIDGET_H
