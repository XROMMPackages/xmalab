#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "core/VideoStream.h"
#include <fstream>

using namespace xma;

VideoStream::VideoStream(QStringList _filenames)
{
	filenames = _filenames;
}

VideoStream::~VideoStream()
{
	delete image;
}


QStringList VideoStream::getFilenames()
{
	return filenames;
}

int VideoStream::getNbImages()
{
	return nbImages;
}

Image* VideoStream::getImage()
{
	return image;
}

void VideoStream::bindTexture()
{
	image->bindTexture();
}



void VideoStream::changeImagePath(QString newfolder, QString oldfolder)
{
	filenames = filenames.replaceInStrings(oldfolder, newfolder);
}

void VideoStream::save(QString path)
{
	std::ofstream outfile(path.toAscii().data());
	for (unsigned int j = 0; j < filenames.size(); ++j){
		outfile << filenames.at(j).toAscii().data() << std::endl;;
	}
	outfile.close();
}
