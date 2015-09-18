#ifndef FROMTODIALOG_H_
#define FROMTODIALOG_H_

#include <QDialog>
#include <QString>

namespace Ui {
	class FromToDialog;
}

namespace xma{
	class FromToDialog : public QDialog{

		Q_OBJECT

	private:
		Ui::FromToDialog *diag;

	public:
		FromToDialog(int from, int to, int max, bool withFormat, QWidget *parent = 0);
		virtual ~FromToDialog();
		int getFrom();
		int getTo();
		QString getFormat();

		public slots:
		void on_pushButton_OK_clicked();
		void on_pushButton_Cancel_clicked();
	};
}


#endif /* CONFIRMATIONDIALOG_H_ */
