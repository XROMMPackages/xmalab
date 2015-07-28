#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/SettingsDialog.h"
#include "ui_SettingsDialog.h"

#include "core/Settings.h"
#include "core/Project.h"
#include "core/Trial.h"

using namespace xma;

SettingsDialog::SettingsDialog(QWidget *parent) :
								QDialog(parent),
								diag(new Ui::SettingsDialog){

	diag->setupUi(this);
	initPhase = true;
	
	diag->scrollArea_Calibration->hide();
	diag->scrollArea_Digitizing->hide();

	diag->checkBox_AutoConfirmPendingChanges->setChecked(Settings::getInstance()->getBoolSetting("AutoConfirmPendingChanges"));
	diag->checkBox_ConfirmQuitXMALab->setChecked(Settings::getInstance()->getBoolSetting("ConfirmQuitXMALab"));
	diag->checkBox_AutoCalibAfterReference->setChecked(Settings::getInstance()->getBoolSetting("AutoCalibAfterReference"));

	diag->checkBox_UseCenteredDetailWindow->setChecked(Settings::getInstance()->getBoolSetting("CenterDetailView"));
	diag->checkBox_ShowAdvancedCrosshairDetailWindow->setChecked(Settings::getInstance()->getBoolSetting("AdvancedCrosshairDetailView"));
	diag->checkBox_Show3DPointDetailWindow->setChecked(Settings::getInstance()->getBoolSetting("Show3dPointDetailView"));
	diag->checkBox_ShowEpiLineDetailWindow->setChecked(Settings::getInstance()->getBoolSetting("ShowEpiLineDetailView"));

	diag->spinBoxEpiPrecision->setValue(Settings::getInstance()->getIntSetting("EpipolarLinePrecision"));
	diag->comboBox_TriangulationMethod->setCurrentIndex(Settings::getInstance()->getIntSetting("TriangulationMethod"));
	diag->comboBox_DetectionMethodForCalibration->setCurrentIndex(Settings::getInstance()->getIntSetting("DetectionMethodForCalibration"));
	diag->doubleSpinBox_MaxError->setValue(Settings::getInstance()->getFloatSetting("MaximumReprojectionError"));
	initPhase = false;
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
	Settings::getInstance()->set("AutoConfirmPendingChanges", diag->checkBox_AutoConfirmPendingChanges->isChecked());
}
void SettingsDialog::on_checkBox_AutoCalibAfterReference_stateChanged(int state){
	Settings::getInstance()->set("AutoCalibAfterReference", diag->checkBox_AutoCalibAfterReference->isChecked());
}
void SettingsDialog::on_checkBox_UseCenteredDetailWindow_stateChanged(int state){
	Settings::getInstance()->set("CenterDetailView", diag->checkBox_UseCenteredDetailWindow->isChecked());
}

void SettingsDialog::on_checkBox_ShowAdvancedCrosshairDetailWindow_stateChanged(int state)
{
	Settings::getInstance()->set("AdvancedCrosshairDetailView", diag->checkBox_ShowAdvancedCrosshairDetailWindow->isChecked());
}

void SettingsDialog::on_checkBox_ConfirmQuitXMALab_stateChanged(int state)
{
	Settings::getInstance()->set("ConfirmQuitXMALab", diag->checkBox_ConfirmQuitXMALab->isChecked());
}

void SettingsDialog::on_doubleSpinBox_MaxError_valueChanged(double value)
{
	Settings::getInstance()->set("MaximumReprojectionError", (float) diag->doubleSpinBox_MaxError->value());
}

void SettingsDialog::on_checkBox_Show3DPointDetailWindow_stateChanged(int state)
{
	Settings::getInstance()->set("Show3dPointDetailView",diag->checkBox_Show3DPointDetailWindow->isChecked());
}

void SettingsDialog::on_checkBox_ShowEpiLineDetailWindow_stateChanged(int state)
{
	Settings::getInstance()->set("ShowEpiLineDetailView", diag->checkBox_ShowEpiLineDetailWindow->isChecked());
}

void SettingsDialog::on_spinBoxEpiPrecision_valueChanged(int value)
{
	Settings::getInstance()->set("EpipolarLinePrecision", diag->spinBoxEpiPrecision->value());
}

void SettingsDialog::on_comboBox_TriangulationMethod_currentIndexChanged(int value)
{
	Settings::getInstance()->set("TriangulationMethod", diag->comboBox_TriangulationMethod->currentIndex());
	if (!initPhase)
	{
		for (int i = 0; i < Project::getInstance()->getTrials().size(); i++)
		{
			Project::getInstance()->getTrials()[i]->update();
		}
	}
}



void SettingsDialog::on_comboBox_DetectionMethodForCalibration_currentIndexChanged(int value)
{
	Settings::getInstance()->set("DetectionMethodForCalibration", diag->comboBox_DetectionMethodForCalibration->currentIndex());
	if (!initPhase)
	{
		for (int i = 0; i < Project::getInstance()->getTrials().size(); i++)
		{
			Project::getInstance()->getTrials()[i]->update();
		}
	}
}