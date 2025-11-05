//  ----------------------------------
//  XMALab -- Copyright (c) 2015, Brown University, Providence, RI.
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
//  PROVIDED "AS IS", INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
//  FOR ANY PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR ANY 
//  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR FOR ANY DAMAGES WHATSOEVER RESULTING 
//  FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
//  OTHER TORTIOUS ACTION, OR ANY OTHER LEGAL THEORY, ARISING OUT OF OR IN CONNECTION 
//  WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
//  ----------------------------------
//  
///\file SequenceNavigationFrame.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/SequenceNavigationFrame.h"
#include "ui_SequenceNavigationFrame.h"

#include "ui/State.h"
#include "ui/ConsoleDockWidget.h"
#include "ui/WizardDockWidget.h"
#include "ui/MainWindow.h"
#include "ui/PlotWindow.h"
#include "ui/WorldViewDockWidget.h"
#include "core/Project.h"
#include "core/Trial.h"
#include "core/Marker.h"

#include <QInputDialog>
#include <core/Settings.h>


using namespace xma;

SequenceNavigationFrame* SequenceNavigationFrame::instance = NULL;


SequenceNavigationFrame::SequenceNavigationFrame(QWidget* parent) :
	QFrame(parent),
	frame(new Ui::SequenceNavigationFrame)
{
	frame->setupUi(this);
	frame->spinBoxFrame->setMinimum(1);
	frame->horizontalSlider->setValue(0);
	frame->spinBoxFrame->setValue(1);
	connect(State::getInstance(), &State::activeFrameCalibrationChanged, this, &SequenceNavigationFrame::activeFrameChanged);
	connect(State::getInstance(), &State::activeFrameTrialChanged, this, &SequenceNavigationFrame::activeFrameChanged);
	connect(State::getInstance(), &State::workspaceChanged, this, &SequenceNavigationFrame::workspaceChanged);
	connect(State::getInstance(), &State::activeTrialChanged, this, &SequenceNavigationFrame::activeTrialChanged);

	play_tag = 0;
	play_timer = new QTimer(this);
	connect(play_timer, &QTimer::timeout, this, &SequenceNavigationFrame::play_update);

	updating = false;
}

SequenceNavigationFrame::~SequenceNavigationFrame()
{
	delete frame;

	instance = NULL;
}

void SequenceNavigationFrame::moveNFrames(int n)
{
	int f;
	if (State::getInstance()->getWorkspace() == CALIBRATION)
	{
		int f = State::getInstance()->getActiveFrameCalibration() + n;
		f = (f < 0) ? 0 : f;
		f = (f >= Project::getInstance()->getNbImagesCalibration()) ? Project::getInstance()->getNbImagesCalibration() - 1 : f;
		changeFrame(f);
	}
	else if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		if (Project::getInstance()->getTrials().size() > 0)
		{
			int f = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveFrame() + n;
			f = (f < Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1) ? Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1 : f;
			f = (f > Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1) ? Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1 : f;
			changeFrame(f);
		}
	}
}

bool SequenceNavigationFrame::setNFrames()
{
	bool ok;
	int n = QInputDialog::getInt(this, "Set number of frames to advance", "Frames", Settings::getInstance()->getIntSetting("FrameAdvance"), 1, 99, 1,& ok);
	if (ok)
	{
		Settings::getInstance()->set("FrameAdvance", n);
		return true;
	}
	return false;
}


SequenceNavigationFrame* SequenceNavigationFrame::getInstance()
{
	if (!instance)

	{
		instance = new SequenceNavigationFrame(MainWindow::getInstance());
	}
	return instance;
}

void SequenceNavigationFrame::setNbImages(int nbImages)
{
	maxFrame = nbImages;
	setStartEndSequence(1, nbImages);
}

void SequenceNavigationFrame::setStartEndSequence(int start, int end)
{
	if (end < start || start <= 0 || end > maxFrame) return;

	endFrame = end;
	startFrame = start;

	if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		if ((int)Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0)
		{
			if (startFrame > Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveFrame())
			{
				State::getInstance()->changeActiveFrameTrial(startFrame - 1);
			}
			else if (endFrame < Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveFrame())
			{
				State::getInstance()->changeActiveFrameTrial(endFrame - 1);
			}
		}
	}

	frame->toolButtonFrameStart->setText(QString::number(startFrame));
	frame->toolButtonFrameEnd->setText(QString::number(endFrame));
	updating = true;
	frame->horizontalSlider->setMinimum(startFrame - 1);
	frame->horizontalSlider->setMaximum(endFrame - 1);

	frame->spinBoxFrame->setMinimum(startFrame);
	frame->spinBoxFrame->setMaximum(endFrame);
	updating = false;

	if (frame->horizontalSlider->value() == startFrame - 1) frame->toolButtonPrev->setEnabled(false);
	if (frame->horizontalSlider->value() == endFrame - 1) frame->toolButtonNext->setEnabled(false);

	PlotWindow::getInstance()->resetRange();
	PlotWindow::getInstance()->draw();
}

