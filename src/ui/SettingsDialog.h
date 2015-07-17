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
		bool initPhase;

		public slots :
		void on_pushButton_General_clicked();
		void on_pushButton_Calibration_clicked();
		void on_pushButton_Digitizing_clicked();

		void on_checkBox_AutoConfirmPendingChanges_stateChanged(int state);
		void on_checkBox_AutoCalibAfterReference_stateChanged(int state);
		void on_checkBox_UseCenteredDetailWindow_stateChanged(int state);
		void on_checkBox_ShowAdvancedCrosshairDetailWindow_stateChanged(int state);
		void on_checkBox_Show3DPointDetailWindow_stateChanged(int state);
		void on_checkBox_ShowEpiLineDetailWindow_stateChanged(int state);

		void on_spinBoxEpiPrecision_valueChanged(int value);
		void on_comboBox_TriangulationMethod_currentIndexChanged(int value);
		void on_comboBox_DetectionMethodForCalibration_currentIndexChanged(int value);
		void on_checkBox_ConfirmQuitXMALab_stateChanged(int state);
	};
}
#endif  // SETTINGSDIALOG_H
