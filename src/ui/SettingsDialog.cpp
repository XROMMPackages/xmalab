//  ----------------------------------
//  XMALab -- Copyright � 2015, Brown University, Providence, RI.
//  
//  All Rights Reserved
//   
//  Use of the XMALab software is provided under the terms of the GNU General Public License version 3 
//  as published by the Free Software Foundation at http://www.gnu.org/licenses/gpl-3.0.html, provided 
//  that this copyright notice appear in all copies and that the name of Brown University not be used in 
//  advertising or publicity pertaining to the use or distribution of the software without specific written 
//  prior permission from Brown University.
//  
//  See license.txt for further information.
//  
//  BROWN UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE WHICH IS 
//  PROVIDED �AS IS�, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
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

#include "ui/StatusColorDialog.h"
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
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
	initPhase = true;

	diag->scrollArea_Calibration->hide();
	diag->scrollArea_Digitizing->hide();

	diag->lineEditWorkspace->setText(Settings::getInstance()->getQStringSetting("WorkspacePath"));
	diag->checkBox_Workspace->setChecked(Settings::getInstance()->getBoolSetting("CustomWorkspacePath"));
	diag->spinBoxFrameAdvance->setValue(Settings::getInstance()->getIntSetting("FrameAdvance"));
	diag->checkBox_exportAll->setChecked(Settings::getInstance()->getBoolSetting("ExportAllEnabled"));
	diag->checkBox_DisableImageSearch->setChecked(Settings::getInstance()->getBoolSetting("DisableImageSearch"));
	diag->checkBox_recomputeWhenSaving->setChecked(Settings::getInstance()->getBoolSetting("RecomputeWhenSaving"));
	
	diag->checkBox_AutoConfirmPendingChanges->setChecked(Settings::getInstance()->getBoolSetting("AutoConfirmPendingChanges"));
	diag->checkBox_ConfirmQuitXMALab->setChecked(Settings::getInstance()->getBoolSetting("ConfirmQuitXMALab"));
	diag->checkBox_AutoCalibAfterReference->setChecked(Settings::getInstance()->getBoolSetting("AutoCalibAfterReference"));
	diag->checkBox_Advanced->setChecked(Settings::getInstance()->getBoolSetting("ShowAdvancedCalibration"));
	diag->checkBox_HideWarningsCalibration->setChecked(Settings::getInstance()->getBoolSetting("HideWarningsDuringCalibration"));
	diag->spinBox_IdentificationThreshold->setValue(Settings::getInstance()->getIntSetting("IdentificationThresholdCalibration"));
	diag->spinBox_OutlierThreshold->setValue(Settings::getInstance()->getIntSetting("OutlierThresholdForCalibration"));
	diag->checkBox_DisableCheckerboardDetection->setChecked(Settings::getInstance()->getBoolSetting("DisableCheckerboardDetection"));
	diag->checkBox_DisableCheckerboardRefinement->setChecked(Settings::getInstance()->getBoolSetting("DisableCheckerboardRefinement"));
	diag->checkBox_FixPrincipal->setChecked(Settings::getInstance()->getBoolSetting("FixPrincipal"));

	diag->checkBox_UseCenteredDetailWindow->setChecked(Settings::getInstance()->getBoolSetting("CenterDetailView"));
	diag->checkBox_ShowAdvancedCrosshairDetailWindow->setChecked(Settings::getInstance()->getBoolSetting("AdvancedCrosshairDetailView"));
	diag->checkBox_DrawProjected2DpositionsForAllPoints->setChecked(Settings::getInstance()->getBoolSetting("DrawProjected2DpositionsForAllPoints"));
	diag->checkBox_Show3DPointDetailWindow->setChecked(Settings::getInstance()->getBoolSetting("Show3dPointDetailView"));
	diag->checkBox_ShowEpiLineDetailWindow->setChecked(Settings::getInstance()->getBoolSetting("ShowEpiLineDetailView"));
	diag->checkBox_ShowIDsInDetail->setChecked(Settings::getInstance()->getBoolSetting("ShowIDsInDetail"));

	diag->spinBoxEpiPrecision->setValue(Settings::getInstance()->getIntSetting("EpipolarLinePrecision"));
	diag->comboBox_TriangulationMethod->setCurrentIndex(Settings::getInstance()->getIntSetting("TriangulationMethod"));
	diag->comboBox_DetectionMethodForCalibration->setCurrentIndex(Settings::getInstance()->getIntSetting("DetectionMethodForCalibration"));
	diag->doubleSpinBox_MaxError->setValue(Settings::getInstance()->getFloatSetting("MaximumReprojectionError"));
	diag->checkBox_RetrackOptimizedTrackedPoints->setChecked(Settings::getInstance()->getBoolSetting("RetrackOptimizedTrackedPoints"));
	diag->checkBox_TrackInterpolated->setChecked(Settings::getInstance()->getBoolSetting("TrackInterpolatedPoints"));
	diag->checkBox_ShowColoredMarkerCross->setChecked(Settings::getInstance()->getBoolSetting("ShowColoredMarkerCross"));
	diag->checkBox_ShowColoredMarkerIDs->setChecked(Settings::getInstance()->getBoolSetting("ShowColoredMarkerIDs"));
	diag->checkBox_optimize2D->setChecked(Settings::getInstance()->getBoolSetting("OptimizeRigidBody"));
	diag->checkBox_DisableRBComputeAdvanced->setChecked(Settings::getInstance()->getBoolSetting("DisableRBComputeAdvanced"));
	diag->checkBox_Filter3DPoints->setChecked(Settings::getInstance()->getBoolSetting("Filter3DPoints"));
	diag->spinBox_DefaultMarkerThreshold->setValue(Settings::getInstance()->getIntSetting("DefaultMarkerThreshold"));

	switch (Settings::getInstance()->getIntSetting("CheckerboadInvertedAxis"))
	{
	case 1:
		diag->radioButton_CheckerboardXInvert->setChecked(true);
		break;
	case 2:
		diag->radioButton_CheckerboardYInvert->setChecked(true);
		break;
	default:
		diag->radioButton_CheckerboardNoInvert->setChecked(true);
		break;

	}
	diag->pushButton_General->click();
	diag->pushButton_General->setFocus();
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
	diag->pushButton_Calibration->setChecked(false);
	diag->pushButton_Digitizing->setChecked(false);
}

