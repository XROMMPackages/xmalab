//  ----------------------------------
//  XMA Lab -- Copyright © 2015, Brown University, Providence, RI.
//  
//  All Rights Reserved
//   
//  Use of the XMA Lab software is provided under the terms of the GNU General Public License version 3 
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
///\file VideoStream.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

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

	while (isValid && count < filenames.at(0).length())
	{
		QString prefix = filenames.at(0).left(count + 1);
		for (int i = 0; i < filenames.size(); i++)
		{
			if (!filenames.at(i).contains(prefix))
			{
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
	outfile.precision(12);
	for (int j = 0; j < filenames.size(); ++j)
	{
		outfile << filenames.at(j).toAscii().data() << std::endl;;
	}
	outfile.close();
}

