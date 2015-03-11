#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "core/ImageSequence.h"
#include <QtCore/QFileInfo>

using namespace xma;

ImageSequence::ImageSequence(QStringList _filenames):VideoStream(_filenames){

	nbImages = filenames.size();

	image = new Image(filenames.at(0));
}

ImageSequence::~ImageSequence(){
	
}

void ImageSequence::setActiveFrame(int _activeFrame)
{
	image->setImage(filenames.at(_activeFrame));
}


QString ImageSequence::getFrameName(int frameNumber)
{
	QFileInfo info(filenames.at(frameNumber));
	return info.fileName();
}