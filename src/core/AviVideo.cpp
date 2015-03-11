#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "core/AviVideo.h"
#include <QtCore/QFileInfo>
#include <opencv/highgui.h>
using namespace xma;

AviVideo::AviVideo(QStringList _filenames) :VideoStream(_filenames){
	nbImages = 0;
	cv::VideoCapture cap(filenames.at(0).toAscii().data()); // open the default camera
	if (cap.isOpened())
	{
		double frnb(cap.get(CV_CAP_PROP_FRAME_COUNT));
		nbImages = (int) (frnb+0.45);

		cap.set(CV_CAP_PROP_POS_FRAMES, 0);
		cv::Mat frame;
		cap.read(frame);
		if (frame.channels() > 1)
		{
			cv::Mat grayimage;
			cv::cvtColor(frame, grayimage, CV_BGR2GRAY);
			image->setImage(grayimage);
			grayimage.release();
		}
		else
		{
			image->setImage(frame);
		}
		frame.release();
	}

	cap.release();
}

AviVideo::~AviVideo(){
	
}

void AviVideo::setActiveFrame(int _activeFrame)
{
	cv::VideoCapture cap(filenames.at(0).toAscii().data());
	if (cap.isOpened() && _activeFrame >= 0 && _activeFrame < nbImages)
	{
		cap.set(CV_CAP_PROP_POS_FRAMES, _activeFrame);
		cv::Mat frame;
		cap.read(frame);
		if (frame.channels() > 1)
		{
			cv::Mat grayimage;
			cv::cvtColor(frame, grayimage, CV_BGR2GRAY);
			image->setImage(grayimage);
			grayimage.release();
		}
		else
		{
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

		cap.set(CV_CAP_PROP_POS_FRAMES, 0);
		cv::Mat frame;
		cap.read(frame);
		if (frame.channels() > 1)
		{
			cv::Mat grayimage;
			cv::cvtColor(frame, grayimage, CV_BGR2GRAY);
			image->setImage(grayimage);
			grayimage.release();
		}
		else
		{
			image->setImage(frame);
		}
		frame.release();
	}

	cap.release();
}