#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/WorldViewDockWidget.h"
#include "ui/MainWindow.h"

#include <QtGui/QApplication>
#include <QtGui/QCloseEvent>

using namespace xma;

WorldViewDockWidget::WorldViewDockWidget(QWidget *parent):QDockWidget(parent)
{
	setObjectName(QString::fromUtf8("WorldViewDockWidget"));
	openGL = new WorldViewDockGLWidget(this);
	setFeatures( DockWidgetFloatable| DockWidgetMovable | DockWidgetClosable);
	setAllowedAreas(Qt::AllDockWidgetAreas);
	resize(500, 500);
    layout = new QGridLayout;
    layout->addWidget(openGL, 0, 0);
	setWidget(openGL);
	setWindowTitle(QApplication::translate("WorldViewDockWidget", "External view", 0, QApplication::UnicodeUTF8));
	
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

void WorldViewDockWidget::closeEvent(QCloseEvent *event)
{
	event->ignore();
	MainWindow::getInstance()->on_action3D_world_view_triggered(false);
}