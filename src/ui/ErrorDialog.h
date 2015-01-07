#ifndef ERRORDIALOG_H_
#define ERRORDIALOG_H_

#include <QDialog>
#include <QString>

namespace Ui {
	class ErrorDialog;
}

namespace xma{
	class ErrorDialog : public QDialog{

		Q_OBJECT

	private:
		Ui::ErrorDialog *diag;
		static ErrorDialog* instance;

	protected:
		ErrorDialog(QWidget *parent = 0);

	public:
		virtual ~ErrorDialog();
		static ErrorDialog* getInstance();

		void showErrorDialog(QString message);

		public slots:
	};
}

#endif /* ErrorDialogDIALOG_H_ */
