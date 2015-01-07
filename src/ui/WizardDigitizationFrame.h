#ifndef WIZARDDIGITIZATIONFRAME_H_
#define WIZARDDIGITIZATIONFRAME_H_

#include <QFrame>
#include "ui/State.h"

namespace Ui {
	class WizardDigitizationFrame;
}

namespace xma{
	class WizardDigitizationFrame : public QFrame{

		Q_OBJECT

	public:
		virtual ~WizardDigitizationFrame();
		WizardDigitizationFrame(QWidget *parent = 0);

		void addDigitizationPoint(int camera, double x, double y);
		void moveDigitizationPoint(int camera, double x, double y);

	private:
		Ui::WizardDigitizationFrame *frame;
		void setDialog();

		public slots:
		void activeCameraChanged(int activeCamera);
		void activeFrameCalibrationChanged(int activeFrame);
		void workspaceChanged(work_state workspace);

		void on_pushButton_clicked();
	};
}


#endif /* WIZARDDIGITIZATIONFRAME_H_ */
