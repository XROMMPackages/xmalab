#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "processing/ThreadScheduler.h"
#include "processing/UpdateTrialFrame.h"

#include "core/Trial.h"
#include "core/RigidBody.h"

#include <vector>
#include "ui/MainWindow.h"


using namespace xma;

ThreadScheduler* ThreadScheduler::instance = NULL;

ThreadScheduler::ThreadScheduler() {

}

ThreadScheduler::~ThreadScheduler(){
	instance = NULL;
}

ThreadScheduler* ThreadScheduler::getInstance()
{
	if(!instance) 
	{
		instance = new ThreadScheduler();
	}
	return instance;
}

void ThreadScheduler::updateTrialData(Trial * trial)
{
	data_ptr = trial;

	std::vector<UpdateTrialFrame*> threads;
		
	if (trial->getRequiresRecomputation()){
		for (int i = 0; i < trial->getNbImages(); i++)
		{
			UpdateTrialFrame * thread = new UpdateTrialFrame(trial, i);
			connect(thread, SIGNAL(signal_finished()), this, SLOT(finalize_updateTrialData()));
			threads.push_back(thread);
		}
	}

	for (int i = 0; i <threads.size(); i++)
	{
		threads[i]->start();
	}
}

void ThreadScheduler::finalize_updateTrialData()
{
	Trial * trial = (Trial *) data_ptr;
	for (int i = 0; i < trial->getRigidBodies().size(); i++){
		trial->getRigidBodies()[i]->makeRotationsContinous();
		trial->getRigidBodies()[i]->filterTransformations();
	}
	trial->setRequiresRecomputation(false);
	MainWindow::getInstance()->redrawGL();
}
