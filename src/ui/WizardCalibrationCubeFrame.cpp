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
///\file WizardCalibrationCubeFrame.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/WizardCalibrationCubeFrame.h"
#include "ui_WizardCalibrationCubeFrame.h"
#include "ui/MainWindow.h"
#include "ui/ErrorDialog.h"
#include "ui/ConfirmationDialog.h"
#include "ui/OptimizationDialog.h"

#include "core/Project.h"
#include "core/Camera.h"
#include "core/Trial.h"
#include "core/Settings.h"
#include "core/CalibrationImage.h"
#include "core/CalibrationObject.h"

#include "processing/BlobDetection.h"
#include "processing/CubeCalibration.h"
#include "processing/CheckerboardDetection.h"
#include "processing/Calibration.h"
#include "processing/MultiCameraCalibration.h"

#include <QInputDialog>

#ifdef __APPLE__
	#include <OpenGL/gl.h>
	#include <OpenGL/glu.h>
#else
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#endif

using namespace xma;

WizardCalibrationCubeFrame::WizardCalibrationCubeFrame(QWidget* parent) :
	QFrame(parent),
	frame(new Ui::WizardCalibrationCubeFrame)
{
	frame->setupUi(this);
#ifdef __APPLE__
	frame->pushButton->setMinimumHeight(26);
	frame->pushButtonDeleteFrame->setMinimumHeight(26);
	frame->pushButtonResetCamera->setMinimumHeight(26);
	frame->pushButtonResetFrame->setMinimumHeight(26);
	frame->pushButtonOptimize->setMinimumHeight(26);
#endif

	setDialog();
	frame->comboBoxPoints->setCurrentIndex(2);
	connect(State::getInstance(), SIGNAL(activeCameraChanged(int)), this, SLOT(activeCameraChanged(int)));
	connect(State::getInstance(), SIGNAL(activeFrameCalibrationChanged(int)), this, SLOT(activeFrameCalibrationChanged(int)));
	connect(State::getInstance(), SIGNAL(workspaceChanged(work_state)), this, SLOT(workspaceChanged(work_state)));
	connect(frame->frametreeWidget, SIGNAL(itemClicked(QTreeWidgetItem*, int)), this, SLOT(frametreeWidgetitemClicked(QTreeWidgetItem*, int)));
	frame->framelist->setVisible(false);
#ifndef WIN32
	frame->label_5->setText("Add a correspondance by using COMMAND+click.");
#endif
	checkerboardManual = Settings::getInstance()->getBoolSetting("DisableCheckerboardDetection");
}

void WizardCalibrationCubeFrame::loadCalibrationSettings()
{
	if (Project::getInstance()->getCalibration() != NO_CALIBRATION){
		if (!CalibrationObject::getInstance()->isCheckerboard() && CalibrationObject::getInstance()->getReferenceIDs().size() >=4)
		{
			for (int i = 0; i < 4; i++)
			{
				selectedReferencePointsIdx[i] = CalibrationObject::getInstance()->getReferenceIDs()[i];
			}
			frame->toolButtonReference1->setText(QString::number(selectedReferencePointsIdx[0] + 1) + "  " + CalibrationObject::getInstance()->getReferenceNames()[0]);
			frame->toolButtonReference2->setText(QString::number(selectedReferencePointsIdx[1] + 1) + "  " + CalibrationObject::getInstance()->getReferenceNames()[1]);
			frame->toolButtonReference3->setText(QString::number(selectedReferencePointsIdx[2] + 1) + "  " + CalibrationObject::getInstance()->getReferenceNames()[2]);
			frame->toolButtonReference4->setText(QString::number(selectedReferencePointsIdx[3] + 1) + "  " + CalibrationObject::getInstance()->getReferenceNames()[3]);
			setupManualPoints();
		}
		else
		{
			setupManualPoints();
		}
	}
}

void WizardCalibrationCubeFrame::on_toolButtonReference1_clicked()
{
	bool ok;
	int idx = QInputDialog::getInt(this, tr("Set Custom Idx for Reference 1"),
	                               tr("Index"), selectedReferencePointsIdx[0] + 1, 1, CalibrationObject::getInstance()->getFrameSpecifications().size(), 1, &ok);
	if (ok)
	{
		selectedReferencePointsIdx[0] = idx - 1;
		frame->toolButtonReference1->setText(QString::number(selectedReferencePointsIdx[0] + 1) + "  CUSTOM INDEX");
	}
}

void WizardCalibrationCubeFrame::on_toolButtonReference2_clicked()
{
	bool ok;
	int idx = QInputDialog::getInt(this, tr("Set Custom Idx for Reference 2"),
	                               tr("Index"), selectedReferencePointsIdx[1] + 1, 1, CalibrationObject::getInstance()->getFrameSpecifications().size(), 1, &ok);
	if (ok)
	{
		selectedReferencePointsIdx[1] = idx - 1;
		frame->toolButtonReference2->setText(QString::number(selectedReferencePointsIdx[1] + 1) + "  CUSTOM INDEX");
	}
}

