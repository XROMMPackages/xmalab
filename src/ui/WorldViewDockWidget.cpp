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
///\file WorldViewDockWidget.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/WorldViewDockWidget.h"
#include "ui_WorldViewDockWidget.h"
#include "ui/MainWindow.h"
#include "ui/Shortcuts.h"
#include "core/Project.h"
#include "core/Trial.h"

#include <QtGui/QApplication>
#include <QtGui/QCloseEvent>

using namespace xma;

WorldViewDockWidget* WorldViewDockWidget::instance = NULL;


WorldViewDockWidget::WorldViewDockWidget(QWidget* parent) : QDockWidget(parent), dock(new Ui::WorldViewDockWidget)
{
	dock->setupUi(this);
	setAllowedAreas(Qt::AllDockWidgetAreas);
	resize(500, 500);

	Shortcuts::getInstance()->installEventFilterToChildren(this);

	dock->spinBoxFrame->setMinimum(1);
	dock->horizontalSlider->setValue(0);
	dock->spinBoxFrame->setValue(1);

	connect(State::getInstance(), SIGNAL(workspaceChanged(work_state)), this, SLOT(workspaceChanged(work_state)));
	connect(State::getInstance(), SIGNAL(activeTrialChanged(int)), this, SLOT(activeTrialChanged(int)));

	play_tag = 0;
	play_timer = new QTimer(this);
	connect(play_timer, SIGNAL(timeout()), this, SLOT(play_update()));

	updating = false;
}

WorldViewDockWidget::~WorldViewDockWidget()
{
	delete dock;
	instance = NULL;
}

WorldViewDockWidget* WorldViewDockWidget::getInstance()
{
	if (!instance)
	{
		instance = new WorldViewDockWidget(MainWindow::getInstance());
		MainWindow::getInstance()->addDockWidget(Qt::LeftDockWidgetArea, instance);
	}
	return instance;
}

void WorldViewDockWidget::setTimeline(bool enabled)
{
	if (enabled)
	{
		dock->openGL->setUseCustomTimeline(true);
		dock->horizontalSlider->setEnabled(true);
		dock->spinBoxFrame->setEnabled(true);
		dock->toolButtonNext->setEnabled(true);
		dock->toolButtonPlay->setEnabled(true);
		dock->toolButtonPlayBackward->setEnabled(true);
		dock->toolButtonPrev->setEnabled(true);
		dock->toolButtonStop->setEnabled(true);

		setStartEndSequence(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame(), Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame());
	}
	else
	{
		dock->openGL->setUseCustomTimeline(false);
		dock->horizontalSlider->setEnabled(false);
		dock->spinBoxFrame->setEnabled(false);
		dock->toolButtonNext->setEnabled(false);
		dock->toolButtonPlay->setEnabled(false);
		dock->toolButtonPlayBackward->setEnabled(false);
		dock->toolButtonPrev->setEnabled(false);
		dock->toolButtonStop->setEnabled(false);
		on_toolButtonStop_clicked();
	}
}

void WorldViewDockWidget::changeFrame(int frame)
{
	if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		if (Project::getInstance()->getTrials().size() > 0 && frame >= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame() - 1 &&
			frame <= Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame() - 1)
		{
			if (frame + 1 != dock->spinBoxFrame->value())
			{
				dock->spinBoxFrame->setValue(frame + 1);
				draw();
			}
			if (frame != dock->horizontalSlider->value())
			{
				dock->horizontalSlider->setValue(frame);
				draw();
			}

			if (dock->spinBoxFrame->value() == dock->spinBoxFrame->maximum())
			{
				dock->toolButtonNext->setEnabled(false);
			}
			else
			{
				if(dock->checkBoxEnable->isChecked()) dock->toolButtonNext->setEnabled(true);
			}

			if (dock->spinBoxFrame->value() == dock->spinBoxFrame->minimum())
			{
				dock->toolButtonPrev->setEnabled(false);
			}
			else
			{
				if (dock->checkBoxEnable->isChecked()) dock->toolButtonPrev->setEnabled(true);
			}
		}
	}
	
}

void WorldViewDockWidget::resizeEvent(QResizeEvent* event)
{
	dock->openGL->repaint();
}

void WorldViewDockWidget::setSharedGLContext(const QGLContext* sharedContext)
{
	QGLContext* context = new QGLContext(sharedContext->format(), dock->openGL);
	context->create(sharedContext);
	dock->openGL->setContext(context, sharedContext, true);
}

