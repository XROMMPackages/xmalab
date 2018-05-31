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
///\file CalibrationObject.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "core/CalibrationObject.h"
#include "core/HelperFunctions.h"
#include "core/Settings.h"

#include <QFileInfo>
#include <QStringList>

using namespace xma;

CalibrationObject* CalibrationObject::instance = NULL;

CalibrationObject::CalibrationObject()
{
	planar = false;
	checkerboard = false;
	initialised = false;
	whiteBlobs = false;
	scale.x = 1.0;
	scale.y = 1.0;
	scale.z = 1.0;
	nbHorizontalSquares = 0;
	nbVerticalSquares = 0;
}

CalibrationObject::~CalibrationObject()
{
	instance = NULL;
}

CalibrationObject* CalibrationObject::getInstance()
{
	if (!instance)
	{
		instance = new CalibrationObject();
	}
	return instance;
}

int CalibrationObject::loadCoords(QString pointsfilename, QString references)
{
	frameSpecificationsFilename = pointsfilename;
	referencesFilename = references;

	planar = false;
	frameSpecifications.clear();
	referenceIDs.clear();
	referenceNames.clear();

	std::ifstream fin;
	fin.open(pointsfilename.toAscii().data());
	std::istringstream in;
	std::string line;
	//read first line 
	littleHelper::safeGetline(fin, line);
	QString header = QString::fromStdString(line);
	//Check blobcolor
	if (header.contains("white"))
	{
		header.replace("white", "");
		whiteBlobs = true;
	}
	//checkScale
	QStringList headerlist = header.split(",");
	if (headerlist.size() > 3)
	{
		if (headerlist.at(0).startsWith("s") &&
			headerlist.at(1).startsWith("s") &&
			headerlist.at(2).startsWith("s"))
		{
			QString tmp = headerlist.at(0);
			tmp.replace("s", "");
			scale.x = tmp.toDouble();

			tmp = headerlist.at(1);
			tmp.replace("s", "");
			scale.y = tmp.toDouble();

			tmp = headerlist.at(2);
			tmp.replace("s", "");
			scale.z = tmp.toDouble();
		}
	}

	while (!littleHelper::safeGetline(fin, line).eof())
	{
		in.clear();
		in.str(line);
		std::vector<double> tmp;
		for (double value; in >> value; littleHelper::comma(in))
		{
			tmp.push_back(value);
		}
		if (tmp.size() > 0) frameSpecifications.push_back(cv::Point3d(tmp[0] * scale.x, tmp[1] * scale.y, tmp[2] * scale.z));
		line.clear();
	}
	fin.close();
	if (!line.empty())
	{
		in.clear();
		in.str(line);
		std::vector<double> tmp;
		for (double value; in >> value; littleHelper::comma(in))
		{
			tmp.push_back(value);
		}
		if (tmp.size() > 0) frameSpecifications.push_back(cv::Point3d(tmp[0] * scale.x, tmp[1] * scale.y, tmp[2] * scale.z));
		line.clear();
	}


	fin.open(references.toAscii().data(), std::ios::binary);

	char str[100];
	int id;

	while (!littleHelper::safeGetline(fin, line).eof())
	{
		if (sscanf(line.c_str(), "%i %20[0-9a-zA-Z ]s", &id, &str[0]) == 2)
		{
			referenceIDs.push_back(id - 1);
			referenceNames.push_back(str);
		}
	}
	fin.close();

	//check Planar
	bool xplanar = true;
	bool yplanar = true;
	bool zplanar = true;

	for (unsigned int i = 0; i < frameSpecifications.size(); i++)
	{
		if (frameSpecifications[i].x != 0) xplanar = false;
		if (frameSpecifications[i].y != 0) yplanar = false;
		if (frameSpecifications[i].z != 0) zplanar = false;
	}

	planar = planar || xplanar || zplanar || yplanar;

	if (planar) std::cerr << "Planar" << std::endl;

	initialised = true;
	return frameSpecifications.size();
}


void CalibrationObject::saveCoords(QString folder)
{
	QFileInfo frameSpecificationsFilenameInfo(frameSpecificationsFilename);
	QString cubefilename = folder + frameSpecificationsFilenameInfo.fileName();
	QFileInfo referencesFilenameInfo(referencesFilename);
	QString references = folder + referencesFilenameInfo.fileName();

	std::ofstream outfile(cubefilename.toAscii().data());
	outfile.precision(12);
	if (whiteBlobs)
	{
		outfile << "white" << std::endl;
	}
	else
	{
		outfile << "x y z" << std::endl;
	}
	for (unsigned int i = 0; i < frameSpecifications.size(); i++)
	{
		outfile << frameSpecifications[i].x << " , " << frameSpecifications[i].y << " , " << frameSpecifications[i].z << std::endl;
	}
	outfile.close();

	std::ofstream outfile_references(references.toAscii().data());
	outfile.precision(12);
	for (unsigned int i = 0; i < referenceIDs.size(); i++)
	{
		outfile_references << referenceIDs[i] + 1 << " " << referenceNames[i].toAscii().data() << std::endl;
	}
	outfile_references.close();
}

void CalibrationObject::setCheckerboard(int _nbHorizontalSquares, int _nbVerticalSquares, double _squareSize)
{
	nbHorizontalSquares = _nbHorizontalSquares;
	nbVerticalSquares = _nbVerticalSquares;
	squareSize = _squareSize;
	planar = true;
	checkerboard = true;

	referenceIDs.clear();
	referenceNames.clear();
	frameSpecifications.clear();
	int inv_x = (Settings::getInstance()->getIntSetting("CheckerboadInvertedAxis") == 1) ? -1 : 1;
	int inv_y = (Settings::getInstance()->getIntSetting("CheckerboadInvertedAxis") == 2) ? -1 : 1;

	//for (int y = nbVerticalSquares - 1; y >=0 ; y--)
	for (int y = 0; y < nbVerticalSquares; y++)
	{
		for (int x = 0; x < _nbHorizontalSquares; x++)
		{
			frameSpecifications.push_back(cv::Point3d(inv_x * x * squareSize, inv_y * y * squareSize, 0));
		}
	}


	initialised = true;
}

