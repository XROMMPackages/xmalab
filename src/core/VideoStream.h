#ifndef VIDEOSTREAM_H
#define VIDEOSTREAM_H

#include "core/Image.h"
#include <QStringList>

namespace xma
{
	class VideoStream{
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


		protected:
			Image * image;
			int nbImages;
			QStringList filenames;
			double fps;
	};
}

#endif  //VIDEOSTREAM_H

