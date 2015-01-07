#ifndef NEWPROJECTDIALOG_H_
#define NEWPROJECTDIALOG_H_

#include <QDialog>

namespace Ui {
	class NewProjectDialog;
}

namespace xma{
	class CameraBox;

	class NewProjectDialog : public QDialog{

		Q_OBJECT

	public:
		explicit NewProjectDialog(QWidget *parent = 0);
		virtual ~NewProjectDialog();

		Ui::NewProjectDialog *diag;
		const std::vector <CameraBox *>& getCameras(){ return cameras; }

		//bool isCubeCalibrationObject();
		//QString getFrameSpecificationsFileName();
		//QString getReferencePointsFileName();
		//int getHorizontalSquares();
		//int getVerticalSquares();
		//int getSizeSquares();

		bool createProject();

	private:
		std::vector <CameraBox *> cameras;
		int nbCams;

		//checks if all inputs are done
		bool isComplete();

		public slots:
		//Cameras
		void on_toolButtonCameraMinus_clicked();
		void on_toolButtonCameraPlus_clicked();

		//Toggle Calibrationobject
		void on_radioButtonCheckerboard_clicked();
		void on_radioButtonCube_clicked();

		//Calibrationcube files
		void on_toolButtonFrameSpecifications_clicked();
		void on_toolButtonReferencePoints_clicked();

		//Footer buttons
		void on_pushButton_OK_clicked();
		void on_pushButton_Cancel_clicked();
	};
}
#endif /* NEWPROJECTDIALOG_H_ */
