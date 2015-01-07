#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
namespace Ui {
	class SettingsDialog;
}

namespace xma{
	class SettingsDialog : public QDialog{

		Q_OBJECT

	public:
		explicit SettingsDialog(QWidget *parent = 0);
		virtual ~SettingsDialog();
		Ui::SettingsDialog *diag;

	protected:

	private:

		public slots :
			void on_pushButton_Workflow_clicked();
		void on_pushButton_Blobdetection_clicked();

		void on_checkBox_AutoConfirmPendingChanges_stateChanged(int state);
		void on_checkBox_AutoCalibAfterReference_stateChanged(int state);
	};
}
#endif  // SETTINGSDIALOG_H
