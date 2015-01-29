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
		void moveDigitizationPoint(int camera, double x, double y, bool noDetection);

		void setDialog();

	private:
		Ui::WizardDigitizationFrame *frame;

		int trackID; //id for point or rb
		int tmptrackID;
		int trackType; //1 point, 2 rb, 2 all
		int trackDirection; // -2 back -1 prev, 0 none , 1 next, 2 forward
		bool singleTrack;

		void trackSinglePoint();
		void trackRB();
		void trackAll();

		void uncheckTrackButtons();

		public slots:
		void activeCameraChanged(int activeCamera);
		void activeFrameTrialChanged(int activeFrame);
		void workspaceChanged(work_state workspace);

		void on_pushButton_clicked();
		void on_pushButton_PointNext_clicked();
		void on_pushButton_PointPrev_clicked();
		void on_pushButton_PointForw_clicked(bool checked);
		void on_pushButton_PointBack_clicked(bool checked);

		void on_pushButton_RBNext_clicked();
		void on_pushButton_RBPrev_clicked();
		void on_pushButton_RBForw_clicked(bool checked);
		void on_pushButton_RBBack_clicked(bool checked);

		void on_pushButton_AllNext_clicked();
		void on_pushButton_AllPrev_clicked();
		void on_pushButton_AllForw_clicked(bool checked);
		void on_pushButton_AllBack_clicked(bool checked);

		void trackSinglePointFinished();
		void trackRBFinished();
		void trackAllFinished();

		void track();
	};
}


#endif /* WIZARDDIGITIZATIONFRAME_H_ */
