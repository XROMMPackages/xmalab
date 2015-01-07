#ifndef IMAGE_H_
#define IMAGE_H_

#include <QString>
#include <opencv/cv.h>

#ifdef __APPLE__
	#include <OpenGL/gl.h>
	#include <OpenGL/glu.h>
#else
	#ifdef _WIN32
		#include <windows.h>
	#endif
	#include <GL/gl.h>
	#include <GL/glu.h>
#endif

namespace xma{
	class Image{
		public:
			Image(QString imageFileName);
			Image(Image *_image);
			virtual ~Image();
		
			void loadTexture();
			void bindTexture();
			void deleteTexture();

			int getWidth(){return width;}
			int getHeight(){return height;}

			void save(QString filename);
			void getImage(cv::Mat &image);
			void setImage(cv::Mat &image , bool _color = false);
			void setImage(QString imageFileName);

		private:
			cv::Mat image;
			int height,width;
			bool color;

			bool textureLoaded;
			bool image_reset;
			GLuint texture;
	};
}

#endif /* IMAGE_H_ */
