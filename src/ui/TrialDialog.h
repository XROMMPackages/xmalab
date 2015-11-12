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
		bool deleteTrial;
		bool updateTrial;

	public:
		explicit TrialDialog(Trial* marker, QWidget *parent = 0);
		virtual ~TrialDialog();

		bool doDeleteTrial();
		bool doUpdateTrial();

		public slots:

		//Footer buttons
		void on_pushButton_OK_clicked();
		void on_pushButton_Cancel_clicked();
		void on_pushButton_DeleteTrial_clicked();
		void on_pushButton_Update_clicked();
	};
}


#endif /* AboutDialog_H_ */
