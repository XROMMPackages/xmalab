#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "core/Settings.h"
#include <QCoreApplication>
#include <QSettings>
#include <QFileInfo>

#include <assert.h>

using namespace xma;

Settings* Settings::instance = NULL;

Settings::Settings(){
	QCoreApplication::setOrganizationName("XROMM");
	QCoreApplication::setApplicationName("XMALab");
	QSettings::setDefaultFormat(QSettings::IniFormat);

	//UI
	addBoolSetting("ShowDetailView", true);
	addBoolSetting("ShowPlot", false);
	addBoolSetting("Show3DView", false);
	addBoolSetting("Console", false);

	//General
	addBoolSetting("AutoConfirmPendingChanges", false);
	addBoolSetting("ConfirmQuitXMALab", true);

	//Undistortion
	addIntSetting("LocalUndistortionNeighbours", 12);
	addQStringSetting("UndistortNamingPattern", "%NAMEBASE%%NUMBER%");
	
	//Calibration
	addBoolSetting("AutoCalibAfterReference", true);

	//Digitizing
	addBoolSetting("CenterDetailView", false);
	addBoolSetting("AdvancedCrosshairDetailView", false);
	addBoolSetting("Show3dPointDetailView", false);
	addBoolSetting("ShowEpiLineDetailView", true);

	//BlobDetector
	addFloatSetting("BlobDetectorThresholdStep", 10.0);
	addFloatSetting("BlobDetectorMinThreshold", 10.0);
	addFloatSetting("BlobDetectorMaxThreshold", 220.0);
	addIntSetting("BlobDetectorMinRepeatability", 2);
	addFloatSetting("BlobDetectorMinDistBetweenBlobs", 5.0);
	
	addBoolSetting("BlobDetectorFilterByColor", true);
	addIntSetting("BlobDetectorBlobColor", 255);

	addBoolSetting("BlobDetectorFilterByArea", true);
	addFloatSetting("BlobDetectorMinArea", 25.0);
	addFloatSetting("BlobDetectorMaxArea", 5000.0);

	addBoolSetting("BlobDetectorFilterByCircularity", true);
	addFloatSetting("BlobDetectorMinCircularity", 0.5);
	addFloatSetting("BlobDetectorMaxCircularity", 5000000.0);

	addBoolSetting("BlobDetectorFilterByInertia", true);
	addFloatSetting("BlobDetectorMinInertiaRatio", 0.5);
	addFloatSetting("BlobDetectorMaxInertiaRatio", 5000000.0);

	addBoolSetting("BlobDetectorFilterByConvexity", true);
	addFloatSetting("BlobDetectorMinConvexity", 0.9);
	addFloatSetting("BlobDetectorMaxConvexity", 5000000.0);

	//ImportExport Points
	addBoolSetting("Import2DCount0",true);
	addBoolSetting("Import2DCount1",false);
	addBoolSetting("Import2DYDown",true);
	addBoolSetting("Import2DYUp",false);
	addBoolSetting("Import2DNoHeader",false);
	addBoolSetting("Import2DHeader",true);
	addBoolSetting("Import2DDistorted", true);
	addBoolSetting("Import2DUndistorted", false);
	addBoolSetting("Import2DNoCols", true);
	addBoolSetting("Import2DOffsetCols", false); 

	addBoolSetting("Export2DMulti", true);
	addBoolSetting("Export2DSingle", false);
	addBoolSetting("Export2DCount0", true);
	addBoolSetting("Export2DCount1", false);
	addBoolSetting("Export2DYDown", true);
	addBoolSetting("Export2DYUp", false);
	addBoolSetting("Export2DNoHeader", false);
	addBoolSetting("Export2DHeader", true);
	addBoolSetting("Export2DDistorted", true);
	addBoolSetting("Export2DUndistorted", false);
	addBoolSetting("Export2DNoCols", true);
	addBoolSetting("Export2DOffsetCols", false);

	addBoolSetting("Export3DMulti", true);
	addBoolSetting("Export3DSingle", false);
	addBoolSetting("Export3DNoHeader", false);
	addBoolSetting("Export3DHeader", true);

	addBoolSetting("ExportTransMulti", true);
	addBoolSetting("ExportTransSingle", false);
	addBoolSetting("ExportTransNoHeader", false);
	addBoolSetting("ExportTransHeader", true);
	addBoolSetting("ExportTransUnfiltered", false);
	addBoolSetting("ExportTransFiltered", true);

}

