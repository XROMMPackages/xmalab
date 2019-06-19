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
///\file WorkspaceNavigationFrame.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/WorkspaceNavigationFrame.h"
#include "ui_WorkspaceNavigationFrame.h"
#include "ui/MainWindow.h"
#include "ui/WizardDockWidget.h"
#include "ui/TrialDialog.h"

#include "core/Project.h"
#include "core/Camera.h"
#include "core/Trial.h"
#include "core/CalibrationImage.h"

#include "processing/ThreadScheduler.h"
#include "ui/CameraSelector.h"
#include "ui/DetailViewDockWidget.h"

using namespace xma;

WorkspaceNavigationFrame* WorkspaceNavigationFrame::instance = NULL;

WorkspaceNavigationFrame::WorkspaceNavigationFrame(QWidget* parent) :
	QFrame(parent),
	frame(new Ui::WorkspaceNavigationFrame)
{
	frame->setupUi(this);
	setTrialVisible(false);
	updating = false;

	connect(State::getInstance(), SIGNAL(workspaceChanged(work_state)), this, SLOT(workspaceChanged(work_state)));
	connect(State::getInstance(), SIGNAL(displayChanged(ui_state)), this, SLOT(displayChanged(ui_state)));
	connect(State::getInstance(), SIGNAL(activeTrialChanged(int)), this, SLOT(activeTrialChanged(int)));
}

WorkspaceNavigationFrame::~WorkspaceNavigationFrame()
{
	delete frame;
	instance = NULL;
}

WorkspaceNavigationFrame* WorkspaceNavigationFrame::getInstance()
{
	if (!instance)

	{
		instance = new WorkspaceNavigationFrame(MainWindow::getInstance());
	}
	return instance;
}

void WorkspaceNavigationFrame::setUndistortionCalibration(bool hasUndistortion, bool hasCalibration)
{
	if (!hasCalibration)
	{
		frame->comboBoxWorkspace->setCurrentIndex(frame->comboBoxWorkspace->findText("Marker Tracking"));
		on_comboBoxWorkspace_currentIndexChanged(frame->comboBoxWorkspace->currentText());
		frame->horizontalSpacer->changeSize(10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum);
		frame->comboBoxWorkspace->setVisible(false);
		frame->label->setVisible(false);
	}
	else if (hasUndistortion)
	{
		frame->comboBoxWorkspace->setCurrentIndex(frame->comboBoxWorkspace->findText("Undistortion"));
		on_comboBoxWorkspace_currentIndexChanged(frame->comboBoxWorkspace->currentText());
		frame->horizontalSpacer->changeSize(10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum);
		frame->comboBoxWorkspace->setVisible(true);
		frame->label->setVisible(true);
	}
	else
	{
		frame->comboBoxWorkspace->setCurrentIndex(frame->comboBoxWorkspace->findText("Calibration"));
		on_comboBoxWorkspace_currentIndexChanged(frame->comboBoxWorkspace->currentText());
		frame->horizontalSpacer->changeSize(10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum);
		frame->comboBoxWorkspace->setVisible(true);
		frame->comboBoxWorkspace->removeItem(frame->comboBoxWorkspace->findText("Undistortion"));
		frame->label->setVisible(true);
	}
}


void WorkspaceNavigationFrame::addTrial(QString name)
{
	frame->comboBoxTrial->addItem(name);
}

void WorkspaceNavigationFrame::removeTrial(QString name)
{
	int idx = frame->comboBoxTrial->findText(name);
	frame->comboBoxTrial->removeItem(idx);

}

void WorkspaceNavigationFrame::closeProject()
{
	updating = true;
	frame->comboBoxTrial->clear();
	setTrialVisible(false);
	updating = false;
}

void WorkspaceNavigationFrame::workspaceChanged(work_state workspace)
{
	if (workspace == UNDISTORTION)
	{
		frame->comboBoxViewspace->setEnabled(true);
		frame->toolButtonCameraSettings->setEnabled(true);
		frame->comboBoxWorkspace->setCurrentIndex(frame->comboBoxWorkspace->findText("Undistortion"));
		frame->label_Optimized->hide();
	}
	else if (workspace == CALIBRATION)
	{
		frame->comboBoxViewspace->setEnabled(true);
		frame->toolButtonCameraSettings->setEnabled(true);
		frame->comboBoxWorkspace->setCurrentIndex(frame->comboBoxWorkspace->findText("Calibration"));
		frame->label_Optimized->hide();
	}
	else if (workspace == DIGITIZATION)
	{
		if (Project::getInstance()->getTrials().size() > 0 && 1){
			frame->comboBoxViewspace->setEnabled(true);
			frame->toolButtonCameraSettings->setEnabled(true);
		} 
		else
		{
			frame->comboBoxViewspace->setEnabled(false);
			frame->toolButtonCameraSettings->setEnabled(false);
		}

		frame->comboBoxWorkspace->setCurrentIndex(frame->comboBoxWorkspace->findText("Marker tracking"));
		if (!State::getInstance()->isLoading() && State::getInstance()->getActiveTrial() >= 0 && State::getInstance()->getActiveTrial() < (int) Project::getInstance()->getTrials().size())
		{
			ThreadScheduler::getInstance()->updateTrialData(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]);
		}

		if (Project::getInstance()->getTrials().size() > 0 && State::getInstance()->getActiveTrial() >= 0 && Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial())
			frame->toolButtonTrialSettings->setEnabled(!Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getIsDefault());

		if (Project::getInstance()->camerasOptimized() || Project::getInstance()->getNbImagesCalibration() <= 1)
		{
			frame->label_Optimized->hide();
		} else 
		{
			frame->label_Optimized->show();
		}
	}
}

