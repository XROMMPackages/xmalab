/*
 * ProgressDialog.h
 *
 *  Created on: Nov 19, 2013
 *      Author: ben
 */

#ifndef ERRORDIALOG_H_
#define ERRORDIALOG_H_

#include <QDialog>
#include <QString>

namespace Ui {
	class ErrorDialog;
}

class ErrorDialog : public QDialog{

	Q_OBJECT

	private:
		Ui::ErrorDialog *diag;
		static ErrorDialog* instance;

	protected:
		ErrorDialog(QWidget *parent = 0);

	public:
		~ErrorDialog();
		static ErrorDialog* getInstance();

		void showErrorDialog(QString message);

	public slots:
};



#endif /* ErrorDialogDIALOG_H_ */
