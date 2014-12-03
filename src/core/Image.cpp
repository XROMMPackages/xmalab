/*
 * Calibration.cpp
 *
 *  Created on: Nov 18, 2013
 *      Author: ben
 */

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "Image.h"
#include "Camera.h"
#include "ui/GLSharedWidget.h"

#include <opencv/highgui.h>

#include <QFileInfo>

#ifndef GL_BGR
#define GL_BGR 0x80E0
#endif

Image::Image(QString _imageFileName){
	color = false;
	cv::Mat imageTMP;
	imageTMP = cv::imread(_imageFileName.toAscii().data(),CV_LOAD_IMAGE_GRAYSCALE | CV_LOAD_IMAGE_ANYDEPTH);

	if(imageTMP.depth() == CV_16U){
		imageTMP.convertTo(image,CV_8U,1.0/256.0);
	}else{
		image = imageTMP.clone();
	}

	width = image.cols;
	height = image.rows;
	textureLoaded = false;
	image_reset = false;
}

Image::Image(Image *_image){
	color = _image->color;
	if(_image->image.depth() == CV_16U){
		_image->image.convertTo(image,CV_8U,1.0/256.0);
	}else{
		image = _image->image.clone();
	}

	width = image.cols;
	height = image.rows;
	textureLoaded = false;
	image_reset = false;
}

Image::~Image(){
	deleteTexture();
}

void Image::getImage(cv::Mat &_image){
	_image.release();
	_image = image.clone();
}

void Image::setImage(cv::Mat &_image, bool _color){
	color = _color;
	image.release();
	image = _image.clone();
	image_reset = true;
}

void Image::loadTexture(){
	if(!textureLoaded || image_reset){
		
		if(!textureLoaded)((QGLContext*) (GLSharedWidget::getInstance()->getQGLContext()))->makeCurrent();

		cv::Mat imageOut;	
		if(!color){
			imageOut.create( image.rows,  image.cols, CV_8UC(3) );
			cvtColor( image, imageOut, CV_GRAY2RGB);
		}else{
			imageOut = image.clone();
		}
		glEnable(GL_TEXTURE_2D);

		if(!textureLoaded){
			glGenTextures(1, &texture);
		}
			
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Set texture clamping method
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.cols, image.rows,
			0, GL_BGR, GL_UNSIGNED_BYTE,imageOut.ptr());       

		imageOut.release();
		textureLoaded = true;
		image_reset = false;
	}
}

void Image::bindTexture(){
	loadTexture();
	glBindTexture(GL_TEXTURE_2D, texture);
}

void Image::deleteTexture(){
	if(textureLoaded){
		GLSharedWidget::getInstance()->makeCurrent();
		glDeleteTextures(1, &texture);
	}
	textureLoaded = false;
}

void Image::save(QString filename){
	cv::imwrite(filename.toAscii().data(),image);
}
