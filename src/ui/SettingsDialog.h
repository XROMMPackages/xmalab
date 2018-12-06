//  ----------------------------------
//  XMALab -- Copyright © 2015, Brown University, Providence, RI.
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
//  PROVIDED “AS IS”, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
//  FOR ANY PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR ANY 
//  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR FOR ANY DAMAGES WHATSOEVER RESULTING 
//  FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
//  OTHER TORTIOUS ACTION, OR ANY OTHER LEGAL THEORY, ARISING OUT OF OR IN CONNECTION 
//  WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
//  ----------------------------------
//  
///\file SettingsDialog.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QCloseEvent>

namespace Ui
{
	class SettingsDialog;
}

namespace xma
{
	class SettingsDialog : public QDialog
	{
		Q_OBJECT

	public:
		explicit SettingsDialog(QWidget* parent = 0);
		virtual ~SettingsDialog();
		Ui::SettingsDialog* diag;

	protected:
		void closeEvent(QCloseEvent* event);

	private:
		bool initPhase;

	public slots :
		void on_pushButton_General_clicked();
		void on_pushButton_Calibration_clicked();
		void on_pushButton_Digitizing_clicked();
		
		void on_toolButton_Workspace_clicked();
		void on_checkBox_Workspace_stateChanged(int state);
		void on_lineEditWorkspace_textChanged(QString text);
		void on_spinBoxFrameAdvance_valueChanged(int value);
		void on_checkBox_exportAll_clicked(bool checked);

		void on_checkBox_DisableCheckerboardRefinement_stateChanged(int state);
		void on_checkBox_DisableCheckerboardDetection_stateChanged(int state);
		void on_radioButton_CheckerboardNoInvert_clicked(bool checked);
		void on_radioButton_CheckerboardXInvert_clicked(bool checked);
		void on_radioButton_CheckerboardYInvert_clicked(bool checked);

		void on_checkBox_AutoConfirmPendingChanges_stateChanged(int state);
		void on_checkBox_AutoCalibAfterReference_stateChanged(int state);
		void on_checkBox_Advanced_stateChanged(int state);
		void on_checkBox_HideWarningsCalibration_stateChanged(int state);
		void on_checkBox_UseCenteredDetailWindow_stateChanged(int state);
		void on_checkBox_ShowAdvancedCrosshairDetailWindow_stateChanged(int state);
		void on_checkBox_Show3DPointDetailWindow_stateChanged(int state);
		void on_checkBox_ShowEpiLineDetailWindow_stateChanged(int state);
		void on_checkBox_ShowColoredMarkerCross_stateChanged(int state);
		void on_checkBox_ShowColoredMarkerIDs_stateChanged(int state);


		void on_spinBoxEpiPrecision_valueChanged(int value);
		void on_comboBox_TriangulationMethod_currentIndexChanged(int value);
		void on_comboBox_DetectionMethodForCalibration_currentIndexChanged(int value);
		void on_checkBox_ConfirmQuitXMALab_stateChanged(int state);
		void on_doubleSpinBox_MaxError_valueChanged(double value);
		void on_spinBox_IdentificationThreshold_valueChanged(int value);
		void on_spinBox_OutlierThreshold_valueChanged(int value);
		void on_checkBox_RetrackOptimizedTrackedPoints_stateChanged(int state);
		void on_checkBox_TrackInterpolated_stateChanged(int state);
		void on_pushButton_MarkerStatus_clicked();
		void on_checkBox_optimize2D_clicked();
		void on_checkBox_DisableRBComputeAdvanced_clicked();
		void on_spinBox_DefaultMarkerThreshold_valueChanged(int value);
		void on_checkBox_ShowIDsInDetail_clicked();
	};
}
#endif // SETTINGSDIALOG_H


