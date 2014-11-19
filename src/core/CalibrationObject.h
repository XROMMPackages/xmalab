/*
 * Calibration.h
 *
 *  Created on: Nov 18, 2013
 *      Author: ben
 */

#ifndef CALIBRATIONOBJECT_H_
#define CALIBRATIONOBJECT_H_

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <QString>

#include <opencv/cv.h>
#include <fstream>

class CalibrationObject{
	public:
		static CalibrationObject* getInstance();
		~CalibrationObject();

		int loadCoords(QString frameSpecificationsFilename , QString referencesFilename);
		void saveCoords(QString folder);
		QString getFrameSpecificationsFilename(){return frameSpecificationsFilename;}
		QString getReferencesFilename(){return referencesFilename;}

		void setCheckerboard(int nbHorizontalSquares, int nbVerticalSquares, int squareSize);

		bool isInitialised(){return initialised;}
		bool isPlanar(){return planar;}
		cv::vector<cv::Point3d>& getFrameSpecifications(){return frameSpecifications;}
		cv::vector<int>& getReferenceIDs(){return referenceIDs;}
		cv::vector<QString>& getReferenceNames(){return referenceNames;}

		int getNbHorizontalSquares(){return nbHorizontalSquares;}
		int getNbVerticalSquares(){return nbVerticalSquares;}
		int getSquareSize(){return squareSize;}

	private:
		CalibrationObject();
		static CalibrationObject* instance;	

		bool planar;
		bool initialised;

		cv::vector<cv::Point3d> frameSpecifications;
		cv::vector<int> referenceIDs;
		cv::vector<QString> referenceNames;

		QString frameSpecificationsFilename;
		QString referencesFilename;

		int nbHorizontalSquares;
		int nbVerticalSquares;
		int squareSize;

		std::istream& comma(std::istream& in);
		std::istream& getline(std::istream &is, std::string &s);
};


#endif /* CALIBRATIONOBJECT_H_ */
