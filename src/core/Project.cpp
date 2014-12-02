#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "core/Project.h"
#include "core/Camera.h"
#include "core/CalibrationObject.h"
#include "core/CalibrationImage.h"
#include "core/UndistortionObject.h"

#include <QApplication>
#include <QFileInfo>

#ifdef WIN32
#define OS_SEP "\\"
#else
#define OS_SEP "/"
#endif

Project* Project::instance = NULL;

Project::Project(){
	projectFilename = "";
	valid = false;
	nbImages = 0;
}

Project::~Project(){
	for(std::vector <Camera*>::iterator it = cameras.begin(); it != cameras.end(); ++it)
		delete (*it);
	cameras.clear();

	delete CalibrationObject::getInstance();

	instance = NULL;
}

Project* Project::getInstance()
{
	if(!instance) 
	{
		instance = new Project();
	}
	return instance;
}

QString Project::getProjectBasename(){
	if(projectFilename.isEmpty())
		return "";

	QFileInfo info(projectFilename);
	return info.completeBaseName(); 
}

void Project::addCamera(Camera * cam){
	cameras.push_back(cam);
	nbImages = cam->getCalibrationImages().size();
}

void Project::recountFrames(){
	if(cameras.size() > 0){
		nbImages = cameras[0]->getCalibrationImages().size();
		for(std::vector <Camera*>::iterator it = cameras.begin(); it != cameras.end(); ++it){	
			if(nbImages != (*it)->getCalibrationImages().size()){
				fprintf(stderr,"Error Invalid number of images");
			}
		}
	}
}

void Project::loadTextures(){
	for(std::vector <Camera*>::iterator it = cameras.begin(); it != cameras.end(); ++it){
		(*it)->loadTextures();
		QApplication::processEvents();
	}
}

void Project::exportDLT(QString foldername){
	for(int frame = 0 ; frame < nbImages ; frame++){
		std::vector<double*> dlts;
		double allCamsSet = true;

		for(std::vector <Camera*>::iterator it = cameras.begin(); it != cameras.end(); ++it){	
			if((*it)->getCalibrationImages()[frame]->isCalibrated()){
				double* out = new double[12];
				(*it)->getDLT(&out[0],frame);
				dlts.push_back(out);
			}else{
				allCamsSet = false;
			}
		}

		if(allCamsSet){
			std::ofstream outfile ((foldername + OS_SEP + "MergedDlts_Frame" + QString::number(frame) +".csv").toAscii().data() );
			for(unsigned int i = 0; i < 11 ; i++){
				for(unsigned int c = 0; c < dlts.size() ; c++){	
					outfile << dlts[c][i];
					if(c == dlts.size()-1) {
						outfile << std::endl;
					}else{
						outfile << ",";
					}
				}
			}
			outfile.close();
		}

		for(unsigned int c = 0; c < dlts.size() ; c++){	
			delete[] dlts[c];
		}
		dlts.clear();
	}
}

void Project::exportMayaCam(QString foldername){
	for(std::vector <Camera*>::iterator it = cameras.begin(); it != cameras.end(); ++it){
		for(int frame = 0 ; frame < (*it)->getCalibrationImages().size();frame++){
			if((*it)->getCalibrationImages()[frame]->isCalibrated()){
				double out[15];
				(*it)->getMayaCam(&out[0],frame);
				std::ofstream outfile ((foldername + OS_SEP + (*it)->getCalibrationImages()[frame]->getFilenameBase() + "_MayaCam.csv").toAscii().data() );
				for(unsigned int i = 0; i < 5 ; i++){
					outfile << out[i*3] << "," << out[i*3+1] << "," << out[i*3+2] << "\n";
				}
				outfile.close();
			}
		}
	}
}

void Project::exportLUT(QString foldername){
	for(std::vector <Camera*>::iterator it = cameras.begin(); it != cameras.end(); ++it){
		if((*it)->hasUndistortion() && (*it)->getUndistortionObject()->isComputed()){
			(*it)->getUndistortionObject()->exportData(foldername + OS_SEP + (*it)->getUndistortionObject()->getFilenameBase() + "_LUT.csv",
														 foldername + OS_SEP + (*it)->getUndistortionObject()->getFilenameBase() + "_INPTS.csv",
														 foldername + OS_SEP + (*it)->getUndistortionObject()->getFilenameBase() + "_BSPTS.csv");
		}
	}
}