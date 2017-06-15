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
///\file Shortcuts.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/Shortcuts.h"
#include "ui/MainWindow.h"
#include "ui/SequenceNavigationFrame.h"
#include "ui/PointsDockWidget.h"
#include "ui/WizardDockWidget.h"
#include "ui/DisplayOptionsDockWidget.h"
#include "ui/PlotWindow.h"
#include <QtGui/QShortcut>
#include <QKeyEvent>

using namespace xma;

Shortcuts* Shortcuts::instance = NULL;

Shortcuts::Shortcuts()
{
}

Shortcuts::~Shortcuts()
{
	instance = NULL;
}

void Shortcuts::bindApplicationShortcuts()
{
	QShortcut* shortcut = new QShortcut(QKeySequence(Qt::Key_Left), SequenceNavigationFrame::getInstance(), SLOT(on_toolButtonPrev_clicked()));
	shortcut->setContext(Qt::ApplicationShortcut);

	shortcut = new QShortcut(QKeySequence(Qt::Key_Right), SequenceNavigationFrame::getInstance(), SLOT(on_toolButtonNext_clicked()));
	shortcut->setContext(Qt::ApplicationShortcut);

	shortcut = new QShortcut(QKeySequence(Qt::Key_Up), PointsDockWidget::getInstance(), SLOT(selectPrevPoint()));
	shortcut->setContext(Qt::ApplicationShortcut);

	shortcut = new QShortcut(QKeySequence(Qt::Key_Down), PointsDockWidget::getInstance(), SLOT(selectNextPoint()));
	shortcut->setContext(Qt::ApplicationShortcut);

	shortcut = new QShortcut(QKeySequence(Qt::Key_Escape), SequenceNavigationFrame::getInstance(), SLOT(on_toolButtonStop_clicked()));
	shortcut->setContext(Qt::ApplicationShortcut);

	shortcut = new QShortcut(QKeySequence(Qt::Key_1), WizardDockWidget::getInstance(), SLOT(trackSelectedPointBackward()));
	shortcut->setContext(Qt::ApplicationShortcut);

	shortcut = new QShortcut(QKeySequence(Qt::Key_2), WizardDockWidget::getInstance(), SLOT(trackSelectedPointForward()));
	shortcut->setContext(Qt::ApplicationShortcut);

	shortcut = new QShortcut(QKeySequence(Qt::Key_3), WizardDockWidget::getInstance(), SLOT(goToLastTrackedFrame()));
	shortcut->setContext(Qt::ApplicationShortcut);

	shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_3), WizardDockWidget::getInstance(), SLOT(goToFirstTrackedFrame()));
	shortcut->setContext(Qt::ApplicationShortcut);

	shortcut = new QShortcut(QKeySequence(Qt::Key_Q), SequenceNavigationFrame::getInstance(), SLOT(on_toolButtonPrev_clicked()));
	shortcut->setContext(Qt::ApplicationShortcut);

	shortcut = new QShortcut(QKeySequence(Qt::Key_W), SequenceNavigationFrame::getInstance(), SLOT(on_toolButtonNext_clicked()));
	shortcut->setContext(Qt::ApplicationShortcut);

	shortcut = new QShortcut(QKeySequence(Qt::Key_5), SequenceNavigationFrame::getInstance(), SLOT(moveNFramesBackward()));
	shortcut->setContext(Qt::ApplicationShortcut);

	shortcut = new QShortcut(QKeySequence(Qt::Key_6), SequenceNavigationFrame::getInstance(), SLOT(moveNFramesForward()));
	shortcut->setContext(Qt::ApplicationShortcut);

	shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_5), SequenceNavigationFrame::getInstance(), SLOT(setAndMoveNFramesBackward()));
	shortcut->setContext(Qt::ApplicationShortcut);

	shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_6), SequenceNavigationFrame::getInstance(), SLOT(setAndMoveNFramesForward()));
	shortcut->setContext(Qt::ApplicationShortcut);

	shortcut = new QShortcut(QKeySequence(Qt::Key_7), SequenceNavigationFrame::getInstance(), SLOT(moveFrameToMissingForward()));
	shortcut->setContext(Qt::ApplicationShortcut);

	shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_7), SequenceNavigationFrame::getInstance(), SLOT(moveFrameToMissingBackward()));
	shortcut->setContext(Qt::ApplicationShortcut);

	shortcut = new QShortcut(QKeySequence(Qt::Key_C), MainWindow::getInstance(), SLOT(centerViews()));
	shortcut->setContext(Qt::ApplicationShortcut);

	shortcut = new QShortcut(QKeySequence(Qt::Key_I), WizardDockWidget::getInstance(), SLOT(interpolateActive()));
	shortcut->setContext(Qt::ApplicationShortcut);

	shortcut = new QShortcut(QKeySequence(Qt::SHIFT + Qt::Key_I), WizardDockWidget::getInstance(), SLOT(interpolateAll()));
	shortcut->setContext(Qt::ApplicationShortcut);
	
	shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_I), PlotWindow::getInstance(), SLOT(setInterpolation()));
	shortcut->setContext(Qt::ApplicationShortcut);
	
	shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_U), PlotWindow::getInstance(), SLOT(setUntrackable()));
	shortcut->setContext(Qt::ApplicationShortcut);

	shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_A), PointsDockWidget::getInstance(), SLOT(selectAllPoints()));
	shortcut->setContext(Qt::ApplicationShortcut);

	shortcut = new QShortcut(QKeySequence(Qt::Key_4), PlotWindow::getInstance(), SLOT(goToNextPointAboveBackprojectionError()));
	shortcut->setContext(Qt::ApplicationShortcut);

	shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_4), PlotWindow::getInstance(), SLOT(goToPrevPointAboveBackprojectionError()));
	shortcut->setContext(Qt::ApplicationShortcut);

	shortcut = new QShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Delete), PlotWindow::getInstance(), SLOT(deleteAllAboveBackprojectionError()));
	shortcut->setContext(Qt::ApplicationShortcut);

	shortcut = new QShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Backspace), PlotWindow::getInstance(), SLOT(deleteAllAboveBackprojectionError()));
	shortcut->setContext(Qt::ApplicationShortcut);

	shortcut = new QShortcut(QKeySequence(Qt::Key_H), DisplayOptionsDockWidget::getInstance(), SLOT(toggleHideAll()));
	shortcut->setContext(Qt::ApplicationShortcut);

	shortcut = new QShortcut(QKeySequence(Qt::Key_O), PlotWindow::getInstance(), SLOT(setEventOn()));
	shortcut->setContext(Qt::ApplicationShortcut);

	shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_O), PlotWindow::getInstance(), SLOT(setEventOff()));
	shortcut->setContext(Qt::ApplicationShortcut);
}