void WizardCalibrationCubeFrame::on_toolButtonReference3_clicked()
{
	bool ok;
	int idx = QInputDialog::getInt(this, tr("Set Custom Idx for Reference 3"),
	                               tr("Index"), selectedReferencePointsIdx[2] + 1, 1, CalibrationObject::getInstance()->getFrameSpecifications().size(), 1, &ok);
	if (ok)
	{
		selectedReferencePointsIdx[2] = idx - 1;
		frame->toolButtonReference3->setText(QString::number(selectedReferencePointsIdx[2] + 1) + "  CUSTOM INDEX");
	}
}

void WizardCalibrationCubeFrame::on_toolButtonReference4_clicked()
{
	bool ok;
	int idx = QInputDialog::getInt(this, tr("Set Custom Idx for Reference 4"),
	                               tr("Index"), selectedReferencePointsIdx[3] + 1, 1, CalibrationObject::getInstance()->getFrameSpecifications().size(), 1, &ok);
	if (ok)
	{
		selectedReferencePointsIdx[3] = idx - 1;
		frame->toolButtonReference4->setText(QString::number(selectedReferencePointsIdx[3] + 1) + "  CUSTOM INDEX");
	}
}

WizardCalibrationCubeFrame::~WizardCalibrationCubeFrame()
{
	delete frame;
}

void WizardCalibrationCubeFrame::activeCameraChanged(int activeCamera)
{
	checkerboardManual = Settings::getInstance()->getBoolSetting("DisableCheckerboardDetection");
	setDialog();
}

void WizardCalibrationCubeFrame::activeFrameCalibrationChanged(int activeFrame)
{
	checkerboardManual = Settings::getInstance()->getBoolSetting("DisableCheckerboardDetection");
	setDialog();
}

void WizardCalibrationCubeFrame::workspaceChanged(work_state workspace)
{
	if (workspace == CALIBRATION)
	{
		checkerboardManual = Settings::getInstance()->getBoolSetting("DisableCheckerboardDetection");
		setDialog();
	}
}

void WizardCalibrationCubeFrame::updateFrameList()
{
	frame->frametreeWidget->clear();
	frame->frametreeWidget->setColumnCount(Project::getInstance()->getCameras().size() + 1);
	frame->frametreeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	frame->frametreeWidget->headerItem()->setText(0, "");

	for (unsigned int i = 0; i < Project::getInstance()->getCameras().size(); i++)
	{
		frame->frametreeWidget->headerItem()->setText(1 + i, "");
		frame->frametreeWidget->header()->setSectionResizeMode(1 + i, QHeaderView::ResizeToContents);
	}
	frame->frametreeWidget->setHeaderHidden(true);

	for (int i = 0; i < Project::getInstance()->getNbImagesCalibration(); i++){
		bool has_calibrated_Frame = false;
		for (auto cam : Project::getInstance()->getCameras())
		{
			has_calibrated_Frame = has_calibrated_Frame | cam->getCalibrationImages()[i]->isCalibrated();
		}
		if (has_calibrated_Frame){
			QTreeWidgetItem* qtreewidgetitem = new QTreeWidgetItem();
			qtreewidgetitem->setText(0, QString::number(i+1));
			QPixmap pix(8, 16);
			pix.fill(QColor::fromRgb(0, 200, 100));
			for (auto cam : Project::getInstance()->getCameras())
			{
				if (cam->getCalibrationImages()[i]->isCalibrated() == 1) {
					QPixmap pix(8, 16);
					pix.fill(QColor::fromRgb(0, 200, 100));
					qtreewidgetitem->setIcon(cam->getID() + 1, pix);
				}
				else if (cam->getCalibrationImages()[i]->isCalibrated() == 2) {
					QPixmap pix(8, 16);
					pix.fill(QColor::fromRgb(0, 100, 200));
					qtreewidgetitem->setIcon(cam->getID() + 1, pix);
				}
			}
			frame->frametreeWidget->addTopLevelItem(qtreewidgetitem);
		}
	}
	auto items = frame->frametreeWidget->findItems(QString::number(State::getInstance()->getActiveFrameCalibration() + 1), Qt::MatchExactly, 0);
	if (!items.empty()){
		frame->frametreeWidget->setItemSelected(items[0], true);
	}
	else
	{
		frame->frametreeWidget->clearSelection();
	}
}

