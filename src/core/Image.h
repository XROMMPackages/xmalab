/*
 * Calibration.h
 *
 *  Created on: Nov 18, 2013
 *      Author: ben
 */

#ifndef IMAGE_H_
#define IMAGE_H_

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

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

class Image{
	public:
		Image(QString imageFileName);
		Image(Image *_image);
		~Image();
		
		void loadTexture();
		void bindTexture();
		void deleteTexture();

		int getWidth(){return width;}
		int getHeight(){return height;}

		void save(QString filename);
		void getImage(cv::Mat &image);
		void setImage(cv::Mat &image , bool _color = false);

	private:
		cv::Mat image;
		int height,width;
		bool color;

		bool textureLoaded;
		bool image_reset;
		GLuint texture;
};


#endif /* IMAGE_H_ */
