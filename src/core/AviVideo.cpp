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
///\file AviVideo.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "core/AviVideo.h"
#include <QtCore/QFileInfo>
#include <opencv/highgui.h>
#include "Project.h"
using namespace xma;

AviVideo::AviVideo(QStringList _filenames) : VideoStream(_filenames)
{
	lastFrame = -1;
	reloadFile();
}

AviVideo::~AviVideo()
{
}

void AviVideo::setActiveFrame(int _activeFrame)
{
	if (lastFrame == _activeFrame)
		return;

	lastFrame = _activeFrame;
	cv::VideoCapture cap(filenames.at(0).toAscii().data());
	if (cap.isOpened() && _activeFrame >= 0 && _activeFrame < nbImages)
	{
		cap.set(CV_CAP_PROP_POS_FRAMES, _activeFrame);

		cv::Mat frame;
		cap.read(frame);
		if (frame.channels() > 1)
		{
			if (Project::getInstance()->getFlipImages())
				cv::flip(frame, frame, 1);
			image->setImage(frame, true);
		}
		else
		{
			if (Project::getInstance()->getFlipImages())
				cv::flip(frame, frame, 1);
			image->setImage(frame);
		}
		frame.release();
	}
	cap.release();
}


QString AviVideo::getFrameName(int frameNumber)
{
	QFileInfo info(filenames.at(0));
	return info.fileName() + " Frame " + QString::number(frameNumber + 1);
}

void AviVideo::reloadFile()
{
	cv::VideoCapture cap(filenames.at(0).toAscii().data()); // open the default camera

	if (cap.isOpened())
	{
		double frnb(cap.get(CV_CAP_PROP_FRAME_COUNT));
		nbImages = (int)(frnb + 0.45);
		fps = cap.get(CV_CAP_PROP_FPS);

		cv::Mat frame;
		cap.read(frame);
		if (frame.channels() > 1)
		{
			if (Project::getInstance()->getFlipImages())
				cv::flip(frame, frame, 1);
			image->setImage(frame, true);
		}
		else
		{
			if (Project::getInstance()->getFlipImages())
				cv::flip(frame, frame, 1);
			image->setImage(frame);
		}
		frame.release();
	}

	cap.release();
}

