#include "core/Settings.h"
#include <QCoreApplication>
#include <QSettings>
#include <QFileInfo>

Settings::Settings(){
}

Settings::~Settings(){

}

void Settings::setup(){
	QCoreApplication::setOrganizationName("XROMM");
    QCoreApplication::setApplicationName("Calibration");
}

void Settings::setLastUsedDirectory(QString filename, bool directory){
	QSettings settings;
	if(!directory){
		QFileInfo fileinfo(filename);
		settings.setValue("lastDirectoryUsed",fileinfo.absolutePath());
	}else{
		settings.setValue("lastDirectoryUsed",filename);
	}
}

QString Settings::getLastUsedDirectory(){
	QSettings settings;
	return settings.value("lastDirectoryUsed").toString();
}

void Settings::setUndistortNamingPattern(QString pattern){
	QSettings settings;
	settings.setValue("UndistortNamingPattern",pattern);
}

QString Settings::getUndistortNamingPattern(){
	QSettings settings;
	return settings.value("UndistortNamingPattern","%NAMEBASE%%NUMBER%").toString();
}

void Settings::setAutoConfirmPendingChanges(bool value){
	QSettings settings;
	settings.setValue("AutoConfirmPendingChanges",value);
}
bool Settings::getAutoConfirmPendingChanges(){
	QSettings settings;
	return settings.value("AutoConfirmPendingChanges",false).toBool();
}
void Settings::setAutoCalibAfterReference(bool value){
	QSettings settings;
	settings.setValue("AutoCalibAfterReference",value);
}
bool Settings::getAutoCalibAfterReference(){
	QSettings settings;
	return settings.value("AutoCalibAfterReference",true).toBool();
}

//BlobDetector
void Settings::setBlobDetectorThresholdStep(float value){
	QSettings settings;
	settings.setValue("BlobDetectorThresholdStep",value);
}
float Settings::getBlobDetectorThresholdStep(){
	QSettings settings;
	return settings.value("BlobDetectorThresholdStep",10.0).toFloat();
}
void Settings::setBlobDetectorMinThreshold(float value){
	QSettings settings;
	settings.setValue("BlobDetectorMinThreshold",value);
}
float Settings::getBlobDetectorMinThreshold(){
	QSettings settings;
	return settings.value("BlobDetectorMinThreshold",10.0).toFloat();
}
void Settings::setBlobDetectorMaxThreshold(float value){
	QSettings settings;
	settings.setValue("BlobDetectorMaxThreshold",value);
}
float Settings::getBlobDetectorMaxThreshold(){
	QSettings settings;
	return settings.value("BlobDetectorMaxThreshold",220.0).toFloat();
}
void Settings::setBlobDetectorMinRepeatability(int value){
	QSettings settings;
	settings.setValue("BlobDetectorMinRepeatability",value);
}
int Settings::getBlobDetectorMinRepeatability(){
	QSettings settings;
	return settings.value("BlobDetectorMinRepeatability",2).toInt();
}
void Settings::setBlobDetectorMinDistBetweenBlobs(float value){
	QSettings settings;
	settings.setValue("BlobDetectorMinDistBetweenBlobs",value);
}
float Settings::getBlobDetectorMinDistBetweenBlobs(){
	QSettings settings;
	return settings.value("BlobDetectorMinDistBetweenBlobs",5.0).toFloat();
}

void Settings::setBlobDetectorFilterByColor(bool value){
	QSettings settings;
	settings.setValue("BlobDetectorFilterByColor",value);
}
bool Settings::getBlobDetectorFilterByColor(){
	QSettings settings;
	return settings.value("BlobDetectorFilterByColor",true).toBool();
}
void Settings::setBlobDetectorBlobColor(int value){
	QSettings settings;
	settings.setValue("BlobDetectorBlobColor",value);
}
int Settings::getBlobDetectorBlobColor(){
	QSettings settings;
	return settings.value("BlobDetectorBlobColor",255).toInt();
}

