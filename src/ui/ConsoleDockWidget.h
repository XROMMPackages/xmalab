/*
 * ProgressDialog.h
 *
 *  Created on: Nov 19, 2013
 *      Author: ben
 */

#ifndef CONSOLEDOCKWIDGET_H_
#define CONSOLEDOCKWIDGET_H_

#include <QDockWidget>
#include <QMutex>
#include "ui/State.h"


#define BUFFERSIZE 655360

namespace Ui {
	class ConsoleDockWidget;
}

class QLabel;

class ConsoleDockWidget : public QDockWidget{

	Q_OBJECT
	
	public:
		~ConsoleDockWidget();
		static ConsoleDockWidget* getInstance();

		void ConsoleDockWidget::writeLog(QString message,unsigned int level = 0);
		void save(QString file);
		void load(QString file);
		void clear();
		void afterLoad();

	private:
		Ui::ConsoleDockWidget *dock;
		static ConsoleDockWidget* instance;
		ConsoleDockWidget(QWidget *parent = 0);
		QMutex mutex;

		char errorBuffer[BUFFERSIZE];
		char outputBuffer[BUFFERSIZE];

		QString LoadText;

		QTimer *timer;
	public slots:
		void logTimer();     
};



#endif /* WIZARDUNDISTORTIONFRAME_H_ */