void WorkspaceNavigationFrame::displayChanged(ui_state display)
{
	if (display == ALL_CAMERAS_FULL_HEIGHT)
	{
		frame->comboBoxViewspace->setCurrentIndex(frame->comboBoxViewspace->findText("All Cameras - Full Height"));
	}
	else if (display == ALL_CAMERAS_1ROW_SCALED)
	{
		frame->comboBoxViewspace->setCurrentIndex(frame->comboBoxViewspace->findText("All Cameras - 1 Row Scaled"));
	}
	else if (display == ALL_CAMERAS_2ROW_SCALED)
	{
		frame->comboBoxViewspace->setCurrentIndex(frame->comboBoxViewspace->findText("All Cameras - 2 Row Scaled"));
	}
	else if (display == ALL_CAMERAS_3ROW_SCALED)
	{
		frame->comboBoxViewspace->setCurrentIndex(frame->comboBoxViewspace->findText("All Cameras - 3 Row Scaled"));
	}
}

void WorkspaceNavigationFrame::activeTrialChanged(int activeTrial)
{
	if (activeTrial >= 0)
	{
		frame->comboBoxViewspace->setEnabled(true);
		frame->toolButtonCameraSettings->setEnabled(true);
		frame->comboBoxTrial->setCurrentIndex(activeTrial);
		if (!State::getInstance()->isLoading() && State::getInstance()->getActiveTrial() >= 0 && State::getInstance()->getActiveTrial() < (int) Project::getInstance()->getTrials().size())
		{
			ThreadScheduler::getInstance()->updateTrialData(Project::getInstance()->getTrials()[activeTrial]);
		}

		if (Project::getInstance()->getTrials().size() > 0 && State::getInstance()->getActiveTrial() >= 0 && Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial())
			frame->toolButtonTrialSettings->setEnabled(!Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getIsDefault());
	} 
	else
	{
		currentComboBoxWorkspaceIndex = frame->comboBoxWorkspace->currentIndex();
	}
}

void WorkspaceNavigationFrame::setTrialVisible(bool visible)
{
	if (visible)
	{
		frame->label_Trial->show();
		frame->toolButtonAddTrial->show();
		frame->comboBoxTrial->show();
		frame->toolButtonTrialSettings->show();
	}
	else
	{
		frame->label_Trial->hide();
		frame->toolButtonAddTrial->hide();
		frame->comboBoxTrial->hide();
		frame->toolButtonTrialSettings->hide();
	}
}

void WorkspaceNavigationFrame::on_comboBoxWorkspace_currentIndexChanged(QString value)
{
	if (currentComboBoxWorkspaceIndex != frame->comboBoxWorkspace->currentIndex())
	{
		if (WizardDockWidget::getInstance()->checkForPendingChanges())
		{
			if (value == "Undistortion")
			{
				State::getInstance()->changeWorkspace(UNDISTORTION);
				currentComboBoxWorkspaceIndex = frame->comboBoxWorkspace->currentIndex();
				setTrialVisible(false);
			}
			else if (value == "Calibration")
			{
				State::getInstance()->changeWorkspace(CALIBRATION);
				currentComboBoxWorkspaceIndex = frame->comboBoxWorkspace->currentIndex();
				setTrialVisible(false);
			}
			else if (value == "Marker tracking")
			{
				State::getInstance()->changeWorkspace(DIGITIZATION);
				currentComboBoxWorkspaceIndex = frame->comboBoxWorkspace->currentIndex();
				if (Project::getInstance()->isCalibrated() || (Project::getInstance()->getCalibration() == NO_CALIBRATION))
				{
					setTrialVisible(true);
				}
				else
				{
					setTrialVisible(false);
				}
			}
		}
		else
		{
			frame->comboBoxWorkspace->setCurrentIndex(currentComboBoxWorkspaceIndex);
		}
	} 
	else
	{
		if (value == "Marker tracking")
		{
			if (Project::getInstance()->isCalibrated() || (Project::getInstance()->getCalibration() == NO_CALIBRATION))
			{
				setTrialVisible(true);
			}
			else
			{
				setTrialVisible(false);
			}
		}

	}
}

