#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "core/Image.h"
#include "core/Camera.h"
#include "core/Project.h"
#include "core/CalibrationImage.h"
#include "core/UndistortionObject.h"

#include <QApplication>

#include <fstream>

#ifdef WIN32
	#define OS_SEP "\\"
#else
	#define OS_SEP "/"
#endif

#ifndef _PI
	#define _PI 3.141592653
#endif

using namespace xma;

Camera::Camera(QString cameraName, int _id){
	calibrationImages.clear();
	undistortionObject = NULL;

	name = cameraName;
	id = _id;
	height=0;
	width=0;
	lightCamera = false;

	calibrated = false;
	cameramatrix.create(3, 3, CV_64F);
	requiresRecalibration = 0;
	updateInfoRequired = false;
}

Camera::~Camera(){
	for(std::vector<CalibrationImage*>::iterator it = calibrationImages.begin(); it != calibrationImages.end(); ++it)
		delete *it;
	calibrationImages.clear();
	if(undistortionObject)
		delete undistortionObject;

	cameramatrix.release();
}

void Camera::reset(){
	cameramatrix.release();
	cameramatrix.create(3, 3, CV_64F);
	setCalibrated(false);
	setRecalibrationRequired(0);
	setUpdateInfoRequired(true);
	for(std::vector<CalibrationImage*>::iterator it = calibrationImages.begin(); it != calibrationImages.end(); ++it){
		(*it)->reset();
	}
}

void Camera::deleteFrame(int id){
	delete calibrationImages[id];
	calibrationImages.erase(calibrationImages.begin()+id);

}

void Camera::loadImages(QStringList fileNames){
	for ( QStringList::const_iterator constIterator = fileNames.constBegin(); constIterator != fileNames.constEnd(); ++constIterator)
		calibrationImages.push_back(new CalibrationImage(this,(*constIterator)));
}

CalibrationImage* Camera::addImage(QString fileName){
	CalibrationImage * image = new CalibrationImage(this,fileName);
	calibrationImages.push_back(image);
	return image;
}

void Camera::loadUndistortionImage(QString undistortionImage){
	undistortionObject = new UndistortionObject(this, undistortionImage);
}

bool Camera::setResolutions(){
	width = calibrationImages[0]->getWidth();
	height = calibrationImages[0]->getHeight();

	for(std::vector<CalibrationImage*>::iterator it = calibrationImages.begin(); it != calibrationImages.end(); ++it){
		if(width != (*it)->getWidth() || height != (*it)->getHeight()) return false;
	}

	if(undistortionObject && (width != undistortionObject->getWidth() || height != undistortionObject->getHeight())) return false;

	return true;
}

void Camera::save(QString folder){
	if(isCalibrated()){
		saveCameraMatrix(folder +  "data" +  OS_SEP  + getFilenameCameraMatrix());
	}

	if(undistortionObject){
		undistortionObject->getImage()->save(folder + undistortionObject->getFilename());
		if(undistortionObject->isComputed()){
			undistortionObject->savePointsDetected(folder +  "data" +  OS_SEP  + undistortionObject->getFilenamePointsDetected());
			undistortionObject->saveGridPointsDistorted(folder +  "data" +  OS_SEP  + undistortionObject->getFilenameGridPointsDistorted());
			undistortionObject->saveGridPointsReferences(folder +  "data" +  OS_SEP  + undistortionObject->getFilenameGridPointsReferences());
			undistortionObject->saveGridPointsInlier(folder +  "data" +  OS_SEP  + undistortionObject->getFilenameGridPointsInlier());
		}
	}
	
	for(std::vector<CalibrationImage*>::iterator it = calibrationImages.begin(); it != calibrationImages.end(); ++it){
		(*it)->getImage()->save(folder + (*it)->getFilename());
		if((*it)->isCalibrated()){
			(*it)->savePointsInlier(folder +  "data" +  OS_SEP  + (*it)->getFilenamePointsInlier());
			(*it)->savePointsDetected(folder +  "data" +  OS_SEP  + (*it)->getFilenamePointsDetected());
			(*it)->savePointsDetectedAll(folder +  "data" +  OS_SEP  + (*it)->getFilenamePointsDetectedAll());
			(*it)->saveRotationMatrix(folder +  "data" +  OS_SEP  + (*it)->getFilenameRotationMatrix());
			(*it)->saveTranslationVector(folder +  "data" +  OS_SEP  + (*it)->getFilenameTranslationVector());
		}
	}
}

