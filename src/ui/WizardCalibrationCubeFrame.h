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
///\file WizardCalibrationCubeFrame.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef WIZARDCALIBRATIONCUBEFRAME_H_
#define WIZARDCALIBRATIONCUBEFRAME_H_

#include <QFrame>

#include "ui/State.h"

#include <opencv/cv.h>

namespace Ui
{
	class WizardCalibrationCubeFrame;
}

class QLabel;
class QCheckBox;
class QRadioButton;

namespace xma
{
	class WizardCalibrationCubeFrame : public QFrame
	{
		Q_OBJECT

	public:
		virtual ~WizardCalibrationCubeFrame();
		WizardCalibrationCubeFrame(QWidget* parent = 0);

		void loadCalibrationSettings();
		bool manualCalibrationRunning();
		void addCalibrationReference(double x, double y);
		void draw();

		void runCalibrationCameraAllFrames();
		bool checkForPendingChanges();

	private:
		Ui::WizardCalibrationCubeFrame* frame;
		void setDialog();
		void reloadManualPoints();
		void setupManualPoints();

		void resetReferences();
		bool calibrateOtherFrames();
		void checkForCalibrationError();

		cv::Point2d selectedReferencePoints[4];
		int selectedReferencePointsIdx[4];

		std::vector<cv::Mat> temporaryTransformationMatrix;
		std::vector<int> temporaryCamIdx;
		std::vector<int> temporaryFrameIdx;

		std::vector<QCheckBox *> manualReferencesCheckBox;
		std::vector<QLabel *> manualReferencesLabel;
		std::vector<QRadioButton *> manualReferencesRadioButton;

		bool checkerboardManual;

	public slots:
		void activeCameraChanged(int activeCamera);
		void activeFrameCalibrationChanged(int activeFrame);
		void workspaceChanged(work_state workspace);

		void on_pushButton_clicked();
		void on_comboBoxImage_currentIndexChanged(int idx);
		void on_comboBoxPoints_currentIndexChanged(int idx);
		void on_comboBoxText_currentIndexChanged(int idx);

		void on_toolButtonReference1_clicked();
		void on_toolButtonReference2_clicked();
		void on_toolButtonReference3_clicked();
		void on_toolButtonReference4_clicked();

		void on_pushButtonDeleteFrame_clicked();
		void on_pushButtonResetCamera_clicked();
		void on_pushButtonResetFrame_clicked();

		void runCalibration();
		void runCalibrationFinished();
		void runCalibrationCameraAllFramesFinished();
		void setTransformationMatrix();

		void on_checkBoxManual_clicked();
		void checkBoxManualReference_clicked();
		void on_pushButtonOptimize_clicked();
		void on_OptimizationDone_clicked();
		void on_checkBoxDistortion_clicked();
	};
}


#endif /* WIZARDUNDISTORTIONFRAME_H_ */