void WorkspaceNavigationFrame::on_comboBoxTrial_currentIndexChanged(int idx)
{
	if (!updating)State::getInstance()->changeActiveTrial(idx);
}

void WorkspaceNavigationFrame::on_comboBoxViewspace_currentIndexChanged(QString value)
{
	if (value == "All Cameras - Full Height")
	{
		State::getInstance()->changeDisplay(ALL_CAMERAS_FULL_HEIGHT);
	}
	else if (value == "All Cameras - 1 Row Scaled")
	{
		State::getInstance()->changeDisplay(ALL_CAMERAS_1ROW_SCALED);
	}
	else if (value == "All Cameras - 2 Row Scaled")
	{
		State::getInstance()->changeDisplay(ALL_CAMERAS_2ROW_SCALED);
	}
	else if (value == "All Cameras - 3 Row Scaled")
	{
		State::getInstance()->changeDisplay(ALL_CAMERAS_3ROW_SCALED);
	}
}

void WorkspaceNavigationFrame::on_toolButtonTrialSettings_clicked()
{
	if (State::getInstance()->getActiveTrial() >= 0 && State::getInstance()->getActiveTrial() < (int) Project::getInstance()->getTrials().size())
	{
		TrialDialog* dialog = new TrialDialog(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]);
		dialog->exec();

		if (dialog->result())
		{
			MainWindow::getInstance()->redrawGL();
		}
		
		if (dialog->getDialogReturn() == TRIALDIALOGCHANGE)
		{
			frame->comboBoxTrial->setItemText(State::getInstance()->getActiveTrial(), Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getName());
			State::getInstance()->changeWorkspace(DIGITIZATION, true); 
			State::getInstance()->changeActiveFrameTrial(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveFrame(), true);
			MainWindow::getInstance()->redrawGL();
		}
		else if (dialog->getDialogReturn() == TRIALDIALOGUPDATEFILTER)
		{
			Trial* trial = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()];
			trial->recomputeAndFilterRigidBodyTransformations();
			MainWindow::getInstance()->redrawGL();
		}
		else if (dialog->getDialogReturn() == TRIALDIALOGUPDATE)
		{
			Trial* trial = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()];
			trial->setRequiresRecomputation(true);
			ThreadScheduler::getInstance()->updateTrialData(trial);
		}
		else if (dialog->getDialogReturn() == TRIALDIALOGDELETE)
		{
			Trial* trial = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()];
			int oldTrial = State::getInstance()->getActiveTrial();
			int activeTrial = State::getInstance()->getActiveTrial();
			Project::getInstance()->deleteTrial(trial);
			frame->comboBoxTrial->removeItem(oldTrial);

			if (activeTrial >= (int) Project::getInstance()->getTrials().size()) activeTrial -= 1;
			State::getInstance()->changeActiveTrial(activeTrial, true);

			if (activeTrial == -1)
			{
				State::getInstance()->changeWorkspace(DIGITIZATION, true);
				currentComboBoxWorkspaceIndex = frame->comboBoxWorkspace->currentIndex();
				if (Project::getInstance()->isCalibrated() || (Project::getInstance()->getCalibration() == NO_CALIBRATION))
				{
					setTrialVisible(true);
				}
				else
				{
					setTrialVisible(false);
				}
			}
		}

		delete dialog;
	}
}

void WorkspaceNavigationFrame::on_toolButtonCameraSettings_clicked()
{
	CameraSelector * diag = new CameraSelector(this); 
	bool ok = diag->exec();
	if (ok)
	{
		DetailViewDockWidget::getInstance()->relayout();
		MainWindow::getInstance()->relayoutCameras();
		MainWindow::getInstance()->redrawGL();
	}
	delete diag;
}

void WorkspaceNavigationFrame::on_toolButtonAddTrial_clicked()
{
	MainWindow::getInstance()->on_pushButtonNewTrial_clicked();
}

void WorkspaceNavigationFrame::setWorkState(work_state workspace)
{
	if (workspace == UNDISTORTION)
	{
		frame->comboBoxWorkspace->setCurrentIndex(frame->comboBoxWorkspace->findText("Undistortion"));
		setTrialVisible(false);
	}
	else if (workspace == CALIBRATION)
	{
		frame->comboBoxWorkspace->setCurrentIndex(frame->comboBoxWorkspace->findText("Calibration"));
		setTrialVisible(false);
	}
	else if (workspace == DIGITIZATION)
	{
		frame->comboBoxWorkspace->setCurrentIndex(frame->comboBoxWorkspace->findText("Marker tracking"));
		setTrialVisible(true);
	}
}

