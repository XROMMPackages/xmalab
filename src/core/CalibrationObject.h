#ifndef CALIBRATIONOBJECT_H_
#define CALIBRATIONOBJECT_H_

#include <QString>

#include <opencv/cv.h>
#include <fstream>

namespace xma{
	class CalibrationObject{
	public:
		static CalibrationObject* getInstance();
		virtual ~CalibrationObject();

		int loadCoords(QString frameSpecificationsFilename, QString referencesFilename);
		void saveCoords(QString folder);
		QString getFrameSpecificationsFilename(){ return frameSpecificationsFilename; }
		QString getReferencesFilename(){ return referencesFilename; }

		void setCheckerboard(int nbHorizontalSquares, int nbVerticalSquares, int squareSize);

		bool isInitialised(){ return initialised; }
		bool isPlanar(){ return planar; }
		bool isCheckerboard(){ return checkerboard; }
		bool hasWhiteBlobs(){ return whiteBlobs; }
		cv::vector<cv::Point3d>& getFrameSpecifications(){ return frameSpecifications; }
		cv::vector<int>& getReferenceIDs(){ return referenceIDs; }
		cv::vector<QString>& getReferenceNames(){ return referenceNames; }

		int getNbHorizontalSquares(){ return nbHorizontalSquares; }
		int getNbVerticalSquares(){ return nbVerticalSquares; }
		int getSquareSize(){ return squareSize; }

	private:
		CalibrationObject();
		static CalibrationObject* instance;

		bool planar;
		bool checkerboard;
		bool initialised;
		bool whiteBlobs;

		cv::Point3d scale;

		cv::vector<cv::Point3d> frameSpecifications;
		cv::vector<int> referenceIDs;
		cv::vector<QString> referenceNames;

		QString frameSpecificationsFilename;
		QString referencesFilename;

		int nbHorizontalSquares;
		int nbVerticalSquares;
		int squareSize;
	};
}


#endif /* CALIBRATIONOBJECT_H_ */
