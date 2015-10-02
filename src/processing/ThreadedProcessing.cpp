#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "processing/ThreadedProcessing.h"	

#include "ui/ProgressDialog.h"

#include <QtCore>

using namespace xma;

int ThreadedProcessing::nbInstances = 0;

ThreadedProcessing::ThreadedProcessing(QString _message) :QObject(){
	nbInstances++;

	message = _message;
}

ThreadedProcessing::~ThreadedProcessing(){

}

void ThreadedProcessing::start(){
	m_FutureWatcher = new QFutureWatcher<void>();
	connect(m_FutureWatcher, SIGNAL(finished()), this, SLOT(thread_complete()));

	QFuture<void> future = QtConcurrent::run(this, &ThreadedProcessing::thread);
	m_FutureWatcher->setFuture( future );

	if (!message.isEmpty()) ProgressDialog::getInstance()->showProgressbar(0, 0, message.toAscii().data());
}

void ThreadedProcessing::thread(){
	process();
}

void ThreadedProcessing::thread_complete(){

	process_finished();

	delete m_FutureWatcher;
	nbInstances--;
	
	if(nbInstances == 0){
		if (!message.isEmpty())ProgressDialog::getInstance()->closeProgressbar();
		emit signal_finished();
		
	}
	delete this;
}
