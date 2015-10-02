#ifndef THREADSCHEDULER_H_
#define THREADSCHEDULER_H_

#include <QObject>

namespace xma{
	class Trial; 

	class ThreadScheduler : public QObject{

		Q_OBJECT

	private:
		static ThreadScheduler* instance;

		void* data_ptr;

	protected:
		ThreadScheduler();


	public:
		virtual ~ThreadScheduler();
		static ThreadScheduler* getInstance();

		void updateTrialData(Trial * trial);


	public slots:
		void finalize_updateTrialData();


	};
}


#endif /* CONFIRMATIONDIALOG_H_ */
