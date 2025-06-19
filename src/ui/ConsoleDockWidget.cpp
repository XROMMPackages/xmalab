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
///\file ConsoleDockWidget.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/ConsoleDockWidget.h"
#include "ui_ConsoleDockWidget.h"
#include "ui/MainWindow.h"

#include <QLabel>
#include <QColor>
#include <QFile>
#include <QTextStream>
#include <QScrollBar>
#include <QCloseEvent>
#include "ui/Shortcuts.h"

using namespace xma;

ConsoleDockWidget* ConsoleDockWidget::instance = nullptr;

ConsoleDockWidget::ConsoleDockWidget(QWidget* parent) :
	QDockWidget(parent),
	dock(new Ui::ConsoleDockWidget)
{
	dock->setupUi(this);

#ifdef _MSC_VER 
#ifndef WITH_CONSOLE
	memset(errorBuffer, 0, sizeof(errorBuffer));
	setvbuf(stderr, errorBuffer, _IOLBF, sizeof(errorBuffer));
	memset(outputBuffer, 0, sizeof(outputBuffer));
	setvbuf(stdout, outputBuffer, _IOLBF, sizeof(outputBuffer));
#endif
#else
	memset(errorBuffer, 0, sizeof(errorBuffer));  
	setvbuf(stderr, errorBuffer, _IOLBF, sizeof(errorBuffer));
	memset(outputBuffer, 0, sizeof(outputBuffer));
	setvbuf(stdout, outputBuffer, _IOLBF, sizeof(outputBuffer));
#endif

	// set up QTimer to call logErrors periodically
	timer = new QTimer(this);
	timer->start(500);
	connect(timer, SIGNAL(timeout()), this, SLOT(logTimer()));

	LoadText = "";

	Shortcuts::getInstance()->installEventFilterToChildren(this);
}


ConsoleDockWidget::~ConsoleDockWidget()
{
	delete dock;
	instance = nullptr;
}

void ConsoleDockWidget::clear()
{
	mutex.lock();
	dock->console->clear();
	dock->console->verticalScrollBar()->setValue(0);
	mutex.unlock();
}

void ConsoleDockWidget::prepareSave()
{
	mutex.lock();
	LoadText = dock->console->toHtml();
	mutex.unlock();
}

void ConsoleDockWidget::save(const QString& filename)
{
	QFile file(filename);
	if (file.open(QIODevice::ReadWrite))
	{
		QTextStream stream(&file);
		stream << LoadText;
		file.flush();
		file.close();
	}
}

void ConsoleDockWidget::afterLoad()
{
	mutex.lock();
	dock->console->clear();
	if (!LoadText.isEmpty())dock->console->setHtml(LoadText);
	dock->console->verticalScrollBar()->setValue(dock->console->verticalScrollBar()->maximum());
	mutex.unlock();
}

void ConsoleDockWidget::load(const QString& filename)
{
	QFile file(filename);
	if (file.exists())
	{
		file.open(QFile::ReadOnly | QFile::Text);

		QTextStream ReadFile(&file);
		LoadText = ReadFile.readAll();
		file.close();
	}
}

ConsoleDockWidget* ConsoleDockWidget::getInstance()
{
	if (!instance)
	{
		instance = new ConsoleDockWidget(MainWindow::getInstance());
		MainWindow::getInstance()->addDockWidget(Qt::BottomDockWidgetArea, instance);
	}
	return instance;
}

void ConsoleDockWidget::writeLog(const QString& message, unsigned int level)
{
	mutex.lock();
	switch (level)
	{
	default:
	case 0:
		dock->console->setTextColor(QColor::fromRgb(0, 0, 0));
		break;
	case 1:
		dock->console->setTextColor(QColor::fromRgb(0, 100, 0));
		break;
	case 2:
		dock->console->setTextColor(QColor::fromRgb(0, 0, 100));
		break;
	case 3:
		dock->console->setTextColor(QColor::fromRgb(100, 0, 0));
		break;
	}

	dock->console->append(message);
	dock->console->verticalScrollBar()->setValue(dock->console->verticalScrollBar()->maximum());
	mutex.unlock();
}

void ConsoleDockWidget::logTimer()
{
	fflush(stderr);

	// if there is stuff in the buffer, send it as errorMessage signal and clear the buffer
	if (strlen(errorBuffer) > 0)
	{
		QString tmp = errorBuffer;
		memset(errorBuffer, 0, sizeof(errorBuffer));
		writeLog(tmp, 3);
	}

	fflush(stdout);

	// if there is stuff in the buffer, send it as errorMessage signal and clear the buffer
	if (strlen(outputBuffer) > 0)
	{
		QString tmp = outputBuffer;
		memset(outputBuffer, 0, sizeof(outputBuffer));
		writeLog(tmp, 2);
	}
}


void ConsoleDockWidget::closeEvent(QCloseEvent* event)
{
	event->ignore();
	MainWindow::getInstance()->on_actionConsole_triggered(false);
}