void SequenceNavigationFrame::activeFrameChanged(int activeFrame)
{
	if (activeFrame + 1 != frame->spinBoxFrame->value())
	{
		frame->spinBoxFrame->setValue(activeFrame + 1);
	}
	if (activeFrame != frame->horizontalSlider->value())
	{
		frame->horizontalSlider->setValue(activeFrame);
	}

	if (frame->spinBoxFrame->value() == frame->spinBoxFrame->maximum())
	{
		frame->toolButtonNext->setEnabled(false);
	}
	else
	{
		frame->toolButtonNext->setEnabled(true);
	}

	if (frame->spinBoxFrame->value() == frame->spinBoxFrame->minimum())
	{
		frame->toolButtonPrev->setEnabled(false);
	}
	else
	{
		frame->toolButtonPrev->setEnabled(true);
	}
}

void SequenceNavigationFrame::workspaceChanged(work_state workspace)
{
	on_toolButtonStop_clicked();

	if (workspace == CALIBRATION && Project::getInstance()->getNbImagesCalibration() > 1)
	{
		setNbImages(Project::getInstance()->getNbImagesCalibration());
		changeFrame(State::getInstance()->getActiveFrameCalibration());

		frame->toolButtonFrameEnd->setDisabled(true);
		frame->toolButtonFrameStart->setDisabled(true);
		frame->toolButtonPlay->hide();
		frame->toolButtonPlayBackward->hide();
		frame->toolButtonStop->hide();
	}
	else if (workspace == DIGITIZATION && Project::getInstance()->getTrials().size() > 0)
	{
		if ((int)Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0)
		{
			setNbImages(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getNbImages());
			setStartEndSequence(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame(), Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame());
			activeFrameChanged(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveFrame());
			changeFrame(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveFrame());
			frame->toolButtonFrameEnd->setDisabled(false);
			frame->toolButtonFrameStart->setDisabled(false);
			frame->toolButtonPlay->show();
			frame->toolButtonPlayBackward->show();
			frame->toolButtonStop->show();
		}
	}
}


void SequenceNavigationFrame::activeTrialChanged(int activeTrial)
{
	if (activeTrial >= 0)
	{
		on_toolButtonStop_clicked();
		if (State::getInstance()->getWorkspace() == DIGITIZATION && Project::getInstance()->getTrials().size() > 0)
		{
			if ((int)Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0)
			{
				setNbImages(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getNbImages());
				setStartEndSequence(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame(), Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame());
				activeFrameChanged(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveFrame());
				changeFrame(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveFrame());
			}
		}
	}
}

void SequenceNavigationFrame::changeFrame(int f)
{
	
	if (State::getInstance()->getWorkspace() == CALIBRATION)
	{
		if (f >= 0 && f < Project::getInstance()->getNbImagesCalibration()){
			State::getInstance()->changeActiveFrameCalibration(f,true);
		}
	}
	else if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		if (Project::getInstance()->getTrials().size() > 0 && f >= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1 &&
			f <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1){
				if (f != Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveFrame())
				{
					Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->setActiveFrame(f);
				}
				if (State::getInstance()->getActiveFrameTrial() != f){
					State::getInstance()->changeActiveFrameTrial(f);
				}
		}
	}
}

