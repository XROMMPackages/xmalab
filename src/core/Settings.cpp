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
///\file Settings.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "core/Settings.h"
#include <QCoreApplication>
#include <QSettings>
#include <QFileInfo>
#include <QColor>
//#include <iostream>
#include <assert.h>

using namespace xma;

Settings* Settings::instance = NULL;

Settings::Settings()
{
	QCoreApplication::setOrganizationName("XROMM");
	QCoreApplication::setApplicationName("XMALab");
	QSettings::setDefaultFormat(QSettings::IniFormat);

	addQStringListSetting("RecentFiles", QStringList());

	//UI
	addBoolSetting("ShowDetailView", true);
	addBoolSetting("ShowDisplayOptions", true);
	addBoolSetting("ShowPlot", true);
	addBoolSetting("ShowEvents", true);
	addBoolSetting("Show3DView", true);
	addBoolSetting("Console", false);
	addQStringSetting("WelcomeDialog", "0");

	//General
	addBoolSetting("AutoConfirmPendingChanges", false);
	addBoolSetting("ConfirmQuitXMALab", true);
	addBoolSetting("CustomWorkspacePath", false);
	addQStringSetting("WorkspacePath", "");
	addIntSetting("FrameAdvance", 10);

	//Undistortion
	addIntSetting("LocalUndistortionNeighbours", 12);
	addQStringSetting("UndistortNamingPattern", "%NAMEBASE%%NUMBER%");

	//Calibration
	addBoolSetting("AutoCalibAfterReference", true);
	addIntSetting("DetectionMethodForCalibration", 0);
	addBoolSetting("ShowAdvancedCalibration", false);
	addBoolSetting("HideWarningsDuringCalibration", false);
	addIntSetting("IdentificationThresholdCalibration", 15);
	addIntSetting("OutlierThresholdForCalibration", 10);
	addBoolSetting("DisableCheckerboardDetection", false);
	addBoolSetting("DisableCheckerboardRefinement", false);

	//Digitizing
	addBoolSetting("CenterDetailView", false);
	addBoolSetting("AdvancedCrosshairDetailView", false);
	addBoolSetting("Show3dPointDetailView", false);
	addBoolSetting("ShowEpiLineDetailView", true);
	addIntSetting("TriangulationMethod", 1);
	addFloatSetting("MaximumReprojectionError", 5.0);
	addBoolSetting("RetrackOptimizedTrackedPoints", true);
	addBoolSetting("TrackInterpolatedPoints", true);
	addBoolSetting("OptimizeRigidBody", true);
	addBoolSetting("DisableRBComputeAdvanced", false);
	

	//Colors
	addQStringSetting("ColorUntrackable", QColor::fromRgb(0, 0, 0).name());
	addQStringSetting("ColorInterpolated", QColor::fromRgb(255, 205, 50).name());
	addQStringSetting("ColorManual", QColor::fromRgb(200, 0, 30).name());
	addQStringSetting("ColorManualAndOpt", QColor::fromRgb(255, 0, 42).name());
	addQStringSetting("ColorSet", QColor::fromRgb(0, 200, 100).name());
	addQStringSetting("ColorSetAndOpt", QColor::fromRgb(0, 255, 128).name());
	addQStringSetting("ColorTracked", QColor::fromRgb(0, 100, 200).name());
	addQStringSetting("ColorTrackedAndOpt", QColor::fromRgb(0, 128, 255).name());
	addQStringSetting("ColorUndefined", QColor::fromRgb(210, 210, 210).name());

	addQStringSetting("ColorInterNone", QColor::fromRgb(255, 255, 255).name());
	addQStringSetting("ColorInterRepeat", QColor::fromRgb(230 ,128, 128).name());
	addQStringSetting("ColorInterLinear", QColor::fromRgb(184, 230, 184).name());
	addQStringSetting("ColorInterCubic", QColor::fromRgb(170, 170, 230).name());

	addBoolSetting("ShowColoredMarkerIDs", false);
	addBoolSetting("ShowColoredMarkerCross", false);

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
	addFloatSetting("BlobDetectorMinConvexity", 0.9f);
	addFloatSetting("BlobDetectorMaxConvexity", 5000000.0);

	//ImportExport Points
	addBoolSetting("Import2DCount0", true);
	addBoolSetting("Import2DCount1", false);
	addBoolSetting("Import2DYDown", true);
	addBoolSetting("Import2DYUp", false);
	addBoolSetting("Import2DNoHeader", false);
	addBoolSetting("Import2DHeader", true);
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
	addBoolSetting("Export3DNoCols", true);
	addBoolSetting("Export3DOffsetCols", false);

	addBoolSetting("ExportTransMulti", true);
	addBoolSetting("ExportTransSingle", false);
	addBoolSetting("ExportTransNoHeader", false);
	addBoolSetting("ExportTransHeader", true);
	addBoolSetting("ExportTransUnfiltered", false);
	addBoolSetting("ExportTransFiltered", true);
	addBoolSetting("ExportTransNoCols", true);
	addBoolSetting("ExportTransOffsetCols", false);

	addBoolSetting("TrialDrawHideAll", false);
	addBoolSetting("TrialDrawMarkers", true);
	addBoolSetting("TrialDrawMarkerIds", false);
	addBoolSetting("TrialDrawEpipolar", true);
	addBoolSetting("TrialDrawRigidBodyConstellation", true);
	addBoolSetting("TrialDrawRigidBodyMeshmodels", true);
	addBoolSetting("TrialDrawFiltered", false);


	addBoolSetting("ShowMarkerStates", false);

	addIntSetting("EpipolarLinePrecision", 5);
	addIntSetting("DefaultMarkerThreshold", 8);
}

