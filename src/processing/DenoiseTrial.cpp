//  ----------------------------------
//  XMALab -- Copyright (c) 2015, Brown University, Providence, RI.
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
//  PROVIDED "AS IS", INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
//  FOR ANY PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR ANY 
//  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR FOR ANY DAMAGES WHATSOEVER RESULTING 
//  FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
//  OTHER TORTIOUS ACTION, OR ANY OTHER LEGAL THEORY, ARISING OUT OF OR IN CONNECTION 
//  WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
//  ----------------------------------
//  
///\file DenoiseTrial.cpp
///\author Benjamin Knorlein
///\date 10/06/2020

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "processing/DenoiseTrial.h" 

#include "core/Trial.h" 
#include "core/VideoStream.h" 
#include "core/Image.h" 
#include "core/ImageSequence.h" 
#include "core/HelperFunctions.h"

#include <QFileInfo>
#include <QDir>

#ifdef WIN32
	#define OS_SEP "\\"
#else
	#define OS_SEP "/"
#endif

using namespace xma;

QString DenoiseTrial::output_directory = "";

DenoiseTrial::DenoiseTrial(Trial * trial, int cameraID, bool relinkTrial, int searchWindowSize, int templateWindowSize, int temporalWindowSize, int filterStrength, QString outputFolder)
	: ThreadedProcessing("Denoise Trial"), m_trial{ trial }, m_cameraID{ cameraID }, m_relinkTrial{ relinkTrial }, m_searchWindowSize{ searchWindowSize }
	, m_templateWindowSize{ templateWindowSize }, m_temporalWindowSize{ temporalWindowSize }, m_filterStrength{ filterStrength }
{
	output_directory = outputFolder;
}

DenoiseTrial::~DenoiseTrial()
{

}

void DenoiseTrial::process()
{
 	std::vector <cv::Mat> images;

	for (int i = 0; i < m_trial->getNbImages(); i++) {
		cv::Mat image;
		m_trial->getVideoStreams()[m_cameraID]->setActiveFrame(i);
		m_trial->getVideoStreams()[m_cameraID]->getImage()->getImage(image, false);
		images.push_back(image);
	}

	QFileInfo info(m_trial->getVideoStreams()[m_cameraID]->getFileBasename());

	QDir().mkpath(output_directory + OS_SEP + "Cam" + QString::number(m_cameraID));
	int window_size = (m_temporalWindowSize - 1) / 2;

	for (int i = 0; i < m_trial->getNbImages(); i++) {
		std::cerr << "Processing " << m_cameraID << " " << i << std::endl;

		cv::Mat out_image;
		
		if (i < window_size || i > m_trial->getNbImages() - window_size) {
			out_image = images[i];
		}
		else {
			out_image = images[i];
			fastNlMeansDenoisingMulti(images, out_image, i, m_temporalWindowSize, m_filterStrength, m_templateWindowSize, m_searchWindowSize);
		}
		QString filename = output_directory + OS_SEP + "Cam" + QString::number(m_cameraID) + OS_SEP + info.completeBaseName() + "." + QString("%1").arg(i + 1, 4, 10, QChar('0')) + ".jpg";
		cv::imwrite(filename.toStdString(), out_image);
	}
}

void DenoiseTrial::process_finished()
{

}

