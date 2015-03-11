#ifndef IMAGESEQUENCE_H_
#define IMAGESEQUENCE_H_

#include <core/VideoStream.h>

namespace xma{
	class ImageSequence: public VideoStream{
		public:
			ImageSequence(QStringList _filenames);
			virtual ~ImageSequence();
		
			void setActiveFrame(int _activeFrame) override;
			QString getFrameName(int frameNumber) override;
		private:
	};
}

#endif /* CINEVIDEO_H_ */
