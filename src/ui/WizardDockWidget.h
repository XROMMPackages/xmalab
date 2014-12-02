/*
 * ProgressDialog.h
 *
 *  Created on: Nov 19, 2013
 *      Author: ben
 */

#ifndef WIZARDDOCKWIDGET_H_
#define WIZARDDOCKWIDGET_H_

#include <QDockWidget>
#include "ui/State.h"

namespace Ui {
	class WizardDockWidget;
}

class WizardUndistortionFrame;
class WizardCalibrationCubeFrame;
class QLabel;

class WizardDockWidget : public QDockWidget{

	Q_OBJECT
	
	public:
		~WizardDockWidget();
		
		void updateFrames();
		static WizardDockWidget* getInstance();
		void addCalibrationReference(double x, double y);
		void draw();
		bool checkForPendingChanges();
		void update();

	private:
		Ui::WizardDockWidget *dock;
		static WizardDockWidget* instance;
		WizardDockWidget(QWidget *parent = 0);

		WizardUndistortionFrame * undistortionFrame;
		WizardCalibrationCubeFrame * calibrationFrame;

	public slots:
		void workspaceChanged(work_state workspace);

};



#endif /* WIZARDUNDISTORTIONFRAME_H_ */

