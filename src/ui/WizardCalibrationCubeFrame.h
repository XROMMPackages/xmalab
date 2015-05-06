#ifndef WIZARDCALIBRATIONCUBEFRAME_H_
#define WIZARDCALIBRATIONCUBEFRAME_H_

#include <QFrame>

#include "ui/State.h"

#include <opencv/cv.h>

namespace Ui {
	class WizardCalibrationCubeFrame;
}

class QLabel;
class QCheckBox;
class QRadioButton;

namespace xma{
	class WizardCalibrationCubeFrame : public QFrame{

		Q_OBJECT

	public:
		virtual ~WizardCalibrationCubeFrame();
		WizardCalibrationCubeFrame(QWidget *parent = 0);

		void loadCalibrationSettings();
		bool manualCalibrationRunning();
		void addCalibrationReference(double x, double y);
		void draw();

		void runCalibrationCameraAllFrames();
		bool checkForPendingChanges();

	private:
		Ui::WizardCalibrationCubeFrame *frame;
		bool planarCalibrationObject;
		void setDialog();
		void reloadManualPoints();
		void setupManualPoints();

		void resetReferences();
		void calibrateOtherFrames();
		void checkForCalibrationError();

		cv::Point2d selectedReferencePoints[4];
		int selectedReferencePointsIdx[4];

		std::vector <cv::Mat> temporaryTransformationMatrix;
		std::vector <int> temporaryCamIdx;
		std::vector <int> temporaryFrameIdx;

		std::vector <QCheckBox * > manualReferencesCheckBox;
		std::vector <QLabel * > manualReferencesLabel;
		std::vector <QRadioButton * > manualReferencesRadioButton;

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
	};
}


#endif /* WIZARDUNDISTORTIONFRAME_H_ */
