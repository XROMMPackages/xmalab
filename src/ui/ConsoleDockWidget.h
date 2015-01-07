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

namespace xma{
	class ConsoleDockWidget : public QDockWidget{

		Q_OBJECT

	public:
		virtual ~ConsoleDockWidget();
		static ConsoleDockWidget* getInstance();

		void writeLog(QString message, unsigned int level = 0);
		void save(QString file);
		void load(QString file);
		void clear();
		void afterLoad();
		void prepareSave();

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
}


#endif /* WIZARDUNDISTORTIONFRAME_H_ */

