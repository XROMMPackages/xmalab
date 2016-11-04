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
///\file VideoStream.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef VIDEOSTREAM_H
#define VIDEOSTREAM_H

#include "core/Image.h"
#include <QStringList>

namespace xma
{
	class VideoStream
	{
	public:
		VideoStream(QStringList _filenames);
		virtual ~VideoStream();

		virtual void setActiveFrame(int _activeFrame) = 0;

		virtual QString getFrameName(int frameNumber) = 0;
		virtual void reloadFile() = 0;

		QStringList getFilenames();
		QString getFileBasename();
		void changeImagePath(QString newfolder, QString oldfolder);
		void save(QString path);
		int getNbImages();
		Image* getImage();
		void bindTexture();
		double getFPS();


		void parseXMLData(int id, QString xml_data);

		const QString& getFilename() const;
		const int& getFileId() const;
		const QString& getFileCategory() const;
		const QString& getCameraNumber() const;
		const QString& getInstrument() const;
		const double& getFrameRate() const;
		const QString& getShutterSpeed() const;
		const QString& getKV() const;
		const QString& getEDR() const;
		const QString& getMA() const;
		const QString& getSid() const;
		const QString& getMagLevel() const;
		const QString& getRadiationtype() const;
		const QString& getPulsewidth() const;
		const QString& getFileDescription() const;
		const QString& getLab() const;

	protected:
		Image* image;
		int nbImages;
		QStringList filenames;
		double fps;

	protected:
		QString Filename;
		int FileID;
		QString FileCategory;
		QString cameraNumber;
		QString Instrument;
		double FrameRate;
		QString ShutterSpeed;
		QString kV;
		QString EDR;
		QString mA;
		QString SID;
		QString MagLevel;
		QString Radiationtype;
		QString Pulsewidth;
		QString FileDescription;
		QString Lab;
	};
}

#endif //VIDEOSTREAM_H


