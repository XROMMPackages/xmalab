#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "core/VideoStream.h"
#include "core/HelperFunctions.h"

#include <fstream>

using namespace xma;

VideoStream::VideoStream(QStringList _filenames)
{
	filenames = _filenames;
	image = new Image("");
	nbImages = -1;
	fps = 0;
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

double VideoStream::getFPS()
{
	return fps;
}

QString VideoStream::getFileBasename()
{
	bool isValid = true;
	int count = 0;
	//int max = (fileNames.size() > 20) ? 20 : fileNames.size();

	while (isValid && count < filenames.at(0).length()){
		QString prefix = filenames.at(0).left(count + 1);
		for (int i = 0; i <filenames.size(); i++){
			if (!filenames.at(i).contains(prefix)){
				isValid = false;
				break;
			}
		}

		if (isValid)count++;
	}
	return filenames.at(0).left(count);
}

void VideoStream::changeImagePath(QString newfolder, QString oldfolder)
{
	filenames = filenames.replaceInStrings("\\", OS_SEP);
	filenames = filenames.replaceInStrings("/", OS_SEP);
	filenames = filenames.replaceInStrings(oldfolder, newfolder);
	reloadFile();
}

void VideoStream::save(QString path)
{
	std::ofstream outfile(path.toAscii().data());
	for (unsigned int j = 0; j < filenames.size(); ++j){
		outfile << filenames.at(j).toAscii().data() << std::endl;;
	}
	outfile.close();
}
