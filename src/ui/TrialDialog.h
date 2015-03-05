#ifndef TRIALDIALOG_H_
#define TRIALDIALOG_H_

#include <QDialog>

namespace Ui {
	class TrialDialog;
}

namespace xma{
	class 
		Trial;


	class TrialDialog : public QDialog{

		Q_OBJECT

	private:
		Ui::TrialDialog *diag;

		Trial * m_trial;

		bool isComplete();

	public:
		explicit TrialDialog(Trial* marker, QWidget *parent = 0);
		virtual ~TrialDialog();

		public slots:

		//Footer buttons
		void on_pushButton_OK_clicked();
		void on_pushButton_Cancel_clicked();
	};
}


#endif /* AboutDialog_H_ */
