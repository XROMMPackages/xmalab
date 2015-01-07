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

		void updateFrames();
		static WizardDockWidget* getInstance();
		void addCalibrationReference(double x, double y);
		void addDigitizationPoint(int camera, double x, double y);
		void moveDigitizationPoint(int camera, double x, double y);
		void draw();
		bool checkForPendingChanges();
		void update();

	private:
		Ui::WizardDockWidget *dock;
		static WizardDockWidget* instance;
		WizardDockWidget(QWidget *parent = 0);

		WizardUndistortionFrame * undistortionFrame;
		WizardCalibrationCubeFrame * calibrationFrame;
		WizardDigitizationFrame * digitizationFrame;

		public slots:
		void workspaceChanged(work_state workspace);

	};
}


#endif /* WIZARDUNDISTORTIONFRAME_H_ */

