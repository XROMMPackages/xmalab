//  ----------------------------------
//  XMA Lab -- Copyright © 2015, Brown University, Providence, RI.
//  
//  All Rights Reserved
//   
//  Use of the XMA Lab software is provided under the terms of the GNU General Public License version 3 
//  as published by the Free Software Foundation at http://www.gnu.org/licenses/gpl-3.0.html, provided 
//  that this copyright notice appear in all copies and that the name of Brown University not be used in 
//  advertising or publicity pertaining to the use or distribution of the software without specific written 
//  prior permission from Brown University.
//  
//  See license.txt for further information.
//  
//  BROWN UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE WHICH IS 
//  PROVIDED “AS IS”, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
//  FOR ANY PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR ANY 
//  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR FOR ANY DAMAGES WHATSOEVER RESULTING 
//  FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
//  OTHER TORTIOUS ACTION, OR ANY OTHER LEGAL THEORY, ARISING OUT OF OR IN CONNECTION 
//  WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
//  ----------------------------------
//  
///\file SettingsDialog.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/SettingsDialog.h"
#include "ui_SettingsDialog.h"

#include "core/Settings.h"
#include "core/Project.h"
#include "core/Trial.h"
#include "State.h"
#include "processing/ThreadScheduler.h"
#include <QFileDialog>

#include "ErrorDialog.h"

using namespace xma;

SettingsDialog::SettingsDialog(QWidget* parent) :
	QDialog(parent),
	diag(new Ui::SettingsDialog)
{
	diag->setupUi(this);
	initPhase = true;

	diag->scrollArea_Calibration->hide();
	diag->scrollArea_Digitizing->hide();

	diag->lineEditWorkspace->setText(Settings::getInstance()->getQStringSetting("WorkspacePath"));
	diag->checkBox_Workspace->setChecked(Settings::getInstance()->getBoolSetting("CustomWorkspacePath"));

	diag->checkBox_AutoConfirmPendingChanges->setChecked(Settings::getInstance()->getBoolSetting("AutoConfirmPendingChanges"));
	diag->checkBox_ConfirmQuitXMALab->setChecked(Settings::getInstance()->getBoolSetting("ConfirmQuitXMALab"));
	diag->checkBox_AutoCalibAfterReference->setChecked(Settings::getInstance()->getBoolSetting("AutoCalibAfterReference"));
	diag->checkBox_Advanced->setChecked(Settings::getInstance()->getBoolSetting("ShowAdvancedCalibration"));
	diag->checkBox_HideWarningsCalibration->setChecked(Settings::getInstance()->getBoolSetting("HideWarningsDuringCalibration"));
	diag->spinBox_IdentificationThreshold->setValue(Settings::getInstance()->getIntSetting("IdentificationThresholdCalibration"));
	diag->spinBox_OutlierThreshold->setValue(Settings::getInstance()->getIntSetting("OutlierThresholdForCalibration"));

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

SettingsDialog::~SettingsDialog()
{
	delete diag;
}

void SettingsDialog::closeEvent(QCloseEvent* event)
{
	if (diag->lineEditWorkspace->text().isEmpty() && diag->checkBox_Workspace->isChecked())
	{
		ErrorDialog::getInstance()->showErrorDialog("You have to select a custom workspace or disable the custom workspace");
		event->ignore();
	}
}

void SettingsDialog::on_pushButton_General_clicked()
{
	diag->scrollArea_General->show();
	diag->scrollArea_Calibration->hide();
	diag->scrollArea_Digitizing->hide();
}

void SettingsDialog::on_pushButton_Calibration_clicked()
{
	diag->scrollArea_General->hide();
	diag->scrollArea_Calibration->show();
	diag->scrollArea_Digitizing->hide();
}

void SettingsDialog::on_pushButton_Digitizing_clicked()
{
	diag->scrollArea_General->hide();
	diag->scrollArea_Calibration->hide();
	diag->scrollArea_Digitizing->show();
}

void SettingsDialog::on_toolButton_Workspace_clicked()
{
	QString folder = QFileDialog::getExistingDirectory(this, tr("Load Directory "), Settings::getInstance()->getLastUsedDirectory());

	if (!folder.isEmpty())
	{
		diag->lineEditWorkspace->setText(folder);
		Settings::getInstance()->setLastUsedDirectory(folder);
	}
}

void SettingsDialog::on_checkBox_Workspace_stateChanged(int state)
{
	Settings::getInstance()->set("CustomWorkspacePath", diag->checkBox_Workspace->isChecked());
	if (diag->checkBox_Workspace->isChecked())
	{
		diag->toolButton_Workspace->setEnabled(true);
		diag->lineEditWorkspace->setEnabled(true);
	} 
	else
	{
		diag->toolButton_Workspace->setEnabled(false);
		diag->lineEditWorkspace->setEnabled(false);
		diag->lineEditWorkspace->setText("");
	}
}

void SettingsDialog::on_lineEditWorkspace_textChanged(QString text)
{
	Settings::getInstance()->set("WorkspacePath", text);
}

void SettingsDialog::on_checkBox_AutoConfirmPendingChanges_stateChanged(int state)
{
	Settings::getInstance()->set("AutoConfirmPendingChanges", diag->checkBox_AutoConfirmPendingChanges->isChecked());
}

void SettingsDialog::on_checkBox_AutoCalibAfterReference_stateChanged(int state)
{
	Settings::getInstance()->set("AutoCalibAfterReference", diag->checkBox_AutoCalibAfterReference->isChecked());
}

void SettingsDialog::on_checkBox_Advanced_stateChanged(int state)
{
	Settings::getInstance()->set("ShowAdvancedCalibration", diag->checkBox_Advanced->isChecked());
}

void SettingsDialog::on_checkBox_UseCenteredDetailWindow_stateChanged(int state)
{
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
	Settings::getInstance()->set("Show3dPointDetailView", diag->checkBox_Show3DPointDetailWindow->isChecked());
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
		for (unsigned int i = 0; i < Project::getInstance()->getTrials().size(); i++)
		{
			Project::getInstance()->getTrials()[i]->setRequiresRecomputation(true);
			if (State::getInstance()->getActiveTrial() == i && State::getInstance()->getActiveTrial() >= 0 && State::getInstance()->getActiveTrial() < (int) Project::getInstance()->getTrials().size())
				ThreadScheduler::getInstance()->updateTrialData(Project::getInstance()->getTrials()[i]);
		}
	}
}


void SettingsDialog::on_comboBox_DetectionMethodForCalibration_currentIndexChanged(int value)
{
	Settings::getInstance()->set("DetectionMethodForCalibration", diag->comboBox_DetectionMethodForCalibration->currentIndex());
}


void SettingsDialog::on_checkBox_HideWarningsCalibration_stateChanged(int state)
{
	Settings::getInstance()->set("HideWarningsDuringCalibration", diag->checkBox_HideWarningsCalibration->isChecked());
}

void SettingsDialog::on_spinBox_IdentificationThreshold_stateChanged(int value)
{
	Settings::getInstance()->set("IdentificationThresholdCalibration", diag->spinBox_IdentificationThreshold->value());
}

void SettingsDialog::on_spinBox_OutlierThreshold_valueChanged(int value)
{
	Settings::getInstance()->set("OutlierThresholdForCalibration", diag->spinBox_OutlierThreshold->value());
}

