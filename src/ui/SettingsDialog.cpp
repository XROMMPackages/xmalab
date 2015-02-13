#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/SettingsDialog.h"
#include "ui_SettingsDialog.h"

#include "core/Settings.h"

using namespace xma;

SettingsDialog::SettingsDialog(QWidget *parent) :
								QDialog(parent),
								diag(new Ui::SettingsDialog){

	diag->setupUi(this);
	diag->scrollArea_Calibration->hide();
	diag->scrollArea_Digitizing->hide();

	diag->checkBox_AutoConfirmPendingChanges->setChecked(Settings::getAutoConfirmPendingChanges());
	diag->checkBox_AutoCalibAfterReference->setChecked(Settings::getAutoCalibAfterReference());
	diag->checkBox_UseCenteredDetailWindow->setChecked(Settings::getCenterDetailView());
}

SettingsDialog::~SettingsDialog(){
	delete diag;
}

void SettingsDialog::on_pushButton_General_clicked(){
	diag->scrollArea_General->show();
	diag->scrollArea_Calibration->hide();
	diag->scrollArea_Digitizing->hide();
}

void SettingsDialog::on_pushButton_Calibration_clicked(){
	diag->scrollArea_General->hide();
	diag->scrollArea_Calibration->show();
	diag->scrollArea_Digitizing->hide();
}

void SettingsDialog::on_pushButton_Digitizing_clicked(){
	diag->scrollArea_General->hide();
	diag->scrollArea_Calibration->hide();
	diag->scrollArea_Digitizing->show();
}

void SettingsDialog::on_checkBox_AutoConfirmPendingChanges_stateChanged(int state){
	Settings::setAutoConfirmPendingChanges(diag->checkBox_AutoConfirmPendingChanges->isChecked());
}
void SettingsDialog::on_checkBox_AutoCalibAfterReference_stateChanged(int state){
	Settings::setAutoCalibAfterReference(diag->checkBox_AutoCalibAfterReference->isChecked());
}
void SettingsDialog::on_checkBox_UseCenteredDetailWindow_stateChanged(int state){
	Settings::setCenterDetailView(diag->checkBox_UseCenteredDetailWindow->isChecked());
}