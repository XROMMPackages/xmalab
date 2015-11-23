//  ----------------------------------
//  XMA Lab -- Copyright © 2015, Brown University, Providence, RI.
//  
//  All Rights Reserved
//   
//  Use of the XMA Lab software is provided under the terms of the GNU General Public License version 3 
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
#include "ui/MainWindow.h"
#include "ui/Shortcuts.h"

#include <QtGui/QApplication>
#include <QtGui/QCloseEvent>

using namespace xma;

WorldViewDockWidget::WorldViewDockWidget(QWidget* parent): QDockWidget(parent)
{
	setObjectName(QString::fromUtf8("WorldViewDockWidget"));
	openGL = new WorldViewDockGLWidget(this);
	setFeatures(DockWidgetFloatable | DockWidgetMovable | DockWidgetClosable);
	setAllowedAreas(Qt::AllDockWidgetAreas);
	resize(500, 500);
	layout = new QGridLayout;
	layout->addWidget(openGL, 0, 0);
	setWidget(openGL);
	setWindowTitle(QApplication::translate("WorldViewDockWidget", "External view", 0, QApplication::UnicodeUTF8));

	Shortcuts::getInstance()->installEventFilterToChildren(this);
}

void WorldViewDockWidget::resizeEvent(QResizeEvent* event)
{
	openGL->repaint();
}

void WorldViewDockWidget::setSharedGLContext(const QGLContext* sharedContext)
{
	QGLContext* context = new QGLContext(sharedContext->format(), openGL);
	context->create(sharedContext);
	openGL->setContext(context, sharedContext, true);
}

void WorldViewDockWidget::draw()
{
	openGL->update();
}

void WorldViewDockWidget::closeEvent(QCloseEvent* event)
{
	event->ignore();
	MainWindow::getInstance()->on_action3D_world_view_triggered(false);
}