void SequenceNavigationFrame::on_horizontalSlider_valueChanged(int value)
{
	if (!updating)changeFrame(value);
	QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

void SequenceNavigationFrame::on_spinBoxFrame_valueChanged(int value)
{
	if (!updating)changeFrame(value - 1);
}

void SequenceNavigationFrame::play_update()
{
	if (play_tag > 0)
	{
		on_toolButtonNext_clicked();
	}
	else if (play_tag < 0)
	{
		on_toolButtonPrev_clicked();
	}

	QApplication::processEvents();

	if ((play_tag < 0 && frame->toolButtonPrev->isEnabled() == false) ||
		(play_tag > 0 && frame->toolButtonNext->isEnabled() == false))
	{
		on_toolButtonStop_clicked();
	}
}



void SequenceNavigationFrame::moveNFramesForward()
{
	moveNFrames(Settings::getInstance()->getIntSetting("FrameAdvance"));
}

void SequenceNavigationFrame::moveNFramesBackward()
{
	moveNFrames(-Settings::getInstance()->getIntSetting("FrameAdvance"));
}

void SequenceNavigationFrame::setAndMoveNFramesForward()
{
	if(setNFrames())moveNFrames(Settings::getInstance()->getIntSetting("FrameAdvance"));
}

void SequenceNavigationFrame::setAndMoveNFramesBackward()
{
	if (setNFrames())moveNFrames(-Settings::getInstance()->getIntSetting("FrameAdvance"));
}

void SequenceNavigationFrame::moveFrameToMissingForward()
{
	if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		if (Project::getInstance()->getTrials().size() > 0 && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarker())
		{
			int f = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveFrame();
			while (f <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1){
				bool allset = true;
				bool allunset = true;
				for (int i = 0; i < Project::getInstance()->getCameras().size(); i++)
				{
					if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarker()->getStatus2D()[i][f] <= UNDEFINED)
					{
						allset = false;
					} 
					else
					{
						allunset = false;
					}
					if (!allset && !allunset)
					{
						changeFrame(f);
						return;
					}
				}

				f++;
			}
		}
	}
}

void SequenceNavigationFrame::moveFrameToMissingBackward()
{
	if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		if (Project::getInstance()->getTrials().size() > 0 && Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarker())
		{
			int f = Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveFrame();
			while (f >= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1){
				bool allset = true;
				bool allunset = true;
				for (int i = 0; i < Project::getInstance()->getCameras().size(); i++)
				{
					if (Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveMarker()->getStatus2D()[i][f] <= UNDEFINED)
					{
						allset = false;
					}
					else
					{
						allunset = false;
					}
					if (!allset && !allunset)
					{
						changeFrame(f);
						return;
					}
				}

				f--;
			}
		}
	}
}

void SequenceNavigationFrame::on_toolButtonPlay_clicked()
{
	on_toolButtonStop_clicked();

	if (play_tag == 0)
	{
		play_tag = 1;
		play_timer->start(1);
	}

	frame->toolButtonPlay->setChecked(true);
}

void SequenceNavigationFrame::on_toolButtonPlayBackward_clicked()
{
	on_toolButtonStop_clicked();

	if (play_tag == 0)
	{
		play_tag = -1;
		play_timer->start(1);
	}

	frame->toolButtonPlayBackward->setChecked(true);
}

void SequenceNavigationFrame::on_toolButtonStop_clicked()
{
	if (play_tag != 0)
	{
		play_tag = 0;
		play_timer->stop();
	}

	frame->toolButtonPlayBackward->setChecked(false);
	frame->toolButtonPlay->setChecked(false);

	WizardDockWidget::getInstance()->stop();
}

void SequenceNavigationFrame::on_toolButtonNext_clicked()
{
	changeFrame(frame->horizontalSlider->value() + 1);
}

void SequenceNavigationFrame::on_toolButtonPrev_clicked()
{
	changeFrame(frame->horizontalSlider->value() - 1);
}

void SequenceNavigationFrame::on_toolButtonFrameStart_clicked()
{
	bool ok;
	int idx = QInputDialog::getInt(this, tr("Set start frame"),
	                               tr("frame "), startFrame, 1, endFrame, 1, &ok);
	if (ok)
	{
		Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->setStartFrame(idx);
		setStartEndSequence(idx, endFrame);
		WorldViewDockWidget::getInstance()->setStartEndSequence(idx, endFrame);
	}
}

void SequenceNavigationFrame::on_toolButtonFrameEnd_clicked()
{
	bool ok;
	int idx = QInputDialog::getInt(this, "Set end frame (Max : " + QString::number(maxFrame) + ")",
	                               tr("frame "), endFrame, startFrame, maxFrame, 1, &ok);
	if (ok)
	{
		Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->setEndFrame(idx);
		setStartEndSequence(startFrame, idx);
		WorldViewDockWidget::getInstance()->setStartEndSequence(startFrame, idx);
	}
}

