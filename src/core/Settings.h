#ifndef SETTINGS_H
#define SETTINGS_H

#include <QString>
#include <QSettings>

#define UI_VERSION 0

namespace xma{
	class Settings{

	public:
		Settings();
		virtual ~Settings();

		//setup Application and Company Name
		static void setup();

		//Filename
		static void setLastUsedDirectory(QString Filename, bool directory = false);
		static QString getLastUsedDirectory();

		//UI
		static void setUIGeometry(QString windowTitle, QByteArray geometry);
		static QByteArray getUIGeometry(QString windowTitle);
		static void setUIState(QString windowTitle, QByteArray state);
		static QByteArray getUIState(QString windowTitle);
		static void setShowDetailView(bool value);
		static bool getShowDetailView();



		//Workflow
		static void setAutoConfirmPendingChanges(bool value);
		static bool getAutoConfirmPendingChanges();
		static void setAutoCalibAfterReference(bool value);
		static bool getAutoCalibAfterReference();

		//NamingPattern
		static void setUndistortNamingPattern(QString pattern);
		static QString getUndistortNamingPattern();

		//BlobDetector
		static void setBlobDetectorThresholdStep(float value);
		static float getBlobDetectorThresholdStep();
		static void setBlobDetectorMinThreshold(float value);
		static float getBlobDetectorMinThreshold();
		static void setBlobDetectorMaxThreshold(float value);
		static float getBlobDetectorMaxThreshold();
		static void setBlobDetectorMinRepeatability(int value);
		static int getBlobDetectorMinRepeatability();
		static void setBlobDetectorMinDistBetweenBlobs(float value);
		static float getBlobDetectorMinDistBetweenBlobs();

		static void setBlobDetectorFilterByColor(bool value);
		static bool getBlobDetectorFilterByColor();
		static void setBlobDetectorBlobColor(int value);
		static int getBlobDetectorBlobColor();

		static void setBlobDetectorFilterByArea(bool value);
		static bool getBlobDetectorFilterByArea();
		static void setBlobDetectorMinArea(float value);
		static float getBlobDetectorMinArea();
		static void setBlobDetectorMaxArea(float value);
		static float getBlobDetectorMaxArea();

		static void setBlobDetectorFilterByCircularity(bool value);
		static bool getBlobDetectorFilterByCircularity();
		static void setBlobDetectorMinCircularity(float value);
		static float getBlobDetectorMinCircularity();
		static void setBlobDetectorMaxCircularity(float value);
		static float getBlobDetectorMaxCircularity();

		static void setBlobDetectorFilterByInertia(bool value);
		static bool getBlobDetectorFilterByInertia();
		static void setBlobDetectorMinInertiaRatio(float value);
		static float getBlobDetectorMinInertiaRatio();
		static void setBlobDetectorMaxInertiaRatio(float value);
		static float getBlobDetectorMaxInertiaRatio();

		static void setBlobDetectorFilterByConvexity(bool value);
		static bool getBlobDetectorFilterByConvexity();
		static void setBlobDetectorMinConvexity(float value);
		static float getBlobDetectorMinConvexity();
		static void setBlobDetectorMaxConvexity(float value);
		static float getBlobDetectorMaxConvexity();

		//Local Undistortion Params
		static void setLocalUndistortionNeighbours(int value);
		static int getLocalUndistortionNeighbours();
	};
}

	

#endif  // SETTINGS_H