Settings::~Settings()
{
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

template <typename T>
int findIdx(QString name, std::vector<std::pair<QString, T> >& vec)
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

void Settings::set(QString name, QStringList value)
{
	QSettings settings;
	int idx = findIdx(name, qstringListSettings);
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

void Settings::addQStringListSetting(QString name, QStringList defaultValue)
{
	qstringListSettings.push_back(std::make_pair(name, defaultValue));
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

QStringList Settings::getQStringListSetting(QString name)
{
	QSettings settings;
	int idx = findIdx(name, qstringListSettings);
	if (idx >= 0)
	{
		return settings.value(name, qstringListSettings[idx].second).toStringList();
	}
	assert(idx < 0);
	return QStringList();
}

//Special Settings
void Settings::setLastUsedDirectory(QString filename, bool directory)
{
	QSettings settings;
	if (!directory)
	{
		QFileInfo fileinfo(filename);
		settings.setValue("lastDirectoryUsed", fileinfo.absolutePath());
	}
	else
	{
		settings.setValue("lastDirectoryUsed", filename);
	}
}

QString Settings::getLastUsedDirectory()
{
	QSettings settings;
	return settings.value("lastDirectoryUsed").toString();
}

void Settings::checkRecenFiles()
{
	QStringList previouslist = getQStringListSetting("RecentFiles");
	foreach(const QString &file, previouslist) {
		QFileInfo check_file(file);
		if (!check_file.exists() || !check_file.isFile())
			previouslist.removeOne(file);
	}
	
	set("RecentFiles", previouslist);
}

void Settings::addToRecentFiles(QString filename)
{
	QStringList previouslist = getQStringListSetting("RecentFiles");
	previouslist.insert(0, filename);
	previouslist.removeDuplicates();
	if (previouslist.size() > 10)
	{
		previouslist.removeLast();
	}
	set("RecentFiles", previouslist);
}

void Settings::setUIGeometry(QString windowTitle, QByteArray geometry)
{
	QSettings settings;
	settings.setValue("UIGeometry_" + windowTitle, geometry);
}

QByteArray Settings::getUIGeometry(QString windowTitle)
{
	QSettings settings;
	return settings.value("UIGeometry_" + windowTitle, "").toByteArray();
}

void Settings::setUIState(QString windowTitle, QByteArray state)
{
	QSettings settings;
	//std::cerr << state.toHex().data() << std::endl;
	settings.setValue("UIState_" + windowTitle, state);
}

QByteArray Settings::getUIState(QString windowTitle)
{
	QSettings settings;
	QByteArray val = settings.value("UIState_" + windowTitle, "").toByteArray();
	if (val == QByteArray())
	{
		val = QByteArray::fromHex(QByteArray("000000ff00000000fd0000000300000000000000a900000278fc0200000003fb000000200050006f0069006e007400730044006f0063006b0057006900640067006500740100000015000002780000006f00fffffffb000000260057006f0072006c006400560069006500770044006f0063006b00570069006400670065007403000003870000027c000001f4000001f4fb0000002000570069007a0061007200640044006f0063006b00570069006400670065007403000003ca0000008300000175000001be000000020000046f00000039fc0100000001fc0000005d0000046f0000000000fffffffa000000010200000002fb0000002400500072006f006700720065007300730044006f0063006b0057006900640067006500740000000000ffffffff0000003900fffffffb000000260057006f0072006c006400560069006500770044006f0063006b005700690064006700650074030000091d000001dc000001f4000001f400000003000004cc00000141fc0100000003fb0000002800440065007400610069006c00560069006500770044006f0063006b0057006900640067006500740100000000000001e4000001e400fffffffb000000140050006c006f007400570069006e0064006f007701000001e8000002e4000002e400fffffffb000000220043006f006e0073006f006c00650044006f0063006b005700690064006700650074020000051e0000021a00000324000001450000041f0000027800000001000000020000000800000008fc00000000"));
	}
	return val;
}

