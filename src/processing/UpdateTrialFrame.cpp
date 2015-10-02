#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "processing/UpdateTrialFrame.h"	

#include "core/Project.h"
#include "core/Trial.h"
#include "core/Marker.h"
#include "core/RigidBody.h"
using namespace xma;


UpdateTrialFrame::UpdateTrialFrame(Trial *trial, int frame) :ThreadedProcessing("Update Trial"), m_frame(frame){
	m_trial = trial;
}

UpdateTrialFrame::~UpdateTrialFrame(){

}


void UpdateTrialFrame::process()
{
	for (int i = 0; i < m_trial->getMarkers().size(); i++){
		if (m_trial->getMarkers()[i]->getRequiresRecomputation())
			m_trial->getMarkers()[i]->reconstruct3DPoint(m_frame, true);
	}

	for (int i = 0; i < m_trial->getRigidBodies().size(); i++){
		m_trial->getRigidBodies()[i]->computePose(m_frame);
	}
}

void UpdateTrialFrame::process_finished()
{
	
}