void Settings::setBlobDetectorFilterByArea(bool value){
	QSettings settings;
	settings.setValue("BlobDetectorFilterByArea",value);
}
bool Settings::getBlobDetectorFilterByArea(){
	QSettings settings;
	return settings.value("BlobDetectorFilterByArea",true).toBool();
}
void Settings::setBlobDetectorMinArea(float value){
	QSettings settings;
	settings.setValue("BlobDetectorMinArea",value);
}
float Settings::getBlobDetectorMinArea(){
	QSettings settings;
	return settings.value("BlobDetectorMinArea",25.0).toFloat();
}
void Settings::setBlobDetectorMaxArea(float value){
	QSettings settings;
	settings.setValue("BlobDetectorMaxArea",value);
}
float Settings::getBlobDetectorMaxArea(){
	QSettings settings;
	return settings.value("BlobDetectorMaxArea",5000.0).toFloat();
}

void Settings::setBlobDetectorFilterByCircularity(bool value){
	QSettings settings;
	settings.setValue("BlobDetectorFilterByCircularity",value);
}
bool Settings::getBlobDetectorFilterByCircularity(){
	QSettings settings;
	return settings.value("BlobDetectorFilterByCircularity",true).toBool();
}
void Settings::setBlobDetectorMinCircularity(float value){
	QSettings settings;
	settings.setValue("BlobDetectorMinCircularity",value);
}
float Settings::getBlobDetectorMinCircularity(){
	QSettings settings;
	return settings.value("BlobDetectorMinCircularity",0.5).toFloat();
}
void Settings::setBlobDetectorMaxCircularity(float value){
	QSettings settings;
	settings.setValue("BlobDetectorMaxCircularity",value);
}
float Settings::getBlobDetectorMaxCircularity(){
	QSettings settings;
	return settings.value("BlobDetectorMaxCircularity",5000000.0).toFloat();
}
				
void Settings::setBlobDetectorFilterByInertia(bool value){
	QSettings settings;
	settings.setValue("BlobDetectorFilterByInertia",value);
}
bool Settings::getBlobDetectorFilterByInertia(){
	QSettings settings;
	return settings.value("BlobDetectorFilterByInertia",true).toBool();
}
void Settings::setBlobDetectorMinInertiaRatio(float value){
	QSettings settings;
	settings.setValue("BlobDetectorMinInertiaRatio",value);
}
float Settings::getBlobDetectorMinInertiaRatio(){
	QSettings settings;
	return settings.value("BlobDetectorMinInertiaRatio",0.5).toFloat();
}
void Settings::setBlobDetectorMaxInertiaRatio(float value){
	QSettings settings;
	settings.setValue("BlobDetectorMaxInertiaRatio",value);
}
float Settings::getBlobDetectorMaxInertiaRatio(){
	QSettings settings;
	return settings.value("BlobDetectorMaxInertiaRatio",5000000.0).toFloat();
}

void Settings::setBlobDetectorFilterByConvexity(bool value){
	QSettings settings;
	settings.setValue("BlobDetectorFilterByConvexity",value);
}
bool Settings::getBlobDetectorFilterByConvexity(){
	QSettings settings;
	return settings.value("BlobDetectorFilterByConvexity",true).toBool();
}
void Settings::setBlobDetectorMinConvexity(float value){
	QSettings settings;
	settings.setValue("BlobDetectorMinConvexity",value);
}
float Settings::getBlobDetectorMinConvexity(){
	QSettings settings;
	return settings.value("BlobDetectorMinConvexity",0.9).toFloat();
}
void Settings::setBlobDetectorMaxConvexity(float value){
	QSettings settings;
	settings.setValue("BlobDetectorMaxConvexity",value);
}
float Settings::getBlobDetectorMaxConvexity(){
	QSettings settings;
	return settings.value("BlobDetectorMaxConvexity",5000000.0).toFloat();
}

void Settings::setLocalUndistortionNeighbours(int value){
	QSettings settings;
	settings.setValue("LocalUndistortionNeighbours",value);
}

int Settings::getLocalUndistortionNeighbours(){
	QSettings settings;
	return settings.value("LocalUndistortionNeighbours",12).toInt();
}