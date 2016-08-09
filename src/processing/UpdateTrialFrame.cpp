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
///\file UpdateTrialFrame.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "processing/UpdateTrialFrame.h" 

#include "core/Project.h"
#include "core/Trial.h"
#include "core/Marker.h"
#include "core/RigidBody.h"
using namespace xma;


UpdateTrialFrame::UpdateTrialFrame(Trial* trial, int frame) : ThreadedProcessing("Update Trial"), m_frame(frame)
{
	m_trial = trial;
}

UpdateTrialFrame::~UpdateTrialFrame()
{
}


void UpdateTrialFrame::process()
{
	for (unsigned int i = 0; i < m_trial->getMarkers().size(); i++)
	{
		if (m_trial->getMarkers()[i]->getRequiresRecomputation()){
			m_trial->getMarkers()[i]->reconstruct3DPoint(m_frame, true);
		}
		else
		{
			m_trial->getMarkers()[i]->reprojectPoint(m_frame);
		}
	}

	for (unsigned int i = 0; i < m_trial->getRigidBodies().size(); i++)
	{
		m_trial->getRigidBodies()[i]->computePose(m_frame);
	}
}

void UpdateTrialFrame::process_finished()
{
}