Settings::~Settings(){
	booleanSettings.clear();
	intSettings.clear();
	floatSettings.clear();
	qstringSettings.clear();
}

Settings* Settings::getInstance()
{
	if (!instance)
	{
		instance = new Settings();
	}
	return instance;
}

template<typename T>
int findIdx(QString name, std::vector< std::pair < QString, T > > &vec)
{
	for (unsigned int i = 0; i < vec.size(); i++)
	{
		if (vec[i].first == name)return i;
	}
	return -1;
}

void Settings::set(QString name, bool value)
{
	QSettings settings;
	int idx = findIdx(name, booleanSettings);
	if (idx >= 0)
	{
		settings.setValue(name, value);
		return;
	}
	assert(idx < 0);
}
void Settings::set(QString name, int value)
{
	QSettings settings;
	int idx = findIdx(name, intSettings);
	if (idx >= 0)
	{
		settings.setValue(name, value);
		return;
	}
	assert(idx < 0);
}
void Settings::set(QString name, float value)
{
	QSettings settings;
	int idx = findIdx(name, floatSettings);
	if (idx >= 0)
	{
		settings.setValue(name, value);
		return;
	}
	assert(idx < 0);
}
void Settings::set(QString name, QString value)
{
	QSettings settings;
	int idx = findIdx(name, qstringSettings);
	if (idx >= 0)
	{
		settings.setValue(name, value);
		return;
	}
	assert(idx < 0);
}

void Settings::addBoolSetting(QString name, bool defaultValue)
{
	booleanSettings.push_back(std::make_pair(name, defaultValue));
}
void Settings::addIntSetting(QString name, int defaultValue)
{
	intSettings.push_back(std::make_pair(name, defaultValue));
}
void Settings::addFloatSetting(QString name, float defaultValue)
{
	floatSettings.push_back(std::make_pair(name, defaultValue));
}
void Settings::addQStringSetting(QString name, QString defaultValue)
{
	qstringSettings.push_back(std::make_pair(name, defaultValue));
}

bool Settings::getBoolSetting(QString name)
{
	QSettings settings;
	int idx = findIdx(name, booleanSettings);
	if (idx >= 0)
	{
		return settings.value(name, booleanSettings[idx].second).toBool();
	}
	assert(idx < 0);
	return false;
}
int Settings::getIntSetting(QString name)
{
	QSettings settings;
	int idx = findIdx(name, intSettings);
	if (idx >= 0)
	{
		return settings.value(name, intSettings[idx].second).toInt();
	}
	assert(idx < 0);
	return -1;
}
float Settings::getFloatSetting(QString name)
{
	QSettings settings;
	int idx = findIdx(name, floatSettings);
	if (idx >= 0)
	{
		return settings.value(name, floatSettings[idx].second).toFloat();
	}
	assert(idx < 0);
	return -1;
}
QString Settings::getQStringSetting(QString name)
{
	QSettings settings;
	int idx = findIdx(name, qstringSettings);
	if (idx >= 0)
	{
		return settings.value(name, qstringSettings[idx].second).toString();
	}
	assert(idx < 0);
	return "";
}

//Special Settings
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
void Settings::setUIGeometry(QString windowTitle, QByteArray geometry){
	QSettings settings;
	settings.setValue("UIGeometry_" + windowTitle, geometry);
}
QByteArray Settings::getUIGeometry(QString windowTitle){
	QSettings settings;
	return settings.value("UIGeometry_" + windowTitle,"").toByteArray();
}
void Settings::setUIState(QString windowTitle, QByteArray state){
	QSettings settings;
	settings.setValue("UIState_" + windowTitle, state);
}
QByteArray Settings::getUIState(QString windowTitle){
	QSettings settings;
	return settings.value("UIState_" + windowTitle,"").toByteArray();
}


