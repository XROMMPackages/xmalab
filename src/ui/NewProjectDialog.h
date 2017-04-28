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
///\file NewProjectDialog.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef NEWPROJECTDIALOG_H_
#define NEWPROJECTDIALOG_H_

#include <QDialog>

namespace Ui
{
	class NewProjectDialog;
}

namespace xma
{
	class CameraBox;

	class NewProjectDialog : public QDialog
	{
		Q_OBJECT

	public:
		explicit NewProjectDialog(QWidget* parent = 0);
		virtual ~NewProjectDialog();

		Ui::NewProjectDialog* diag;

		const std::vector<CameraBox *>& getCameras()
		{
			return cameras;
		}

		void addCalibrationImage(int id_camera, QString filename);
		void addGridImage(int id_camera, QString filename);
		void setCalibrationCubeCSV(QString filename);
		void setCalibrationCubeREF(QString filename);

		//bool isCubeCalibrationObject();
		//QString getFrameSpecificationsFileName();
		//QString getReferencePointsFileName();
		//int getHorizontalSquares();
		//int getVerticalSquares();
		//int getSizeSquares();

		int createProject();

	private:
		std::vector<CameraBox *> cameras;
		int nbCams;

		//checks if all inputs are done
		bool isComplete();
		bool referencesValid();
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

