/*
 * ProgressDialog.h
 *
 *  Created on: Nov 19, 2013
 *      Author: ben
 */

#ifndef WORLDVIEWDOCKWIDGET_H_
#define WORLDVIEWDOCKWIDGET_H_

#include "ui/WorldViewDockGLWidget.h"
#include <QGridLayout>
#include <QDockWidget>

class WorldViewDockWidget : public QDockWidget
{
    Q_OBJECT

public:
    WorldViewDockWidget(QWidget *parent);
	WorldViewDockGLWidget *openGL;

	void setSharedGLContext(const QGLContext * sharedContext);
	void draw();
	
private:
	QGridLayout *layout;
protected:
	void  resizeEvent ( QResizeEvent * event );
};



#endif /* PROGRESSDIALOG_H_ */
