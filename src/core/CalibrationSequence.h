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
///\file CalibrationSequence.h
///\author Benjamin Knorlein
///\date 11/15/2016

#ifndef CALIBRATIONSEQUENCE_H_
#define CALIBRATIONSEQUENCE_H_

#include <QString>
#include <QStringList>

#include <opencv/cv.h>
#include <fstream>

namespace xma
{
	class CalibrationImage;
	class Camera;
	class Image;
	class VideoStream;

	class CalibrationSequence
	{
	public:
		CalibrationSequence(Camera * camera);
		virtual ~CalibrationSequence();

		const std::vector<CalibrationImage*>& getCalibrationImages()
		{
			return calibrationImages;
		}

		void reset();
		void deleteFrame(int id);
		void getResolution(int &width, int &height);
		bool checkResolution(int width, int height);

		void loadImages(QStringList fileNames);
		CalibrationImage* addImage(QString fileName);

		void save(QString folder);
		void loadTextures();
		void undistort();
		Image* getImage(int id, bool dist);
		void bindTexture(int id, int type);

		bool hasCalibrationSequence();
		QString getFilename();
		int getNbImages();
		void setCalibrationSequence(QString filename, int nbImages, int width, int height);

	private:
		std::vector<CalibrationImage*> calibrationImages;
		VideoStream * sequence;
		int sequence_width;
		int sequence_height;
		Camera* m_camera;

		void undstortSequenceImage(int id);
		Image* undistortedImage;
		int lastUndistorted; 
		QString sequence_filename;
	};
}


#endif /* CALIBRATIONSEQUENCE_H_ */

