#ifndef THREADEDPROCESSING_H
#define THREADEDPROCESSING_H

#include <QFutureWatcher>
#include <QObject>
#include <QString>

namespace xma{
	
	class ThreadedProcessing : public QObject{

		Q_OBJECT;

	public:
		ThreadedProcessing(QString _message = "");
		virtual ~ThreadedProcessing();
		void start();

		static bool isRunning(){
			return (nbInstances > 0);
		}

	signals:
		void signal_finished();

	public slots:
		void thread_complete();

	private:
		void thread();
		QFutureWatcher<void>* m_FutureWatcher;
		static int nbInstances;

		QString message;

	protected:
		virtual void process() = 0;
		virtual void process_finished() = 0;
	};
}
#endif  // MARKERDETECTION_H