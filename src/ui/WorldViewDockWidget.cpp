/*
 * ProgressDialog.cpp
 *
 *  Created on: Nov 19, 2013
 *      Author: ben
 */

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/WorldViewDockWidget.h"
#include <QtGui/QApplication>

WorldViewDockWidget::WorldViewDockWidget(QWidget *parent):QDockWidget(parent)
{
	setObjectName(QString::fromUtf8("WorldViewDockWidget"));
	openGL = new WorldViewDockGLWidget(this);
	setFeatures(QDockWidget::DockWidgetFloatable|QDockWidget::DockWidgetMovable);
	setAllowedAreas(Qt::AllDockWidgetAreas);
	setMinimumSize(0,0);
	this->resize(500,500);
    layout = new QGridLayout;
    layout->addWidget(openGL, 0, 0);
	setWidget(openGL);
	setWindowTitle(QApplication::translate("WorldViewDockWidget", "3D view", 0, QApplication::UnicodeUTF8));
    
}

void  WorldViewDockWidget::resizeEvent ( QResizeEvent * event )
{
	openGL->repaint();
}

void WorldViewDockWidget::setSharedGLContext(const QGLContext * sharedContext){
	QGLContext* context = new QGLContext(sharedContext->format(), openGL);
	context->create(sharedContext);
	openGL->setContext(context,sharedContext,true);
}

void WorldViewDockWidget::draw(){
	openGL->update();
}