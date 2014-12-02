/*
 * ProgressDialog.h
 *
 *  Created on: Nov 19, 2013
 *      Author: ben
 */

#ifndef PROGRESSDIALOG_H_
#define PROGRESSDIALOG_H_

#include <QDockWidget>


namespace Ui {
	class ProgressDockWidget;
}

class ProgressDialog : public QDockWidget{

	Q_OBJECT

	private:
		Ui::ProgressDockWidget *diag;
		static ProgressDialog* instance;

	protected:
		ProgressDialog(QWidget *parent = 0);
	public:
		~ProgressDialog();
		static ProgressDialog* getInstance();

		void setProgress(double progress);
		void showProgressbar(int min, int max, const char* key = "Computing");
		void closeProgressbar();

	public slots:
};



#endif /* PROGRESSDIALOG_H_ */
