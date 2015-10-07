#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "core/Project.h"
#include "core/Camera.h"
#include "core/Trial.h"
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

using namespace xma;

Project* Project::instance = NULL;

Project::Project(){
	projectFilename = "";
	calibrated = false;
	nbImagesCalibration = 0;
}

Project::~Project(){
	for(std::vector <Camera*>::iterator it = cameras.begin(); it != cameras.end(); ++it)
		delete (*it);
	cameras.clear();

	for (std::vector <Trial*>::iterator it = trials.begin(); it != trials.end(); ++it)
		delete (*it);
	trials.clear();

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

QString Project::getProjectFilename()
{
	return projectFilename;
}

const std::vector<Camera*>& Project::getCameras()
{
	return cameras;
}

const std::vector<Trial*>& Project::getTrials()
{
	return trials;
}

Trial* Project::getTrialByName(QString Name)
{
	for (std::vector <Trial*>::iterator it = trials.begin(); it != trials.end(); ++it)
	{
		if ((*it)->getName() == Name) return *it;
	}
	return NULL;
}

QString Project::getProjectBasename(){
	if(projectFilename.isEmpty())
		return "";

	QFileInfo info(projectFilename);
	return info.completeBaseName(); 
}

int Project::getNbImagesCalibration()
{
	return nbImagesCalibration;
}

bool Project::isCalibrated()
{
	return calibrated;
}

void Project::checkCalibration()
{
	calibrated = true;
	for (std::vector <Camera*>::iterator it = cameras.begin(); it != cameras.end(); ++it)
		calibrated = (*it)->isCalibrated() && calibrated;
}

void Project::addCamera(Camera * cam){
	cameras.push_back(cam);
	nbImagesCalibration = cam->getCalibrationImages().size();
}

void Project::addTrial(Trial* trial)
{
	trials.push_back(trial);
}

void Project::deleteTrial(Trial* trial)
{
	std::vector<Trial *>::iterator position = std::find(trials.begin(), trials.end(), trial);
	delete *position;
	trials.erase(position);
}

void Project::recountFrames(){
	if(cameras.size() > 0){
		nbImagesCalibration = cameras[0]->getCalibrationImages().size();
		for(std::vector <Camera*>::iterator it = cameras.begin(); it != cameras.end(); ++it){	
			if(nbImagesCalibration != (*it)->getCalibrationImages().size()){
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
	for(int frame = 0 ; frame < nbImagesCalibration ; frame++){
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
			outfile.precision(12);
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
				outfile.precision(12);
				for(unsigned int i = 0; i < 5 ; i++){
					outfile << out[i*3] << "," << out[i*3+1] << "," << out[i*3+2] << "\n";
				}
				outfile.close();
			}
		}
	}
}

void Project::exportMayaCamVersion2(QString foldername){
	for (std::vector <Camera*>::iterator it = cameras.begin(); it != cameras.end(); ++it){
		for (int frame = 0; frame < (*it)->getCalibrationImages().size(); frame++){
			if ((*it)->getCalibrationImages()[frame]->isCalibrated()){
				(*it)->saveMayaCamVersion2(frame, foldername + OS_SEP + (*it)->getCalibrationImages()[frame]->getFilenameBase() + "_MayaCam.txt");
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