void WorldViewDockWidget::draw()
{
	if (!dock->checkBoxEnable->isChecked()){
		dock->openGL->setFrame(State::getInstance()->getActiveFrameTrial());
	}
	else
	{
		dock->openGL->setFrame(dock->spinBoxFrame->value() - 1);
	}
	dock->openGL->update();
}

void WorldViewDockWidget::setStartEndSequence(int start, int end)
{
	if (State::getInstance()->getWorkspace() == DIGITIZATION)
	{
		if ((int)Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0)
		{
			if (start > Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveFrame())
			{
				changeFrame(start - 1);
			}
			else if (end < Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getActiveFrame())
			{
				changeFrame(end - 1);
			}
		}
	}

	updating = true;
	dock->horizontalSlider->setMinimum(start - 1);
	dock->horizontalSlider->setMaximum(end - 1);

	dock->spinBoxFrame->setMinimum(start);
	dock->spinBoxFrame->setMaximum(end);
	updating = false;

	if (dock->checkBoxEnable->isChecked()){
		dock->toolButtonPrev->setEnabled(dock->horizontalSlider->value() != start - 1);
		dock->toolButtonNext->setEnabled(dock->horizontalSlider->value() != end - 1);
	}
}

void WorldViewDockWidget::closeEvent(QCloseEvent* event)
{
	on_toolButtonStop_clicked();
	event->ignore();
	MainWindow::getInstance()->on_action3D_world_view_triggered(false);
}

void WorldViewDockWidget::activeTrialChanged(int activeTrial)
{
	if (activeTrial >= 0)
	{
		on_toolButtonStop_clicked();
		if (State::getInstance()->getWorkspace() == DIGITIZATION && Project::getInstance()->getTrials().size() > 0)
		{
			if ((int)Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0)
			{
				setStartEndSequence(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame(), Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame());
			}
		}
	}
}

void WorldViewDockWidget::workspaceChanged(work_state workspace)
{
	on_toolButtonStop_clicked();
	setTimeline(false);

	if (workspace == DIGITIZATION && Project::getInstance()->getTrials().size() > 0)
	{
		
		if ((int)Project::getInstance()->getTrials().size() > State::getInstance()->getActiveTrial() && State::getInstance()->getActiveTrial() >= 0)
		{
			dock->checkBoxEnable->setEnabled(true);
			setStartEndSequence(Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getStartFrame(), Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEndFrame());
		}
	} 
	else
	{	
		dock->checkBoxEnable->setChecked(false);
		dock->checkBoxEnable->setEnabled(false);
		dock->spinBoxFrame->setMinimum(1);
		dock->horizontalSlider->setValue(0);
		dock->spinBoxFrame->setValue(1);
	}
}

void WorldViewDockWidget::on_checkBoxEnable_clicked(bool state)
{
	setTimeline(dock->checkBoxEnable->isChecked());

	draw();
}

void WorldViewDockWidget::on_horizontalSlider_valueChanged(int value)
{
	if (!updating)changeFrame(value);
	QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

void WorldViewDockWidget::on_spinBoxFrame_valueChanged(int value)
{
	if (!updating)changeFrame(value - 1);
}

void WorldViewDockWidget::play_update()
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

	if (play_tag < 0 && dock->toolButtonPrev->isEnabled() == false)
	{
		changeFrame(dock->horizontalSlider->maximum());
	} 
	if	(play_tag > 0 && dock->toolButtonNext->isEnabled() == false)
	{
		changeFrame(dock->horizontalSlider->minimum());
	}
}


void WorldViewDockWidget::on_toolButtonPlay_clicked()
{
	on_toolButtonStop_clicked();

	if (play_tag == 0)
	{
		play_tag = 1;
		play_timer->start(1);
	}

	dock->toolButtonPlay->setChecked(true);
}

void WorldViewDockWidget::on_toolButtonPlayBackward_clicked()
{
	on_toolButtonStop_clicked();

	if (play_tag == 0)
	{
		play_tag = -1;
		play_timer->start(1);
	}

	dock->toolButtonPlayBackward->setChecked(true);
}

void WorldViewDockWidget::on_toolButtonStop_clicked()
{
	if (play_tag != 0)
	{
		play_tag = 0;
		play_timer->stop();
	}

	dock->toolButtonPlayBackward->setChecked(false);
	dock->toolButtonPlay->setChecked(false);
}

void WorldViewDockWidget::on_toolButtonNext_clicked()
{
	changeFrame(dock->horizontalSlider->value() + 1);
}

void WorldViewDockWidget::on_toolButtonPrev_clicked()
{
	changeFrame(dock->horizontalSlider->value() - 1);
}