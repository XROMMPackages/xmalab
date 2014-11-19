/*
 * ProgressDialog.h
 *
 *  Created on: Nov 19, 2013
 *      Author: ben
 */

#ifndef WIZARDUNDISTORTIONFRAME_H_
#define WIZARDUNDISTORTIONFRAME_H_

#include <QFrame>
#include "ui/State.h"

namespace Ui {
	class WizardUndistortionFrame;
}

class WizardUndistortionFrame : public QFrame{

	Q_OBJECT
	
	public:
		~WizardUndistortionFrame();
		WizardUndistortionFrame(QWidget *parent = 0);

		bool checkForPendingChanges();

	private:
		Ui::WizardUndistortionFrame *frame;

	public slots:
		void undistortionChanged(undistortion_state undistortion);
		void on_pushButton_clicked();

		void on_comboBoxImage_currentIndexChanged(int idx);
		void on_comboBoxPoints_currentIndexChanged(int idx);
		void on_radioButtonMouseClickCenter_clicked(bool checked);
		void on_radioButtonMouseClickOutlier_clicked(bool checked);
		void on_radioButtonMouseClickNone_clicked(bool checked);
		void computeUndistortion();
		void recomputeUndistortion();
		void undistortionFinished();
};



#endif /* WIZARDUNDISTORTIONFRAME_H_ */
