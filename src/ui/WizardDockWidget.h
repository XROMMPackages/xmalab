#ifndef WIZARDDOCKWIDGET_H_
#define WIZARDDOCKWIDGET_H_

#include <QDockWidget>
#include "ui/State.h"

namespace Ui {
	class WizardDockWidget;
}

class QLabel;

namespace xma{
	class WizardUndistortionFrame;
	class WizardCalibrationCubeFrame;
	class WizardDigitizationFrame;

	class WizardDockWidget : public QDockWidget{

		Q_OBJECT

	public:
		virtual ~WizardDockWidget();

		void update();
		static WizardDockWidget* getInstance();
		void addCalibrationReference(double x, double y);
		void addDigitizationPoint(int camera, double x, double y);
		void selectDigitizationPoint(int camera, double x, double y);
		void moveDigitizationPoint(int camera, double x, double y, bool noDetection);
		void draw();
		bool checkForPendingChanges();
		void updateDialog();
		void stop();
	
	private:
		Ui::WizardDockWidget *dock;
		static WizardDockWidget* instance;
		WizardDockWidget(QWidget *parent = 0);

		WizardUndistortionFrame * undistortionFrame;
		WizardCalibrationCubeFrame * calibrationFrame;
		WizardDigitizationFrame * digitizationFrame;

		public slots:
		void workspaceChanged(work_state workspace);
		void trackSelectedPointForward();
		void trackSelectedPointBackward();
		void goToLastTrackedFrame();
		void goToFirstTrackedFrame();
	};
}


#endif /* WIZARDUNDISTORTIONFRAME_H_ */