void Shortcuts::installEventFilterToChildren(QObject* object)
{
	QObjectList list = object->children();
	for (int i = 0; i < list.size(); i++)
	{
		installEventFilterToChildren(list.at(i));
	}

	object->installEventFilter(this);
}

void Shortcuts::removeEventFilterToChildren(QObject* object)
{
	QObjectList list = object->children();
	for (int i = 0; i < list.size(); i++)
	{
		installEventFilterToChildren(list.at(i));
	}

	object->removeEventFilter(this);
}

bool Shortcuts::checkShortcut(QObject* target, QEvent* event)
{
	return eventFilter(target, event);
}

bool Shortcuts::eventFilter(QObject* target, QEvent* event)
{
	if (event->type() == QEvent::KeyRelease)
	{
		QKeyEvent* _keyEvent = static_cast<QKeyEvent*>(event);
		if (_keyEvent->key() == Qt::Key_Shift)
		{
			PlotWindow::getInstance()->ShiftReleased();
		}
	}

	if (event->type() == QEvent::KeyPress)
	{
		QKeyEvent* _keyEvent = static_cast<QKeyEvent*>(event);

		if (_keyEvent->key() == Qt::Key_Shift)
		{
			PlotWindow::getInstance()->ShiftPressed();
		}

		if (_keyEvent->key() == Qt::Key_Left)
		{
			SequenceNavigationFrame::getInstance()->on_toolButtonPrev_clicked();
			return true;
		}
		if (_keyEvent->key() == Qt::Key_Right)
		{
			SequenceNavigationFrame::getInstance()->on_toolButtonNext_clicked();
			return true;
		}
		if (_keyEvent->key() == Qt::Key_Up)
		{
			PointsDockWidget::getInstance()->selectPrevPoint();
			return true;
		}
		if (_keyEvent->key() == Qt::Key_Down)
		{
			PointsDockWidget::getInstance()->selectNextPoint();
			return true;
		}
		if (_keyEvent->key() == Qt::Key_Escape)
		{
			SequenceNavigationFrame::getInstance()->on_toolButtonStop_clicked();
			return true;
		}
		if (_keyEvent->key() == Qt::Key_1)
		{
			WizardDockWidget::getInstance()->trackSelectedPointBackward();
			return true;
		}
		if (_keyEvent->key() == Qt::Key_2)
		{
			WizardDockWidget::getInstance()->trackSelectedPointForward();
			return true;
		}
		if (_keyEvent->key() == Qt::Key_3 && !_keyEvent->modifiers().testFlag(Qt::ControlModifier))
		{
			WizardDockWidget::getInstance()->goToLastTrackedFrame();
			return true;
		}
		if (_keyEvent->key() == Qt::Key_3 && _keyEvent->modifiers().testFlag(Qt::ControlModifier))
		{
			WizardDockWidget::getInstance()->goToFirstTrackedFrame();
			return true;
		}
		if (_keyEvent->key() == Qt::Key_5  && !_keyEvent->modifiers().testFlag(Qt::ControlModifier))
		{
			SequenceNavigationFrame::getInstance()->moveNFramesBackward();
			return true;
		}
		if (_keyEvent->key() == Qt::Key_6  && !_keyEvent->modifiers().testFlag(Qt::ControlModifier))
		{
			SequenceNavigationFrame::getInstance()->moveNFramesForward();
			return true;
		}
		if (_keyEvent->key() == Qt::Key_5  && _keyEvent->modifiers().testFlag(Qt::ControlModifier))
		{
			SequenceNavigationFrame::getInstance()->setAndMoveNFramesBackward();
			return true;
		}
		if (_keyEvent->key() == Qt::Key_6  && _keyEvent->modifiers().testFlag(Qt::ControlModifier))
		{
			SequenceNavigationFrame::getInstance()->setAndMoveNFramesForward();
			return true;
		}
		if (_keyEvent->key() == Qt::Key_7  && !_keyEvent->modifiers().testFlag(Qt::ControlModifier))
		{
			SequenceNavigationFrame::getInstance()->moveFrameToMissingForward();
			return true;
		}
		if (_keyEvent->key() == Qt::Key_7  && _keyEvent->modifiers().testFlag(Qt::ControlModifier))
		{
			SequenceNavigationFrame::getInstance()->moveFrameToMissingBackward();
			return true;
		}
		if (_keyEvent->key() == Qt::Key_Q)
		{
			SequenceNavigationFrame::getInstance()->on_toolButtonPrev_clicked();
			return true;
		}
		if (_keyEvent->key() == Qt::Key_W)
		{
			SequenceNavigationFrame::getInstance()->on_toolButtonNext_clicked();
			return true;
		}
		if (_keyEvent->key() == Qt::Key_C)
		{
			MainWindow::getInstance()->centerViews();
			return true;
		}
		if (_keyEvent->key() == Qt::Key_O && !_keyEvent->modifiers().testFlag(Qt::ControlModifier))
		{
			PlotWindow::getInstance()->setEventOn();
			return true;
		}
		if (_keyEvent->key() == Qt::Key_O  && _keyEvent->modifiers().testFlag(Qt::ControlModifier))
		{
			PlotWindow::getInstance()->setEventOff();
			return true;
		}
		if (_keyEvent->key() == Qt::Key_A && _keyEvent->modifiers().testFlag(Qt::ControlModifier))
		{
			PointsDockWidget::getInstance()->selectAllPoints();
			return true;
		}
		if (_keyEvent->key() == Qt::Key_I && !_keyEvent->modifiers().testFlag(Qt::ShiftModifier) && !_keyEvent->modifiers().testFlag(Qt::ControlModifier))
		{
			WizardDockWidget::getInstance()->interpolateActive();
			return true;
		}
		if (_keyEvent->key() == Qt::Key_I && _keyEvent->modifiers().testFlag(Qt::ShiftModifier) && !_keyEvent->modifiers().testFlag(Qt::ControlModifier))
		{
			WizardDockWidget::getInstance()->interpolateAll();
			return true;
		}
		if (_keyEvent->key() == Qt::Key_I && !_keyEvent->modifiers().testFlag(Qt::ShiftModifier) && _keyEvent->modifiers().testFlag(Qt::ControlModifier))
		{
			PlotWindow::getInstance()->setInterpolation();
			return true;
		}
		if (_keyEvent->key() == Qt::Key_U && !_keyEvent->modifiers().testFlag(Qt::ShiftModifier) && _keyEvent->modifiers().testFlag(Qt::ControlModifier))
		{
			PlotWindow::getInstance()->setUntrackable();
			return true;
		}
		if (_keyEvent->key() == Qt::Key_4 && !_keyEvent->modifiers().testFlag(Qt::ControlModifier))
		{
			PlotWindow::getInstance()->goToNextPointAboveBackprojectionError();
			return true;
		}
		if (_keyEvent->key() == Qt::Key_4&& _keyEvent->modifiers().testFlag(Qt::ControlModifier))
		{
			PlotWindow::getInstance()->goToPrevPointAboveBackprojectionError();
			return true;
		}
		if (_keyEvent->key() == Qt::Key_Delete&& _keyEvent->modifiers().testFlag(Qt::ShiftModifier))
		{
			PlotWindow::getInstance()->deleteAllAboveBackprojectionError();
			return true;
		}
		if (_keyEvent->key() == Qt::Key_Backspace&& _keyEvent->modifiers().testFlag(Qt::ShiftModifier))
		{
			PlotWindow::getInstance()->deleteAllAboveBackprojectionError();
			return true;
		}
		if (_keyEvent->key() == Qt::Key_H)
		{
			DisplayOptionsDockWidget::getInstance()->toggleHideAll();
			return true;
		}
	}
	return false;
}

Shortcuts* Shortcuts::getInstance()
{
	if (!instance)
	{
		instance = new Shortcuts();
	}
	return instance;
}

