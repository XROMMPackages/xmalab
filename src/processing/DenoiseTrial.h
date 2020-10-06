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
///\file DenoiseTrial.h
///\author Benjamin Knorlein
///\date 10/06/2020

#ifndef DENOISETRIAL_H
#define DENOISETRIAL_H


#include "processing/ThreadedProcessing.h"

namespace xma
{
	class Trial;

	class DenoiseTrial : public ThreadedProcessing
	{
		Q_OBJECT;

	public:
		DenoiseTrial(Trial * trial, int cameraID,bool relinkTrial,int searchWindowSize,int templateWindowSize,int temporalWindowSize,int filterStrength,QString outputFolder);
		virtual ~DenoiseTrial();

		static QString output_directory;
	protected:
		void process() override;
		void process_finished() override;

	private:

		Trial * m_trial;
		int m_cameraID;

		bool m_relinkTrial;
		int m_searchWindowSize;
		int m_templateWindowSize;
		int m_temporalWindowSize;
		int m_filterStrength;
	};
}
#endif // MARKERTRACKING_H