bool WizardCalibrationCubeFrame::checkForPendingChanges()
{
	bool hasPendingChanges = false;
	for (unsigned int j = 0; j < Project::getInstance()->getCameras().size(); j ++)
	{
		if (Project::getInstance()->getCameras()[j]->isRecalibrationRequired())
		{
			hasPendingChanges = true;
		}
	}

	if (hasPendingChanges)
	{
		if (Settings::getInstance()->getBoolSetting("AutoConfirmPendingChanges") || ConfirmationDialog::getInstance()->showConfirmationDialog(
			"You have modified the calibration input and before continuing you first have to recompute the calibration. If you want to recompute click \'yes\', if you want to continue changing calibration parameters click \'cancel\' "
		))
		{
			QEventLoop loop;
			for (unsigned int j = 0; j < Project::getInstance()->getCameras().size(); j ++)
			{
				if (Project::getInstance()->getCameras()[j]->isRecalibrationRequired())
				{
					Calibration* calibration = new Calibration(j, CalibrationObject::getInstance()->isPlanar());
					connect(calibration, SIGNAL(signal_finished()), &loop, SLOT(quit()));
					calibration->start();
				}
			}
			if (Calibration::isRunning())loop.exec();

			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return true;
	}
}

void WizardCalibrationCubeFrame::draw()
{
	for (int i = 0; i < 4; i++)
	{
		glColor3f(1.0, 0.0, 0.0);
		glBegin(GL_LINES);
		if (selectedReferencePoints[i].x > -1 &&
			selectedReferencePoints[i].y > -1)
		{
			glVertex2f(selectedReferencePoints[i].x - 5, selectedReferencePoints[i].y - 5);
			glVertex2f(selectedReferencePoints[i].x + 5, selectedReferencePoints[i].y + 5);
			glVertex2f(selectedReferencePoints[i].x + 5, selectedReferencePoints[i].y - 5);
			glVertex2f(selectedReferencePoints[i].x - 5, selectedReferencePoints[i].y + 5);
		}
		glEnd();
	}
}

void WizardCalibrationCubeFrame::resetReferences()
{
	loadCalibrationSettings();

	frame->labelReference1->setText("");
	frame->labelReference2->setText("");
	frame->labelReference3->setText("");
	frame->labelReference4->setText("");

	frame->checkBoxReference1->setChecked(false);
	frame->checkBoxReference2->setChecked(false);
	frame->checkBoxReference3->setChecked(false);
	frame->checkBoxReference4->setChecked(false);

	frame->radioButtonReference1->setChecked(true);

	for (int i = 0; i < 4; i++)
	{
		selectedReferencePoints[i].x = -1;
		selectedReferencePoints[i].y = -1;
	}
}

void WizardCalibrationCubeFrame::on_comboBoxImage_currentIndexChanged(int idx)
{
	if (idx == 0)
	{
		if (frame->comboBoxPoints->currentIndex() == 4)
		{
			frame->comboBoxPoints->setCurrentIndex(2);
		}
		else if (frame->comboBoxPoints->currentIndex() == 5)
		{
			frame->comboBoxPoints->setCurrentIndex(3);
		}
	}
	else if (idx == 1)
	{
		if (frame->comboBoxPoints->currentIndex() == 2)
		{
			frame->comboBoxPoints->setCurrentIndex(4);
		}
		else if (frame->comboBoxPoints->currentIndex() == 3)
		{
			frame->comboBoxPoints->setCurrentIndex(5);
		}
	}

	State::getInstance()->changeCalibrationVisImage(calibrationVisImage_state(idx));
	MainWindow::getInstance()->redrawGL();
}

void WizardCalibrationCubeFrame::on_comboBoxPoints_currentIndexChanged(int idx)
{
	//distorted
	if (idx == 1 || idx == 2 || idx == 3)
	{
		if (frame->comboBoxImage->currentIndex() == 1) frame->comboBoxImage->setCurrentIndex(0);
	}
	//undistorted
	else if (idx == 4 || idx == 5)
	{
		if (frame->comboBoxImage->currentIndex() == 0) frame->comboBoxImage->setCurrentIndex(1);
	}

	State::getInstance()->changeCalibrationVisPoints(calibrationVisPoints_state(idx));
	MainWindow::getInstance()->redrawGL();
}

void WizardCalibrationCubeFrame::on_comboBoxText_currentIndexChanged(int idx)
{
	State::getInstance()->changeCalibrationVisText(calibrationVisText_state(idx));
	MainWindow::getInstance()->redrawGL();
}

bool WizardCalibrationCubeFrame::manualCalibrationRunning()
{
	return frame->checkBoxManual->isChecked();
}


void WizardCalibrationCubeFrame::addCalibrationReference(double x, double y)
{
	if (frame->checkBoxManual->isChecked())
	{
		for (unsigned int i = 0; i < manualReferencesRadioButton.size(); i++)
		{
			if (manualReferencesRadioButton[i]->isChecked())
			{
				if (State::getInstance()->getCalibrationVisImage() == DISTORTEDCALIBIMAGE)
				{
					Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->setPoint(i, x, y, true);
				}
				else
				{
					Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->setPoint(i, x, y, false);
				}
				Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->setInlier(i, true);
				reloadManualPoints();
				if (i + 1 < (int) manualReferencesRadioButton.size())manualReferencesRadioButton[i + 1]->setChecked(true);
				return;
			}
		}
	}
	else
	{
		if (State::getInstance()->getUndistortion() == UNDISTORTED)
		{
			if (CalibrationObject::getInstance()->isCheckerboard()){
				if (Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->isCalibrated() != 1)
				{
					frame->comboBoxText->setCurrentIndex(1);
					frame->comboBoxPoints->setCurrentIndex(2);
				}
			}

			if (CalibrationObject::getInstance()->isCheckerboard() && !checkerboardManual)
			{
				selectedReferencePoints[0].x = x;
				selectedReferencePoints[0].y = y;
				if (Settings::getInstance()->getBoolSetting("AutoCalibAfterReference")) on_pushButton_clicked();
			}
			else
			{
				if (frame->radioButtonReference1->isChecked())
				{
					selectedReferencePoints[0].x = x;
					selectedReferencePoints[0].y = y;
					frame->checkBoxReference1->setChecked(true);
					frame->labelReference1->setText(QString::number(x, 'f', 2) + " / " + QString::number(y, 'f', 2));
					frame->radioButtonReference2->setChecked(true);
				}
				else if (frame->radioButtonReference2->isChecked())
				{
					selectedReferencePoints[1].x = x;
					selectedReferencePoints[1].y = y;
					frame->checkBoxReference2->setChecked(true);
					frame->labelReference2->setText(QString::number(x, 'f', 2) + " / " + QString::number(y, 'f', 2));
					frame->radioButtonReference3->setChecked(true);
				}
				else if (frame->radioButtonReference3->isChecked())
				{
					selectedReferencePoints[2].x = x;
					selectedReferencePoints[2].y = y;
					frame->checkBoxReference3->setChecked(true);
					frame->labelReference3->setText(QString::number(x, 'f', 2) + " / " + QString::number(y, 'f', 2));
					frame->radioButtonReference4->setChecked(true);
				}
				else if (frame->radioButtonReference4->isChecked())
				{
					selectedReferencePoints[3].x = x;
					selectedReferencePoints[3].y = y;
					frame->checkBoxReference4->setChecked(true);
					frame->labelReference4->setText(QString::number(x, 'f', 2) + " / " + QString::number(y, 'f', 2));
					//run calibration
					if (Settings::getInstance()->getBoolSetting("AutoCalibAfterReference")) on_pushButton_clicked();
				}
			}
		}
	}
}

void WizardCalibrationCubeFrame::setDialog()
{
	if (Project::getInstance()->getCalibration() != INTERNAL)
		return;

	if (State::getInstance()->getUndistortion() == NOTUNDISTORTED)
	{
		frame->label->setText("You first have to perform an undistortion!");
		frame->frameReferences->hide();
		frame->frameModifyCalibration->hide();
		frame->pushButtonRemoveOutlierAutomatically->hide();
		frame->pushButtonResetCamera->hide();
		frame->pushButtonResetFrame->hide();
		frame->pushButton->hide();
		frame->checkBoxManual->hide();
		frame->frameManual->hide();
		frame->framelist->hide();

		return;
	}
	if (frame->checkBoxFrameList->isChecked())
	{
		frame->framelist->show();
		updateFrameList();
	} else
	{
		frame->framelist->hide();
	}

	if (State::getInstance()->getActiveCamera() >= 0 && State::getInstance()->getActiveFrameCalibration() >= 0)
	{
		frame->groupBoxOptimization->show();

		if (Settings::getInstance()->getBoolSetting("ShowAdvancedCalibration"))
		{
			frame->checkBoxDistortion->show();
		}
		else
		{
			frame->checkBoxDistortion->hide();
		}

		
		if (Project::getInstance()->camerasOptimized())
		{
			frame->checkBoxOptimized->setChecked(true);
			frame->checkBoxOptimized->setEnabled(true);
		} 
		else
		{
			frame->checkBoxOptimized->setChecked(false);
			frame->checkBoxOptimized->setEnabled(false);
		}
		frame->checkBoxDistortion->setChecked(Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->hasModelDistortion());

		frame->checkBoxManual->show();
		if (Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->isCalibrated() != 1)
		{
			resetReferences();


			if (frame->checkBoxManual->isChecked())
			{
				frame->frameReferences->hide();
				frame->frameManual->show();
				reloadManualPoints();
			}
			else
			{
				if (!CalibrationObject::getInstance()->isCheckerboard())
				{
					frame->label->setText("Select the references");
					frame->frameReferences->show();
				}
				else
				{
					if (checkerboardManual)
					{
						frame->label->setText("Click on the four interior corners of the checkerboard. Start with the upper left corner of the chessboard");
						frame->frameReferences->show();
					}
					else{
						frame->label->setText("Click on the interior upper left corner of the chessboard");		
						frame->frameReferences->hide();
					}
				}
				frame->frameManual->hide();
			}
			frame->frameModifyCalibration->hide();
			frame->pushButtonRemoveOutlierAutomatically->hide();
			frame->pushButtonResetCamera->hide();
			frame->pushButtonResetFrame->hide();
			frame->pushButton->show();
			frame->pushButton->setText("compute calibration");
		}
		else
		{
			resetReferences();
			if (!frame->checkBoxManual->isChecked())
			{
				frame->frameManual->hide();
			}
			else
			{
				frame->frameManual->show();
				reloadManualPoints();
			}
			frame->label->setText("Modify calibration");
			frame->frameReferences->hide();
			frame->frameModifyCalibration->show();
			frame->pushButtonResetCamera->show();
			frame->pushButtonResetFrame->show();
			frame->pushButtonRemoveOutlierAutomatically->hide();
			frame->pushButton->show();
			frame->pushButton->setText("recompute calibration");
		}
	}
}

void WizardCalibrationCubeFrame::on_pushButton_clicked()
{
	if (frame->checkBoxManual->isChecked())
	{
		int count = 0;
		for (unsigned int i = 0; i < manualReferencesCheckBox.size(); i++)
		{
			if (manualReferencesCheckBox[i]->isChecked())count++;
		}

		if (count < 7)
		{
			ErrorDialog::getInstance()->showErrorDialog("You need to at least select 7 points for a manual calibration");
		}
		else
		{
			Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->setCalibrated(1);
			runCalibrationCameraAllFrames();
		}
	}
	else
	{
		if (Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->isCalibrated() != 1)
		{
			if (!CalibrationObject::getInstance()->isCheckerboard())
			{
				int count = 0;
				for (int i = 0; i < 4; i++)
				{
					if (selectedReferencePoints[i].x >= 0 && selectedReferencePoints[i].y >= 0)count++;
				}
				if (count < 3)
				{
					ErrorDialog::getInstance()->showErrorDialog("You need to at least select 3 reference points");
				}
				else
				{
					BlobDetection* blobdetection = new BlobDetection(State::getInstance()->getActiveCamera(), State::getInstance()->getActiveFrameCalibration());
					connect(blobdetection, SIGNAL(signal_finished()), this, SLOT(runCalibration()));
					blobdetection->start();
				}
			}
			else
			{
				int count = 0;
				for (int i = 0; i < 4; i++)
				{
					if (selectedReferencePoints[i].x >= 0 && selectedReferencePoints[i].y >= 0)count++;
				}
				if (count < 1)
				{
					ErrorDialog::getInstance()->showErrorDialog("You need to select the interior upper left corner of the chessboard");
				}
				else
				{
					CheckerboardDetection* checkerdetection;
					if (!checkerboardManual){
						checkerdetection = new CheckerboardDetection(State::getInstance()->getActiveCamera(), State::getInstance()->getActiveFrameCalibration());			
					} else
					{
						checkerdetection = new CheckerboardDetection(State::getInstance()->getActiveCamera(), State::getInstance()->getActiveFrameCalibration(),selectedReferencePoints);
					}
					connect(checkerdetection, SIGNAL(detectCorner_finished()), this, SLOT(runCalibration()));
					checkerdetection->detectCorner();
				}
			}
		}
		else
		{
			runCalibrationCameraAllFrames();
		}
	}
}

void WizardCalibrationCubeFrame::runCalibrationCameraAllFrames()
{
	std::vector<Calibration *> calibs;
	if (!CalibrationObject::getInstance()->isCheckerboard())
	{
		for (unsigned int j = 0; j < Project::getInstance()->getCameras().size(); j++)
		{
			if (Project::getInstance()->getCameras()[j]->isRecalibrationRequired())
			{
				Calibration* calibration = new Calibration(j, CalibrationObject::getInstance()->isPlanar());

				connect(calibration, SIGNAL(signal_finished()), this, SLOT(runCalibrationCameraAllFramesFinished()));
				calibs.push_back(calibration);
			}
		}
	}
	else
	{
		for (unsigned int j = 0; j < Project::getInstance()->getCameras().size(); j++)
		{
			if (Project::getInstance()->getCameras()[j]->isRecalibrationRequired())
			{
				int count = 0;
				for (int m = 0; m < Project::getInstance()->getNbImagesCalibration(); m++)
				{
					if (Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getCalibrationImages()[m]->getDetectedPointsAll().size() > 0) count++;
				}
				if (count >= 2)
				{
					Calibration* calibration = new Calibration(j, true);
					connect(calibration, SIGNAL(signal_finished()), this, SLOT(runCalibrationCameraAllFramesFinished()));
					calibs.push_back(calibration);
				}
			}
		}
	}
	for (unsigned int i = 0; i < calibs.size(); i++)
	{
		calibs[i]->start();
	}
}

void WizardCalibrationCubeFrame::runCalibrationCameraAllFramesFinished()
{
	setDialog();

	for (unsigned int i = 0; i < Project::getInstance()->getTrials().size(); i++)
	{
		if (!State::getInstance()->isLoading())
		{
			Project::getInstance()->getTrials()[i]->setRequiresRecomputation(true);
		}
	}

	MainWindow::getInstance()->redrawGL();
}

void WizardCalibrationCubeFrame::runCalibration()
{
	if (Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->getDetectedPointsAll().size() < 6)
	{
		if (!CalibrationObject::getInstance()->isCheckerboard())
		{
			ErrorDialog::getInstance()->showErrorDialog("Not enough points found to run calibration");
		}
		else
		{
			ErrorDialog::getInstance()->showErrorDialog("Could not detect chessboard. Please try the manual mode");
			checkerboardManual = true;
			setDialog();
		}
		return;
	}
	if (!CalibrationObject::getInstance()->isCheckerboard())
	{
		CubeCalibration* calibration = new CubeCalibration(State::getInstance()->getActiveCamera(), State::getInstance()->getActiveFrameCalibration(), selectedReferencePoints, selectedReferencePointsIdx);
		if (!Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->isCalibrated())
		{
			connect(calibration, SIGNAL(computePoseAndCam_finished()), this, SLOT(runCalibrationFinished()));
			calibration->computePoseAndCam();
		}
		else
		{
			connect(calibration, SIGNAL(computePose_finished()), this, SLOT(runCalibrationFinished()));
			calibration->computePose();
		}
	}
	else
	{
		Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->sortGridByReference(selectedReferencePoints[0].x, selectedReferencePoints[0].y);
		int count = 0;
		for (int m = 0; m < Project::getInstance()->getNbImagesCalibration(); m++)
		{
			if (Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getCalibrationImages()[m]->getDetectedPointsAll().size() > 0) count++;
		}
		if (count >= 2)
		{
			Calibration* calibration = new Calibration(State::getInstance()->getActiveCamera(), true);
			connect(calibration, SIGNAL(signal_finished()), this, SLOT(runCalibrationCameraAllFramesFinished()));
			calibration->start();
		}
	}
}

void WizardCalibrationCubeFrame::checkForCalibrationError()
{
	QString errorMessage = "There were problems during Calibration with the following frames:\n";
	bool hasError = false;
	for (unsigned int j = 0; j < Project::getInstance()->getCameras().size(); j ++)
	{
		for (unsigned int k = 0; k < Project::getInstance()->getCameras()[j]->getCalibrationImages().size(); k ++)
		{
			if (Project::getInstance()->getCameras()[j]->getCalibrationImages()[k]->isCalibrated() < 0)
			{
				hasError = true;
				errorMessage += "Camera " + QString::number(j + 1) + " Frame " + QString::number(k + 1) + "\n";
				Project::getInstance()->getCameras()[j]->getCalibrationImages()[k]->setCalibrated(0);
			}
		}
	}
	if (hasError)
	{
		errorMessage += "Please try to select the references again. If you keep getting this error please have a look at the 3D view and check if the already computed cameras are correct. If necessary reset the erroneous frames or reset the camera!";
		if (!Settings::getInstance()->getBoolSetting("HideWarningsDuringCalibration"))ErrorDialog::getInstance()->showErrorDialog(errorMessage);
	}
}

void WizardCalibrationCubeFrame::runCalibrationFinished()
{
	checkForCalibrationError();

	bool calibrateOtherFramesFailed = false;
	for (unsigned int p = 0; p < temporaryCamIdx.size(); p++)
	{
		if (Project::getInstance()->getCameras()[temporaryCamIdx[p]]->getCalibrationImages()[temporaryFrameIdx[p]]->isCalibrated() != 1)
		{
			if (!Settings::getInstance()->getBoolSetting("HideWarningsDuringCalibration"))ErrorDialog::getInstance()->showErrorDialog("Could not set Camera " + QString::number(temporaryCamIdx[p] + 1) + " Frame " + QString::number(temporaryFrameIdx[p] + 1) + " from other Frames. Please check your calibration.");
			calibrateOtherFramesFailed = true;
		}
	}
	temporaryTransformationMatrix.clear();
	temporaryCamIdx.clear();
	temporaryFrameIdx.clear();

	if (calibrateOtherFramesFailed || !calibrateOtherFrames())
	{
		setDialog();
		MainWindow::getInstance()->redrawGL();

		runCalibrationCameraAllFrames();
	}
}

bool WizardCalibrationCubeFrame::calibrateOtherFrames()
{
	temporaryTransformationMatrix.clear();
	temporaryCamIdx.clear();
	temporaryFrameIdx.clear();
	std::vector<BlobDetection *> detectors;
	bool isRunning = false;
	std::vector<std::vector<cv::Mat> > CamJToCamKTransformation;
	std::vector<std::vector<bool> > CamJToCamKTransformationSet;

	for (unsigned int j = 0; j < Project::getInstance()->getCameras().size(); j ++)
	{
		std::vector<cv::Mat> jTokTransformations;
		std::vector<bool> jTokTransformationSet;

		for (unsigned int k = 0; k < Project::getInstance()->getCameras().size(); k ++)
		{
			cv::Mat t = cv::Mat::zeros(4, 4,CV_64F);
			bool set = false;
			if (j != k && Project::getInstance()->getCameras()[k]->isCalibrated() && Project::getInstance()->getCameras()[j]->isCalibrated())
			{
				for (int m = 0; m < Project::getInstance()->getNbImagesCalibration(); m++)
				{
					CalibrationImage* fk = Project::getInstance()->getCameras()[k]->getCalibrationImages()[m];
					CalibrationImage* fj = Project::getInstance()->getCameras()[j]->getCalibrationImages()[m];
					if ((fk->isCalibrated() == 1) && (fj->isCalibrated() == 1))
					{
						t = fk->getTransformationMatrix().inv() * fj->getTransformationMatrix();
						set = true;
						break;
					}
				}
			}
			jTokTransformations.push_back(t);
			jTokTransformationSet.push_back(set);
		}
		CamJToCamKTransformation.push_back(jTokTransformations);
		CamJToCamKTransformationSet.push_back(jTokTransformationSet);
	}

	for (unsigned int j = 0; j < Project::getInstance()->getCameras().size(); j ++)
	{
		for (unsigned int k = 0; k < Project::getInstance()->getCameras().size(); k ++)
		{
			if (Project::getInstance()->getCameras()[k]->isCalibrated() && Project::getInstance()->getCameras()[j]->isCalibrated())
			{
				for (int m = 0; m < Project::getInstance()->getNbImagesCalibration(); m++)
				{
					CalibrationImage* fk = Project::getInstance()->getCameras()[k]->getCalibrationImages()[m];
					CalibrationImage* fj = Project::getInstance()->getCameras()[j]->getCalibrationImages()[m];
					if (j != k && (fk->isCalibrated() != 1) && (fj->isCalibrated() == 1))
					{
						bool save = true;
						for (unsigned int p = 0; p < temporaryCamIdx.size(); p++)
						{
							if (temporaryCamIdx[p] == k && temporaryFrameIdx[p] == m)
								save = false;
						}

						if (save && CamJToCamKTransformationSet[k][j])
						{
							if (!CalibrationObject::getInstance()->isCheckerboard())
							{
								temporaryCamIdx.push_back(k);
								temporaryFrameIdx.push_back(m);
								temporaryTransformationMatrix.push_back(fj->getTransformationMatrix() * CamJToCamKTransformation[k][j]);
								BlobDetection* blobdetection = new BlobDetection(k, m);
								connect(blobdetection, SIGNAL(signal_finished()), this, SLOT(setTransformationMatrix()));
								detectors.push_back(blobdetection);
								isRunning = true;
							}
						}
					}
				}
			}
		}
	}
	//clean
	for (unsigned int j = 0; j < CamJToCamKTransformation.size(); j ++)
	{
		for (unsigned int k = 0; k < CamJToCamKTransformation[j].size(); k++)
		{
			CamJToCamKTransformation[j][k].release();
		}
		CamJToCamKTransformation[j].clear();
		CamJToCamKTransformationSet.clear();
	}
	CamJToCamKTransformation.clear();
	CamJToCamKTransformationSet.clear();

	for (unsigned int i = 0; i < detectors.size(); i++)
	{
		detectors[i]->start();
	}
	return isRunning;
}


void WizardCalibrationCubeFrame::reloadManualPoints()
{
	if (CalibrationObject::getInstance()->getFrameSpecifications().size() > Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->getInliers().size())
	{
		Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->init(CalibrationObject::getInstance()->getFrameSpecifications().size());
	}

	for (unsigned int i = 0; i < CalibrationObject::getInstance()->getFrameSpecifications().size(); i++)
	{
		manualReferencesCheckBox[i]->setChecked(
			Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->getInliers()[i] > 0
		);
		manualReferencesLabel[i]->setText(
			QString::number(Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->getDetectedPoints()[i].x) + " / " +
			QString::number(Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->getDetectedPoints()[i].y)
		);
	}
}

void WizardCalibrationCubeFrame::setupManualPoints()
{
	for (unsigned int i = 0; i < manualReferencesCheckBox.size(); i++)
	{
		frame->gridLayout_10->removeWidget(manualReferencesCheckBox[i]);
		delete manualReferencesCheckBox[i];
		frame->gridLayout_10->removeWidget(manualReferencesLabel[i]);
		delete manualReferencesLabel[i];
		frame->gridLayout_10->removeWidget(manualReferencesRadioButton[i]);
		delete manualReferencesRadioButton[i];
	}
	manualReferencesCheckBox.clear();
	manualReferencesLabel.clear();
	manualReferencesRadioButton.clear();

	for (unsigned int i = 0; i < CalibrationObject::getInstance()->getFrameSpecifications().size(); i++)
	{
		manualReferencesCheckBox.push_back(new QCheckBox(this));
		manualReferencesLabel.push_back(new QLabel("", this));
		manualReferencesRadioButton.push_back(new QRadioButton(QString::number(i + 1), this));

		frame->gridLayout_10->addWidget(manualReferencesRadioButton[i], i, 0, 1, 1);
		frame->gridLayout_10->addWidget(manualReferencesLabel[i], i, 1, 1, 1);
		frame->gridLayout_10->addWidget(manualReferencesCheckBox[i], i, 2, 1, 1);
		connect(manualReferencesCheckBox[i], SIGNAL(clicked()), this, SLOT(checkBoxManualReference_clicked()));
	}
	if (CalibrationObject::getInstance()->getFrameSpecifications().size() > 0)
		manualReferencesRadioButton[0]->setChecked(true);
}

void WizardCalibrationCubeFrame::checkBoxManualReference_clicked()
{
	for (unsigned int i = 0; i < manualReferencesCheckBox.size(); i++)
	{
		Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->setInlier(
			i, manualReferencesCheckBox[i]->isChecked());
	}
	MainWindow::getInstance()->redrawGL();
}

void WizardCalibrationCubeFrame::on_pushButtonOptimize_clicked()
{
	if (Settings::getInstance()->getBoolSetting("ShowAdvancedCalibration")){
		OptimizationDialog* optdiag = new OptimizationDialog(this);
		optdiag->exec();

		if (optdiag->result())
		{
			MultiCameraCalibration* multi = new MultiCameraCalibration(optdiag->getMethod(), optdiag->getIterations(), optdiag->getInitial());
			connect(multi, SIGNAL(optimizeCameraSetup_finished()), this, SLOT(optimizationDone()));
			multi->optimizeCameraSetup();
		}
		delete optdiag;
	} 
	else
	{
		bool ok;
		int nbIterations = QInputDialog::getInt(this, tr("Set number of iterations"),
			tr("Iterations:"), 10000, 1, 99999999999, 1, &ok);
		if (ok)
		{
			MultiCameraCalibration* multi = new MultiCameraCalibration(0, nbIterations, 0.01);
			connect(multi, SIGNAL(optimizeCameraSetup_finished()), this, SLOT(optimizationDone()));
			multi->optimizeCameraSetup();
		}
		
	}
}

void WizardCalibrationCubeFrame::optimizationDone()
{
	setDialog();
	MainWindow::getInstance()->redrawGL();
}

void WizardCalibrationCubeFrame::on_checkBoxDistortion_clicked()
{
	if (frame->checkBoxDistortion->isChecked())
	{
		cv::Mat dist = Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getDistortionCoefficiants().clone();
		Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->setDistortionCoefficiants(dist);
	}
	else
	{
		Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->resetDistortion();
	}

	Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->setRecalibrationRequired(1);
	Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->setUpdateInfoRequired(true);
	MainWindow::getInstance()->redrawGL();
}

void WizardCalibrationCubeFrame::on_checkBoxManual_clicked()
{
	reloadManualPoints();
	setDialog();
}

void WizardCalibrationCubeFrame::on_checkBoxFrameList_clicked()
{
	frame->framelist->setVisible(frame->checkBoxFrameList->isChecked());
	if (frame->checkBoxFrameList->isChecked()) 
		updateFrameList();
}

void WizardCalibrationCubeFrame::frametreeWidgetitemClicked(QTreeWidgetItem* item, int column)
{
	int frame = item->text(0).toInt() - 1;
	State::getInstance()->changeActiveFrameCalibration(frame);
}

void WizardCalibrationCubeFrame::setTransformationMatrix()
{
	for (unsigned int p = 0; p < temporaryCamIdx.size(); p++)
	{
		CubeCalibration* calibration = new CubeCalibration(temporaryCamIdx[p], temporaryFrameIdx[p], selectedReferencePoints, selectedReferencePointsIdx);
		connect(calibration, SIGNAL(computePose_finished()), this, SLOT(runCalibrationFinished()));
		calibration->setPose(temporaryTransformationMatrix[p]);
	}
}

void WizardCalibrationCubeFrame::on_pushButtonDeleteFrame_clicked()
{
	if (Project::getInstance()->getNbImagesCalibration() > 1 
		&& ConfirmationDialog::getInstance()->showConfirmationDialog("Are you sure you want to delete frame " + QString::number(State::getInstance()->getActiveFrameCalibration()) + "  for all cameras?"))
	{
		for (auto cam : Project::getInstance()->getCameras())
		{
			cam->deleteFrame(State::getInstance()->getActiveFrameCalibration());
			
		}
		MainWindow::getInstance()->recountFrames();
		int newFrame = State::getInstance()->getActiveFrameCalibration() - 1;
		if (newFrame < 0) newFrame = 0;
		State::getInstance()->changeActiveFrameCalibration(newFrame);

		for (auto cam : Project::getInstance()->getCameras())
		{
			int countCalibrated = 0;
			for (auto it : cam->getCalibrationImages())
			{
				if (it->isCalibrated() == 1)
					countCalibrated++;
			}
			if (countCalibrated == 0)
			{
				cam->reset();
			}
			else if (countCalibrated > 0)
			{
				cam->setRecalibrationRequired(1);
			}
		}

		setDialog();
		MainWindow::getInstance()->redrawGL();
		runCalibrationCameraAllFrames();
	}
}

void WizardCalibrationCubeFrame::on_pushButtonResetCamera_clicked()
{
	if (ConfirmationDialog::getInstance()->showConfirmationDialog("Are you sure you want to reset " + Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getName() + " and all detected frames"))
	{
		Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->reset();
		setDialog();
		MainWindow::getInstance()->redrawGL();
	}
}

void WizardCalibrationCubeFrame::on_pushButtonResetFrame_clicked()
{
	if (ConfirmationDialog::getInstance()->showConfirmationDialog("Are you sure you want to reset Frame " + QString::number(State::getInstance()->getActiveFrameCalibration() + 1) + " for " + Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getName()))
	{
		int countCalibrated = 0;
		for (auto im : Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getCalibrationImages())
		{
			if (im->isCalibrated() == 1)
				countCalibrated++;
		}

		if (countCalibrated == 1)
		{
			Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->reset();
			setDialog();
			MainWindow::getInstance()->redrawGL();
		}
		else if (countCalibrated > 1)
		{
			Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->reset();
			Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->getCalibrationImages()[State::getInstance()->getActiveFrameCalibration()]->init(CalibrationObject::getInstance()->getFrameSpecifications().size());
			Project::getInstance()->getCameras()[State::getInstance()->getActiveCamera()]->setRecalibrationRequired(1);
			runCalibrationCameraAllFrames();
		}
	}
}

void WizardCalibrationCubeFrame::on_checkBoxOptimized_clicked()
{
	if (ConfirmationDialog::getInstance()->showConfirmationDialog("Are you sure you want to remove the optimization?"))
	{
		for (auto cam : Project::getInstance()->getCameras())
		{
			cam->setRecalibrationRequired(1);
		}
		on_pushButton_clicked();
	} 
	else
	{
		frame->checkBoxOptimized->setChecked(true);
	}
}
