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
///\file CalibrationObject.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef CALIBRATIONOBJECT_H_
#define CALIBRATIONOBJECT_H_

#include <QString>

#include <opencv2/opencv.hpp>
#include <fstream>

namespace xma
{
	class CalibrationObject
	{
	public:
		static CalibrationObject* getInstance();
		virtual ~CalibrationObject();

		int loadCoords(QString frameSpecificationsFilename, QString referencesFilename);
		void saveCoords(QString folder);

		QString getFrameSpecificationsFilename()
		{
			return frameSpecificationsFilename;
		}

		QString getReferencesFilename()
		{
			return referencesFilename;
		}

		void setCheckerboard(int nbHorizontalSquares, int nbVerticalSquares, double squareSize);

		bool isInitialised()
		{
			return initialised;
		}

		bool isPlanar()
		{
			return planar;
		}

		bool isCheckerboard()
		{
			return checkerboard;
		}

		bool hasWhiteBlobs()
		{
			return whiteBlobs;
		}

		std::vector<cv::Point3d>& getFrameSpecifications()
		{
			return frameSpecifications;
		}

		std::vector<int>& getReferenceIDs()
		{
			return referenceIDs;
		}

		std::vector<QString>& getReferenceNames()
		{
			return referenceNames;
		}

		int getNbHorizontalSquares()
		{
			return nbHorizontalSquares;
		}

		int getNbVerticalSquares()
		{
			return nbVerticalSquares;
		}

		double getSquareSize()
		{
			return squareSize;
		}

	private:
		CalibrationObject();
		static CalibrationObject* instance;

		bool planar;
		bool checkerboard;
		bool initialised;
		bool whiteBlobs;

		cv::Point3d scale;

		std::vector<cv::Point3d> frameSpecifications;
		std::vector<int> referenceIDs;
		std::vector<QString> referenceNames;

		QString frameSpecificationsFilename;
		QString referencesFilename;

		int nbHorizontalSquares;
		int nbVerticalSquares;
		double squareSize;
	};
}


#endif /* CALIBRATIONOBJECT_H_ */

