/*
 * ConfirmationDialog.h
 *
 *  Created on: Nov 19, 2013
 *      Author: ben
 */

#ifndef CONFIRMATIONDIALOG_H_
#define CONFIRMATIONDIALOG_H_

#include <QDialog>
#include <QString>

namespace Ui {
	class ConfirmationDialog;
}

class ConfirmationDialog : public QDialog{

	Q_OBJECT

	private:
		Ui::ConfirmationDialog *diag;
		static ConfirmationDialog* instance;

	protected:
		ConfirmationDialog(QWidget *parent = 0);

	public:
		~ConfirmationDialog();
		static ConfirmationDialog* getInstance();

		bool showConfirmationDialog(QString message);


	public slots:
		void on_pushButton_OK_clicked();
		void on_pushButton_Cancel_clicked();
};



#endif /* CONFIRMATIONDIALOG_H_ */