void Camera::loadTextures(){
	if(undistortionObject)
		undistortionObject->loadTextures();
	
	QApplication::processEvents();

	for(std::vector<CalibrationImage*>::iterator it = calibrationImages.begin(); it != calibrationImages.end(); ++it){
		(*it)->loadTextures();
		QApplication::processEvents();
	}
}

void Camera::undistort(){
	if(undistortionObject && undistortionObject->isComputed()){
		undistortionObject->undistort(undistortionObject->getImage(),undistortionObject->getUndistortedImage());
		for(std::vector<CalibrationImage*>::iterator it = calibrationImages.begin(); it != calibrationImages.end(); ++it){
			undistortionObject->undistort((*it)->getImage(),(*it)->getUndistortedImage());
			(*it)->undistortPoints();
			if(isCalibrated())setRecalibrationRequired(1);
		}
	}
}

void Camera::setCalibrated(bool value)
{
	calibrated = value;
	Project::getInstance()->checkCalibration();
}

void Camera::setCameraMatrix(cv::Mat & _cameramatrix){
	cameramatrix = _cameramatrix.clone();
	setCalibrated(true);
}

cv::Mat Camera::getCameraMatrix(){
	return cameramatrix.clone();
}

cv::Mat Camera::getProjectionMatrix(int referenceFrame)
{
	cv::Mat projectionmatrix;
	projectionmatrix.create(3, 4, CV_64F);

	cv::Mat tmp;
	cv::Mat rotationmatrix;
	rotationmatrix.create(3, 3, CV_64F);
	cv::Rodrigues(getCalibrationImages()[referenceFrame]->getRotationVector(), rotationmatrix);
	rotationmatrix.copyTo(tmp);
	cv::hconcat(tmp, getCalibrationImages()[referenceFrame]->getTranslationVector(), tmp);
	projectionmatrix = cameramatrix * tmp;

	rotationmatrix.release();
	tmp.release();
	return projectionmatrix;
}

QString Camera::getFilenameCameraMatrix(){
	return this->getName() + "_CameraMatrix.csv";
}
void Camera::saveCameraMatrix( QString filename){
	std::ofstream outfile (filename.toAscii().data());
	for(unsigned  int i = 0; i < 3; ++i ){
		outfile << cameramatrix.at<double>(i,0) << ", "  << cameramatrix.at<double>(i,1) << ", "  << cameramatrix.at<double>(i,2) << std::endl;
	}
	
	outfile.close();
}
std::istream&Camera::comma(std::istream& in)
{
    if ((in >> std::ws).peek() != std::char_traits<char>::to_int_type(',')) {
        in.setstate(std::ios_base::failbit);
    }
    return in.ignore();
}
std::istream& Camera::getline(std::istream &is, std::string &s)
{ 
    char ch;
    s.clear();
    while (is.get(ch) && ch != '\n' && ch != '\r')
        s += ch;
    return is;
}


void Camera::loadCameraMatrix( QString filename){
	std::vector<std::vector<double> > values;
	std::ifstream fin(filename.toAscii().data());
    std::istringstream in;
    for (std::string line; std::getline(fin, line); )
    {
        in.clear();
        in.str(line);
        std::vector<double> tmp;
        for (double value; in >> value; comma(in)) {
            tmp.push_back(value);
        }
		if(tmp.size()>0) values.push_back(tmp);
    }
	fin.close();

	for (unsigned int y = 0; y < 3 ; y++ )
	{
		 cameramatrix.at<double>(y,0) = values[y][0];
		 cameramatrix.at<double>(y,1) = values[y][1];
		 cameramatrix.at<double>(y,2) = values[y][2];
	}

	for (unsigned int y = 0; y < values.size();y++ )
	{
		values[y].clear();
	}
	values.clear();
}

