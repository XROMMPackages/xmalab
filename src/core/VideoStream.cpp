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
///\file VideoStream.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "core/VideoStream.h"
#include "core/HelperFunctions.h"

#include <fstream>
#include <QXmlStreamReader>
#include "Project.h"
#include "Camera.h"

using namespace xma;

VideoStream::VideoStream(QStringList _filenames)
{
	filenames = _filenames;
	image = new Image("",false);
	nbImages = -1;
	fps = 0;

	Filename = "";
	FileID = -1;
	FileCategory = "";
	cameraNumber = "";
	Instrument = "";
	FrameRate = 0;
	ShutterSpeed = "";
	kV = "";
	mA = "";
	SID = "";
	MagLevel = "";
	Radiationtype = "";
	Pulsewidth = "";
	FileDescription = "";
	Lab = "";
	portalID = -1;
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

void VideoStream::parseXMLData(int id, QString xml_data)
{
	QString Filename_tmp;
	int FileID_tmp;
	QString FileCategory_tmp;
	QString cameraNumber_tmp;
	QString Instrument_tmp;
	double FrameRate_tmp;
	QString ShutterSpeed_tmp;
	QString kV_tmp;
	QString EDR_tmp;
	QString mA_tmp;
	QString SID_tmp;
	QString MagLevel_tmp;
	QString Radiationtype_tmp;
	QString Pulsewidth_tmp;
	QString FileDescription_tmp;
	QString Lab_tmp;

	int portal_id = Project::getInstance()->getCameras()[id]->getPortalId();
	if (portal_id == -1)
	{
		std::set <int> s;

		QXmlStreamReader xml2(xml_data);
		int count = 0;
		while (!xml2.atEnd() && !xml2.hasError())
		{
			QXmlStreamReader::TokenType token = xml2.readNext();

			if (token == QXmlStreamReader::StartDocument)
			{
				continue;
			}

			if (token == QXmlStreamReader::StartElement)
			{
				if (xml2.name() == "File")
				{
					while (!(xml2.tokenType() == QXmlStreamReader::EndElement && xml2.name() == "File"))
					{
						if (xml2.tokenType() == QXmlStreamReader::StartElement)
						{
							if (xml2.name() == "metadata")
							{
								while (!(xml2.tokenType() == QXmlStreamReader::EndElement && xml2.name() == "metadata"))
								{
									if (xml2.tokenType() == QXmlStreamReader::StartElement)
									{
										if (xml2.name() == "cameraNumber")
										{
											s.insert(xml2.readElementText().replace("cam", "").toInt());
										}
									}
									xml2.readNext();
								}
							}
						}
						xml2.readNext();
					}
				}
			}
		}

		if(!s.empty())portal_id = *std::next(s.begin(), id);
	}


	QXmlStreamReader xml(xml_data);
	int count = 0;
	while (!xml.atEnd() && !xml.hasError())
	{
		QXmlStreamReader::TokenType token = xml.readNext();

		if (token == QXmlStreamReader::StartDocument)
		{
			continue;
		}

		if (token == QXmlStreamReader::StartElement)
		{
			if (xml.name() == "File")
			{
				Filename_tmp = "";
				FileID_tmp = -1;
				FileCategory_tmp = "";
				cameraNumber_tmp = "";
				Instrument_tmp = "";
				FrameRate_tmp = 0;
				ShutterSpeed_tmp = "";
				kV_tmp = "";
				EDR_tmp = "";
				mA_tmp = "";
				SID_tmp = "";
				MagLevel_tmp = "";
				Radiationtype_tmp = "";
				Pulsewidth_tmp = "";
				FileDescription_tmp = "";
				Lab_tmp = "";
				
				while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "File"))
				{
					if (xml.tokenType() == QXmlStreamReader::StartElement)
					{
						if (xml.name() == "Filename")
						{
							Filename_tmp = xml.readElementText();
						}
						else if (xml.name() == "FileID")
						{
							FileID_tmp = xml.readElementText().toInt();
						}
						else if (xml.name() == "FileCategory")
						{
							FileCategory_tmp = xml.readElementText();
						}
						else if (xml.name() == "metadata")
						{
							while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "metadata"))
							{
								if (xml.tokenType() == QXmlStreamReader::StartElement)
								{
									if (xml.name() == "cameraNumber")
									{
										cameraNumber_tmp = xml.readElementText();
									}
									else if (xml.name() == "Instrument")
									{
										Instrument_tmp = xml.readElementText();
									}
									else if (xml.name() == "FrameRate")
									{
										FrameRate_tmp = xml.readElementText().toDouble();
									}
									else if (xml.name() == "ShutterSpeed")
									{
										ShutterSpeed_tmp = xml.readElementText();
									}
									else if (xml.name() == "kV")
									{
										kV_tmp = xml.readElementText();
									}
									else if (xml.name() == "EDR")
									{
										EDR_tmp = xml.readElementText();
									}
									else if (xml.name() == "mA")
									{
										mA_tmp = xml.readElementText();
									}
									else if (xml.name() == "SID")
									{
										SID_tmp = xml.readElementText();
									}
									else if (xml.name() == "MagLevel")
									{
										MagLevel_tmp = xml.readElementText();
									}
									else if (xml.name() == "Radiationtype")
									{
										Radiationtype_tmp = xml.readElementText();
									}
									else if (xml.name() == "Pulsewidth")
									{
										Pulsewidth_tmp = xml.readElementText();
									}
									else if (xml.name() == "FileDescription")
									{
										FileDescription_tmp = xml.readElementText();
									}
									else if (xml.name() == "Lab")
									{
										Lab_tmp = xml.readElementText();
									}
								}
								xml.readNext();
							}
						}
					}
					xml.readNext();
				}
				if (cameraNumber_tmp.replace("cam", "").toInt() == portal_id)
				{
					Filename = Filename_tmp;
					FileID = FileID_tmp;
					FileCategory = FileCategory_tmp;
					cameraNumber = cameraNumber_tmp;
					Instrument = Instrument_tmp;
					FrameRate = FrameRate_tmp;
					ShutterSpeed = ShutterSpeed_tmp;
					kV = kV_tmp;
					EDR = EDR_tmp;
					mA = mA_tmp;
					SID = SID_tmp;
					MagLevel = MagLevel_tmp;
					Radiationtype = Radiationtype_tmp;
					Pulsewidth = Pulsewidth_tmp;
					FileDescription = FileDescription_tmp;
					Lab = Lab_tmp;
					portalID = cameraNumber_tmp.replace("cam", "").toInt();
					return;
				}
				count++;
			}	
		}
	}
}

