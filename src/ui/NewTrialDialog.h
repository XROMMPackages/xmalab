#ifndef NEWTRAILDIALOG_H_
#define NEWTRAILDIALOG_H_

#include <QDialog>

namespace Ui {
	class NewTrialDialog;
}

namespace xma{
	class CameraBoxTrial;

	class NewTrialDialog : public QDialog{

		Q_OBJECT

	public:
		explicit NewTrialDialog(QWidget *parent = 0);
		virtual ~NewTrialDialog();

		Ui::NewTrialDialog *diag;
		const std::vector <CameraBoxTrial *>& getCameras(){ return cameras; }

		bool createTrial();
		QString trialname;
	private:
		std::vector <CameraBoxTrial *> cameras;

		//checks if all inputs are done
		bool isComplete();

		public slots:
		//Footer buttons
		void on_pushButton_OK_clicked();
		void on_pushButton_Cancel_clicked();

		void on_lineEditTrialName_textChanged(QString text);
	};
}
#endif /* NEWTRAILDIALOG_H_ */