void Camera::getMayaCam(double * out, int frame)
{
	cv::Mat transTmp;
	cv::Mat rotTmp;
	cv::Mat camTmp;
	camTmp.create(3, 3, CV_64F);
	rotTmp.create(3, 3, CV_64F);
	transTmp.create(3, 1, CV_64F);

	camTmp = cameramatrix.clone();
	transTmp = getCalibrationImages()[frame]->getTranslationVector();
	cv::Rodrigues(getCalibrationImages()[frame]->getRotationVector(), rotTmp);

	//adjust y - inversion
	transTmp.at<double>(0,0) = -transTmp.at<double>(0,0);
	transTmp.at<double>(0,2) = -transTmp.at<double>(0,2);
	for(int i = 0; i < 3 ; i ++){
		rotTmp.at<double>(0,i) = -rotTmp.at<double>(0,i);
		rotTmp.at<double>(2,i) = -rotTmp.at<double>(2,i);
	}	
	camTmp.at<double>(1,2) = (getHeight() - 1) - camTmp.at<double>(1,2);

	//Invert Transformation
	rotTmp = rotTmp.inv();
	transTmp = -rotTmp * transTmp;

	//0-2 Translation
	out[0] = transTmp.at<double>(0,0);
	out[1] = transTmp.at<double>(0,1);
	out[2] = transTmp.at<double>(0,2);

	//3-5 rotation angles in yaw pitch roll
	out[3] = 180.0 / _PI * atan2(-rotTmp.at<double>(2,1),rotTmp.at<double>(2,2));
	out[4] = 180.0 / _PI *atan2(rotTmp.at<double>(2,0), sqrt(rotTmp.at<double>(2,1) * rotTmp.at<double>(2,1)
									+rotTmp.at<double>(2,2)*rotTmp.at<double>(2,2)));
	out[5] = 180.0 / _PI *atan2(-rotTmp.at<double>(1,0),-rotTmp.at<double>(0,0));
	
	//6-8 planeX,planeY,planeZ
	double _near = 0.1;
	double f = - 0.5 * (camTmp.at<double>(0,0) + camTmp.at<double>(1,1));

	//Have to check here
	out[6]  = (0.5 *  getWidth() - (camTmp.at<double>(0,2) + 1.0))*_near; //(-0.1 for x coordinates from 1)
	out[7]  = (0.5 *  getHeight() - (camTmp.at<double>(1,2) + 1.0))*_near; //(-0.1 for y coordinates from 1)
	out[8]  = _near*f;

	//9-11 center_x, center_y, f
	out[9]  =  camTmp.at<double>(0,2) + 1.0; //(+1 for x coordinates from 1)
	out[10]  = camTmp.at<double>(1,2) + 1.0; //(+1 for y coordinates from 1)
	out[11]  = f;

	//scale, width, height
	out[12]  = _near;
	out[13]  =  getWidth();
	out[14]  =  getHeight();
}

void Camera::getDLT(double * out, int frame){
	cv::Mat transTmp;
	cv::Mat rotTmp;
	cv::Mat camTmp;
	camTmp.create(3, 3, CV_64F);
	rotTmp.create(3, 3, CV_64F);
	transTmp.create(3, 1, CV_64F);

	camTmp = cameramatrix.clone();
	transTmp = getCalibrationImages()[frame]->getTranslationVector();
	cv::Rodrigues(getCalibrationImages()[frame]->getRotationVector(), rotTmp);

	//adjust y - inversion
	transTmp.at<double>(0,0) = -transTmp.at<double>(0,0);
	transTmp.at<double>(0,2) = -transTmp.at<double>(0,2);
	for(int i = 0; i < 3 ; i ++){
		rotTmp.at<double>(0,i) = -rotTmp.at<double>(0,i);
		rotTmp.at<double>(2,i) = -rotTmp.at<double>(2,i);
	}	
	camTmp.at<double>(1,2) = (getHeight() - 1) - camTmp.at<double>(1,2);
	
	cv::Mat tmp;
	cv::Mat projectionmatrix;
	projectionmatrix.create(3,4,CV_64F);
	rotTmp.copyTo(tmp);
	cv::hconcat(tmp,transTmp,tmp);
	projectionmatrix = camTmp * tmp;
	int count = 0;
	for(unsigned int i = 0 ; i < 3; i++){
		for(unsigned int j = 0; j < 4; j ++){
			out[count] = projectionmatrix.at<double>(i,j);
			count++;
		}
	}
	projectionmatrix.release();

	//normalize
	for(unsigned int i = 0; i < 4 ; i++){
		out[i]		= (out[i]		+ out[i + 8]) / out[11]; // (+1 for x coordinates from 1)
		out[i + 4]  = (out[i + 4]   + out[i + 8]) / out[11]; // (+1 for x coordinates from 1)
		out[i + 8]  =  out[i + 8]				  / out[11];
	}
}