const QString& VideoStream::getFilename() const
{
	return Filename;
}

const int& VideoStream::getFileId() const
{
	return FileID;
}

const QString& VideoStream::getFileCategory() const
{
	return FileCategory;
}

const QString& VideoStream::getCameraNumber() const
{
	return cameraNumber;
}

const QString& VideoStream::getInstrument() const
{
	return Instrument;
}

const double& VideoStream::getFrameRate() const
{
	return FrameRate;
}

const QString& VideoStream::getShutterSpeed() const
{
	return ShutterSpeed;
}

const QString& VideoStream::getKV() const
{
	return kV;
}

const QString& VideoStream::getEDR() const
{
	return EDR;
}

const QString& VideoStream::getMA() const
{
	return mA;
}

const QString& VideoStream::getSid() const
{
	return SID;
}

const QString& VideoStream::getMagLevel() const
{
	return MagLevel;
}

const QString& VideoStream::getRadiationtype() const
{
	return Radiationtype;
}

const QString& VideoStream::getPulsewidth() const
{
	return Pulsewidth;
}

const QString& VideoStream::getFileDescription() const
{
	return FileDescription;
}

const QString& VideoStream::getLab() const
{
	return Lab;
}

const int& VideoStream::getPortalID() const
{
	return portalID;
}

void VideoStream::setFlipped(bool flipped)
{
	isFlipped = flipped;
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

