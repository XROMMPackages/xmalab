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
	diag->scrollArea_Blobdetection->hide();

	diag->checkBox_AutoConfirmPendingChanges->setChecked(Settings::getAutoConfirmPendingChanges());
	diag->checkBox_AutoCalibAfterReference->setChecked(Settings::getAutoCalibAfterReference());
}

SettingsDialog::~SettingsDialog(){
	delete diag;
}

void SettingsDialog::on_pushButton_Workflow_clicked(){
	diag->scrollArea_Workflow->show();
	diag->scrollArea_Blobdetection->hide();
}

void SettingsDialog::on_pushButton_Blobdetection_clicked(){
	diag->scrollArea_Workflow->hide();
	diag->scrollArea_Blobdetection->show();
}

void SettingsDialog::on_checkBox_AutoConfirmPendingChanges_stateChanged(int state){
	Settings::setAutoConfirmPendingChanges(diag->checkBox_AutoConfirmPendingChanges->isChecked());
}
void SettingsDialog::on_checkBox_AutoCalibAfterReference_stateChanged(int state){
	Settings::setAutoCalibAfterReference(diag->checkBox_AutoCalibAfterReference->isChecked());
}