void SettingsDialog::on_pushButton_Calibration_clicked()
{
	diag->scrollArea_General->hide();
	diag->scrollArea_Calibration->show();
	diag->scrollArea_Digitizing->hide();
	diag->pushButton_General->setChecked(false);
	diag->pushButton_Digitizing->setChecked(false);
}

void SettingsDialog::on_pushButton_Digitizing_clicked()
{
	diag->scrollArea_General->hide();
	diag->scrollArea_Calibration->hide();
	diag->scrollArea_Digitizing->show();
	diag->pushButton_Calibration->setChecked(false);
	diag->pushButton_General->setChecked(false);
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

void SettingsDialog::on_spinBoxFrameAdvance_valueChanged(int value)
{
	Settings::getInstance()->set("FrameAdvance", diag->spinBoxFrameAdvance->value());
}

void SettingsDialog::on_checkBox_exportAll_clicked(bool checked)
{
	Settings::getInstance()->set("ExportAllEnabled", diag->checkBox_exportAll->isChecked());
}

void SettingsDialog::on_checkBox_DisableImageSearch_clicked(bool checked)
{
	Settings::getInstance()->set("DisableImageSearch", diag->checkBox_DisableImageSearch->isChecked());
}

void SettingsDialog::on_checkBox_recomputeWhenSaving_clicked(bool checked)
{
	Settings::getInstance()->set("RecomputeWhenSaving", diag->checkBox_recomputeWhenSaving->isChecked());
}

void SettingsDialog::on_checkBox_DisableCheckerboardRefinement_stateChanged(int state)
{
	Settings::getInstance()->set("DisableCheckerboardRefinement", diag->checkBox_DisableCheckerboardRefinement->isChecked());
}

void SettingsDialog::on_checkBox_DisableCheckerboardDetection_stateChanged(int state)
{
	Settings::getInstance()->set("DisableCheckerboardDetection", diag->checkBox_DisableCheckerboardDetection->isChecked());
}

void SettingsDialog::on_radioButton_CheckerboardNoInvert_clicked(bool checked)
{
	if (checked)
		Settings::getInstance()->set("CheckerboadInvertedAxis", 0);
}

void SettingsDialog::on_radioButton_CheckerboardXInvert_clicked(bool checked)
{
	if (checked)
		Settings::getInstance()->set("CheckerboadInvertedAxis", 1);
}

void SettingsDialog::on_radioButton_CheckerboardYInvert_clicked(bool checked)
{
	if (checked)
		Settings::getInstance()->set("CheckerboadInvertedAxis", 2);
}

void SettingsDialog::on_checkBox_FixPrincipal_clicked(bool checked)
{
	Settings::getInstance()->set("FixPrincipal", diag->checkBox_FixPrincipal->isChecked());
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

void SettingsDialog::on_checkBox_DrawProjected2DpositionsForAllPoints_stateChanged(int state)
{
	Settings::getInstance()->set("DrawProjected2DpositionsForAllPoints", diag->checkBox_DrawProjected2DpositionsForAllPoints->isChecked());
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

void SettingsDialog::on_checkBox_ShowColoredMarkerCross_stateChanged(int state)
{
	Settings::getInstance()->set("ShowColoredMarkerCross", diag->checkBox_ShowColoredMarkerCross->isChecked());
}

void SettingsDialog::on_checkBox_ShowColoredMarkerIDs_stateChanged(int state)
{
	Settings::getInstance()->set("ShowColoredMarkerIDs", diag->checkBox_ShowColoredMarkerIDs->isChecked());
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

void SettingsDialog::on_spinBox_IdentificationThreshold_valueChanged(int value)
{
	Settings::getInstance()->set("IdentificationThresholdCalibration", diag->spinBox_IdentificationThreshold->value());
}

void SettingsDialog::on_checkBox_RetrackOptimizedTrackedPoints_stateChanged(int state)
{
	Settings::getInstance()->set("RetrackOptimizedTrackedPoints", diag->checkBox_RetrackOptimizedTrackedPoints->isChecked());
}

void SettingsDialog::on_checkBox_TrackInterpolated_stateChanged(int state)
{
	Settings::getInstance()->set("TrackInterpolatedPoints", diag->checkBox_TrackInterpolated->isChecked());
}

void SettingsDialog::on_pushButton_MarkerStatus_clicked()
{
	this->close();

	StatusColorDialog * diag = new StatusColorDialog(this->parentWidget());
	diag->exec();
    delete diag;
}

void SettingsDialog::on_checkBox_optimize2D_clicked()
{
	Settings::getInstance()->set("OptimizeRigidBody", diag->checkBox_optimize2D->isChecked());
}

void SettingsDialog::on_checkBox_DisableRBComputeAdvanced_clicked()
{
	Settings::getInstance()->set("DisableRBComputeAdvanced", diag->checkBox_DisableRBComputeAdvanced->isChecked());
}

void SettingsDialog::on_checkBox_Filter3DPoints_clicked()
{
	Settings::getInstance()->set("Filter3DPoints", diag->checkBox_Filter3DPoints->isChecked());
}

void SettingsDialog::on_spinBox_DefaultMarkerThreshold_valueChanged(int value)
{
	Settings::getInstance()->set("DefaultMarkerThreshold", diag->spinBox_DefaultMarkerThreshold->value());
}

void SettingsDialog::on_checkBox_ShowIDsInDetail_clicked()
{
	Settings::getInstance()->set("ShowIDsInDetail", diag->checkBox_ShowIDsInDetail->isChecked());
}

void SettingsDialog::on_spinBox_OutlierThreshold_valueChanged(int value)
{
	Settings::getInstance()->set("OutlierThresholdForCalibration", diag->spinBox_OutlierThreshold->value());
}

