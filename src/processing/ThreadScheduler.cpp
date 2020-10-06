//  ----------------------------------
//  XMALab -- Copyright © 2015, Brown University, Providence, RI.
//  
//  All Rights Reserved
//   
//  Use of the XMALab software is provided under the terms of the GNU General Public License version 3 
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
///\file ThreadScheduler.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

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

ThreadScheduler::ThreadScheduler()
{
	data_ptr = NULL;
	running = false;
}

ThreadScheduler::~ThreadScheduler()
{
	instance = NULL;
}

ThreadScheduler* ThreadScheduler::getInstance()
{
	if (!instance)
	{
		instance = new ThreadScheduler();
	}
	return instance;
}

void ThreadScheduler::updateTrialData(Trial* trial)
{
	running = true;

	trials.push_back(trial);
	
	std::vector<UpdateTrialFrame*> threads;

	if (trial->getRequiresRecomputation())
	{
		//ensure that the vector sizes of the rigid bodies is of the length of nbImages
		for (auto rb : trial->getRigidBodies())
		{
			if (rb->getPoseComputed().size() != trial->getNbImages())
			{
				rb->init(trial->getNbImages());
			}
		}

		for (int i = 0; i < trial->getNbImages(); i++)
		{
			UpdateTrialFrame* thread = new UpdateTrialFrame(trial, i);
			connect(thread, SIGNAL(signal_finished()), this, SLOT(finalize_updateTrialData()));
			threads.push_back(thread);
		}
	}

	for (unsigned int i = 0; i < threads.size(); i++)
	{
		threads[i]->start();
	}
}

void ThreadScheduler::finalize_updateTrialData()
{
	for (auto & trial : trials) {
		for (unsigned int i = 0; i < trial->getRigidBodies().size(); i++)
		{
			trial->getRigidBodies()[i]->makeRotationsContinous();
			trial->getRigidBodies()[i]->filterTransformations();
		}
		trial->setRequiresRecomputation(false);
	}
	trials.clear();
	running = false;
	MainWindow::getInstance()->redrawGL();
}

