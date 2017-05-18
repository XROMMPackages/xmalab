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
///\file ProgressDialog.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/ProgressDialog.h"
#include "ui_ProgressDockWidget.h"
#include "ui/MainWindow.h"
#include <iostream>

using namespace xma;

ProgressDialog* ProgressDialog::instance = NULL;

ProgressDialog::ProgressDialog(QWidget* parent) :
	QDockWidget(parent),
	diag(new Ui::ProgressDockWidget)
{
	diag->setupUi(this);
	diag->progressBar->setValue(0.0);
#ifdef __APPLE__
	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(update()));
#endif
}

ProgressDialog::~ProgressDialog()
{
	delete diag;
	instance = NULL;
#ifdef __APPLE__
	timer->stop();
	delete timer;
#endif
}

ProgressDialog* ProgressDialog::getInstance()
{
	if (!instance)
	{
		instance = new ProgressDialog(MainWindow::getInstance());
		MainWindow::getInstance()->addDockWidget(Qt::BottomDockWidgetArea, instance);
	}
	return instance;
}

void ProgressDialog::setProgress(double progress)
{
	diag->progressBar->setTextVisible(true);
	diag->progressBar->setValue(progress);
	diag->progressBar->update();
	diag->progressBar->repaint();
	QApplication::processEvents();
}

void ProgressDialog::showProgressbar(int min_, int max_, const char* key)
{
	MainWindow::getInstance()->setEnabled(false);
	this->show();
#ifdef __APPLE__
	if (min_ == 0 && max_ == 0)
	{
		diag->progressBar->setMaximum(100);
		diag->progressBar->setMinimum(0);
		timer->start(100);
	}
	else{
		diag->progressBar->setMaximum(max_);
		diag->progressBar->setMinimum(min_);
	}
#else
	diag->progressBar->setMaximum(max_);
	diag->progressBar->setMinimum(min_);
#endif
	diag->progressBar->setTextVisible(false);
	setWindowTitle(QApplication::translate("ProgressDialog", key, 0, QApplication::UnicodeUTF8));
	QApplication::processEvents();
}

void ProgressDialog::closeProgressbar()
{
#ifdef __APPLE__
	timer->stop();
#endif
	MainWindow::getInstance()->setEnabled(true);
	this->close();
}

#ifdef __APPLE__
void ProgressDialog::update()
{
	int val = diag->progressBar->value() + 1;
	if (val > 100) val = 0;
	diag->progressBar->setValue(val);
}
#endif