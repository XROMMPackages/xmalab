#ifndef AVIVIDEO_H_
#define AVIVIDEO_H_

#include <core/VideoStream.h>

namespace xma{
	class AviVideo: public VideoStream{
		public:
			AviVideo(QStringList _filenames);
			virtual ~AviVideo();
		
			void setActiveFrame(int _activeFrame) override;
			QString getFrameName(int frameNumber) override;
		private:
	};
}

#endif /* AVIVIDEO_H_ */
