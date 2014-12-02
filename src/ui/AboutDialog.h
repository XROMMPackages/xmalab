/*
 * AboutDialog.h
 *
 *  Created on: Nov 19, 2013
 *      Author: ben
 */

#ifndef AboutDialog_H_
#define AboutDialog_H_



#include <QDialog>

namespace Ui {
	class AboutDialog;
}

class AboutDialog : public QDialog{

	Q_OBJECT

	private:
		Ui::AboutDialog *diag;

	public:
		explicit AboutDialog(QWidget *parent = 0);
		~AboutDialog();

	public slots:
};



#endif /* AboutDialog_H_ */
