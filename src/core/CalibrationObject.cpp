#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "core/CalibrationObject.h"
#include "core/HelperFunctions.h"

#include <QFileInfo>

using namespace xma;

CalibrationObject* CalibrationObject::instance = NULL;

CalibrationObject::CalibrationObject()
{
	planar = false;
	initialised = false;
}

CalibrationObject::~CalibrationObject()
{
	instance = NULL;
}

CalibrationObject* CalibrationObject::getInstance()
{
	if(!instance) 
	{
		instance = new CalibrationObject();
	}
	return instance;
}

int CalibrationObject::loadCoords(QString pointsfilename , QString references)
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
	while (!littleHelper::safeGetline(fin, line).eof())
    {
        in.clear();
        in.str(line);
        std::vector<double> tmp;
        for (double value; in >> value; littleHelper::comma(in)) {
            tmp.push_back(value);
        }
		if(tmp.size()>0) frameSpecifications.push_back(cv::Point3d(tmp[0],tmp[1],tmp[2]));
		line.clear();
    }
	fin.close();
	if(!line.empty()){
		in.clear();
        in.str(line);
        std::vector<double> tmp;
		for (double value; in >> value; littleHelper::comma(in)) {
            tmp.push_back(value);
        }
		if(tmp.size()>0) frameSpecifications.push_back(cv::Point3d(tmp[0],tmp[1],tmp[2]));
		line.clear();
	}


	fin.open(references.toAscii().data(), std::ios::binary);
		
	char str[100];
	int id;

	while (!littleHelper::safeGetline(fin, line).eof())
	{
		if(sscanf (line.c_str(),"%i %20[0-9a-zA-Z ]s",&id, &str[0]) == 2){
			referenceIDs.push_back(id-1);
			referenceNames.push_back(str);
		}
	}
	fin.close();
	
	initialised = true;
	return frameSpecifications.size();
}



void CalibrationObject::saveCoords(QString folder)
{
	QFileInfo frameSpecificationsFilenameInfo(frameSpecificationsFilename);
	QString cubefilename = folder + frameSpecificationsFilenameInfo.fileName();
	QFileInfo referencesFilenameInfo(referencesFilename);
	QString references = folder + referencesFilenameInfo.fileName();

	std::ofstream outfile (cubefilename.toAscii().data());
	outfile << "x y z" << std::endl;
	for(unsigned int i = 0; i < frameSpecifications.size() ; i++){
		outfile <<  frameSpecifications[i].x << " , " << frameSpecifications[i].y << " , " << frameSpecifications[i].z << std::endl;
	}
	outfile.close();

	std::ofstream outfile_references (references.toAscii().data());
	for(unsigned int i = 0; i < referenceIDs.size() ; i++){
		outfile_references <<  referenceIDs[i]+1 << " " << referenceNames[i].toAscii().data() << std::endl;
	}
	outfile_references.close();
}

void CalibrationObject::setCheckerboard(int _nbHorizontalSquares, int _nbVerticalSquares, int _squareSize)
{
	nbHorizontalSquares = _nbHorizontalSquares;
	nbVerticalSquares = _nbVerticalSquares;
    squareSize = _squareSize;

	planar = true;
	frameSpecifications.clear();
	referenceIDs.clear();
	referenceNames.clear();



	initialised = true;
}