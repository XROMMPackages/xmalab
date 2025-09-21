//  ----------------------------------
//  XMALab -- Copyright � 2015, Brown University, Providence, RI.
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
//  PROVIDED �AS IS�, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
//  FOR ANY PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR ANY 
//  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR FOR ANY DAMAGES WHATSOEVER RESULTING 
//  FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
//  OTHER TORTIOUS ACTION, OR ANY OTHER LEGAL THEORY, ARISING OUT OF OR IN CONNECTION 
//  WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
//  ----------------------------------
//  
///\file EventDockWidget.cpp
///\author Benjamin Knorlein
///\date 06/12/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/EventDockWidget.h"
#include "ui_EventDockWidget.h"
#include "ui/MainWindow.h"
#include "ui/Shortcuts.h"

#include "core/Trial.h"
#include "core/Project.h"

#include <QCloseEvent>
#include <iostream>
#include <QInputDialog>
#include <QCheckBox>
#include <QToolButton>
#include <QColorDialog>
#include "ErrorDialog.h"
#include "PlotWindow.h"


using namespace xma;

EventDockWidget* EventDockWidget::instance = nullptr;

EventDockWidget::EventDockWidget(QWidget* parent) :
	QDockWidget(parent),
	dock(new Ui::EventDockWidget)
{
	dock->setupUi(this);

	Shortcuts::getInstance()->installEventFilterToChildren(this);
	// Use direct connections with lambdas instead of QSignalMapper (Qt6)

	connect(State::getInstance(), SIGNAL(workspaceChanged(work_state)), this, SLOT(workspaceChanged(work_state)));
	connect(State::getInstance(), SIGNAL(activeTrialChanged(int)), this, SLOT(activeTrialChanged(int)));
}


EventDockWidget::~EventDockWidget()
{
	delete dock;
	instance = nullptr;
}


EventDockWidget* EventDockWidget::getInstance()
{
	if (!instance)
	{
		instance = new EventDockWidget(MainWindow::getInstance());
	}
	return instance;
}

void EventDockWidget::addEvent(const QString& name, const QColor& color, bool draw)
{
	event_entry entry;
	entry.name = name;
	entry.checkbox = new QCheckBox(dock->scrollAreaWidgetContents);
	entry.checkbox->setText(name);
	entry.checkbox->setChecked(draw);
	entry.button = new QToolButton(dock->scrollAreaWidgetContents);
	entry.button->setText("");
	entry.color = color;

	QPixmap pix3(16, 16);
	pix3.fill(QColor(entry.color));
	entry.button->setIcon(pix3);

	   connect(entry.button, &QToolButton::clicked, this, [this, entry]() {
		   changeColor(entry.name);
	   });
	   connect(entry.checkbox, &QCheckBox::clicked, this, [this, entry]() {
		   checkbox_clicked(entry.name);
	   });
	
	dock->gridLayout_3->addWidget(entry.checkbox, entries.size(), 0, 1, 1);
	dock->gridLayout_3->addWidget(entry.button, entries.size(), 1, 1, 1);
	entries.push_back(entry);
}

void EventDockWidget::deleteEvent(int idx)
{
	dock->gridLayout_3->removeWidget(entries[idx].button);
	dock->gridLayout_3->removeWidget(entries[idx].checkbox);

	// No need to remove mappings with lambdas

	delete entries[idx].button;
	delete entries[idx].checkbox;

	entries.erase(entries.begin() + idx);
}

void EventDockWidget::closeEvent(QCloseEvent* event)
{
	event->ignore();
	MainWindow::getInstance()->on_actionEvents_triggered(false);
}


int EventDockWidget::getIndex(QString name)
{
	for (int i = 0; i < entries.size(); i++)
	{
		if (entries[i].name == name)
		{
			return i;
		}
	}
	return -1;
}

void EventDockWidget::clear()
{
	for (int i = entries.size() - 1; i >= 0; i--)
	{
		deleteEvent(i);
	}
	PlotWindow::getInstance()->resetRange(true);
}

void EventDockWidget::populateActive()
{
	if (State::getInstance()->getActiveTrial() < 0 || State::getInstance()->getActiveTrial() >= Project::getInstance()->getTrials().size())
		return;

	for (auto e : Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEvents())
	{
		addEvent(e->getName(), e->getColor(), e->getDraw());
	}
	PlotWindow::getInstance()->resetRange(true);
	PlotWindow::getInstance()->draw();
}

void EventDockWidget::on_pushButtonAdd_clicked()
{
	bool ok;
	QString name = QInputDialog::getText(this, tr("Set event name"), tr("name:"), QLineEdit::Normal, "Event" + QString::number(entries.size() + 1), &ok);
	
	if (ok && !name.isEmpty()){
		for (int i = 0; i < entries.size(); i++)
		{
			if (entries[i].name == name)
			{
				ErrorDialog::getInstance()->showErrorDialog("Event with name " + name + " is already present. Please choose another name");
				return;
			}
		}

		Project::getInstance();
		addEvent(name, QColor::fromRgb(255, 0, 0),false);
		Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->addEvent(name, QColor::fromRgb(255, 0, 0));
		PlotWindow::getInstance()->resetRange(true);
		PlotWindow::getInstance()->draw();
	}
}

void EventDockWidget::on_pushButtonDelete_clicked()
{
	QStringList names;
	for (auto e : entries)
	{
		names << e.name;
	}
	
	bool ok;
	QString item = QInputDialog::getItem(this, tr("Choose event to delete"),
		tr("Event:"), names, 0, false, &ok);
	
	if (ok && !item.isEmpty())
	{
		int idx = getIndex(item);
		if (idx == -1)
			return;

		deleteEvent(idx);
		Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->removeEvent(idx);
		PlotWindow::getInstance()->resetRange(true);
		PlotWindow::getInstance()->draw();
	}
}

void EventDockWidget::on_pushButtonUnset_clicked()
{
	PlotWindow::getInstance()->setEventOff();
}

void EventDockWidget::on_pushButtonSet_clicked()
{
	PlotWindow::getInstance()->setEventOn();
}

void EventDockWidget::changeColor(const QString& name)
{
	int idx = getIndex(name);
	if (idx == -1)
		return;

	QColor color = QColorDialog::getColor(entries[idx].color, this);

	if (color.isValid()){
		entries[idx].color = color;

		QPixmap pix3(16, 16);
		pix3.fill(QColor(entries[idx].color));
		entries[idx].button->setIcon(pix3);
		Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEvents()[idx]->setColor(color);
		PlotWindow::getInstance()->draw();
	}
}

void EventDockWidget::checkbox_clicked(const QString& name)
{
	int idx = getIndex(name);
	if (idx == -1)
		return;

	Project::getInstance()->getTrials()[State::getInstance()->getActiveTrial()]->getEvents()[idx]->setDraw(entries[idx].checkbox->isChecked());
	PlotWindow::getInstance()->resetRange(true);
	PlotWindow::getInstance()->draw();
}

void EventDockWidget::workspaceChanged(work_state workspace)
{
	clear();
	if (workspace == DIGITIZATION)
	{
		populateActive();
	}
}

void EventDockWidget::activeTrialChanged(int)
{
	clear();
	populateActive();
}
