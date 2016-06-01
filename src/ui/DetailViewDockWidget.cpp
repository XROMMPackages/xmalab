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
///\file DetailViewDockWidget.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "core/Project.h"

#include "ui/MainWindow.h"
#include "ui/DetailViewDockWidget.h"
#include "ui_DetailViewDockWidget.h"
#include "ui/CameraViewDetailWidget.h"
#include "ui/Shortcuts.h"
#include "ui/GLSharedWidget.h"

#include <QLabel>
#include <QCloseEvent>


using namespace xma;

DetailViewDockWidget* DetailViewDockWidget::instance = NULL;

DetailViewDockWidget::DetailViewDockWidget(QWidget* parent) :
	QDockWidget(parent),
	dock(new Ui::DetailViewDockWidget)
{
	dock->setupUi(this);

	Shortcuts::getInstance()->installEventFilterToChildren(this);
}

DetailViewDockWidget::~DetailViewDockWidget()
{
	clear();
	delete dock;
	instance = NULL;
}

DetailViewDockWidget* DetailViewDockWidget::getInstance()
{
	if (!instance)
	{
		instance = new DetailViewDockWidget(MainWindow::getInstance());
		MainWindow::getInstance()->addDockWidget(Qt::TopDockWidgetArea, instance);
	}
	return instance;
}


void DetailViewDockWidget::draw()
{
	if (this->isVisible())
	{
		for (unsigned int i = 0; i < cameraViews.size(); i++)
		{
			cameraViews[i]->draw();
		}
	}
}

void DetailViewDockWidget::setup()
{
	for (std::vector<Camera*>::const_iterator it = Project::getInstance()->getCameras().begin(); it != Project::getInstance()->getCameras().end(); ++it)
	{
		CameraViewDetailWidget* cam_widget = new CameraViewDetailWidget((*it), this);
		cam_widget->setSharedGLContext(GLSharedWidget::getInstance()->getQGLContext());
		cameraViews.push_back(cam_widget);
	}

	for (unsigned int i = 0; i < cameraViews.size(); i++)
	{
		dock->horizontalLayout->addWidget(cameraViews[i]);
	}

	Shortcuts::getInstance()->installEventFilterToChildren(this);
}

void DetailViewDockWidget::clear()
{
	for (unsigned int i = 0; i < cameraViews.size(); i++)
	{
		dock->horizontalLayout->removeWidget(cameraViews[i]);
	}

	for (unsigned int i = 0; i < cameraViews.size(); i++)
	{
		delete cameraViews[i];
	}
	cameraViews.clear();
}

void DetailViewDockWidget::centerViews()
{
	for (unsigned int i = 0; i < cameraViews.size(); i++)
	{
		cameraViews[i]->centerViews();
	}
}

void DetailViewDockWidget::closeEvent(QCloseEvent* event)
{
	event->ignore();
	MainWindow::getInstance()->on_actionDetailed_View_triggered(false);
}

