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
///\file Image.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "Image.h"
#include "Camera.h"
#include "ui/GLSharedWidget.h"

#include <opencv/highgui.h>

#include <QFileInfo>
#include "Project.h"

#ifndef GL_BGR
#define GL_BGR 0x80E0
#endif

using namespace xma;

Image::Image(QString _imageFileName)
{
	cv::Mat imageTMP;

	imageTMP = cv::imread(_imageFileName.toAscii().data(), CV_LOAD_IMAGE_ANYCOLOR | CV_LOAD_IMAGE_ANYDEPTH);

	if (imageTMP.channels()> 1){
		colorImage_set = COLOR_ORIGINAL;

		if (imageTMP.depth() == CV_16U)
		{
			imageTMP.convertTo(image_color, CV_8UC3, 1.0 / 256.0);
		}
		else
		{
			image_color = imageTMP.clone();
		}
		cvtColor(image_color, image, CV_RGB2GRAY);
	} 
	else
	{
		colorImage_set = GRAY;
		if (imageTMP.depth() == CV_16U)
		{
			imageTMP.convertTo(image, CV_8U, 1.0 / 256.0);
		}
		else
		{
			image = imageTMP.clone();
		}
	}


	if (Project::getInstance()->getFlipImages())
	{
		cv::flip(image, image, 1);
		if (colorImage_set)cv::flip(image_color, image_color, 1);
	}

	width = image.cols;
	height = image.rows;
	textureLoaded = false;
	image_reset = false;
	imageTMP.release();
	texture = 0;
}

Image::Image(Image* _image)
{
	colorImage_set = _image->colorImage_set;
	if (_image->image.depth() == CV_16U)
	{
		_image->image.convertTo(image,CV_8U, 1.0 / 256.0);
	}
	else
	{
		image = _image->image.clone();
	}

	if (colorImage_set > GRAY)
	{
		if (_image->image_color.depth() == CV_16U)
		{
			_image->image_color.convertTo(image_color, CV_8UC3, 1.0 / 256.0);
		}
		else
		{
			image_color = _image->image_color.clone();
		}
	}

	width = image.cols;
	height = image.rows;
	textureLoaded = false;
	image_reset = false;

	texture = 0;
}


Image::~Image()
{
	deleteTexture();
}

void Image::getImage(cv::Mat& _image, bool color)
{
	_image.release();
	if (color &&  colorImage_set > COLOR_CONVERTED)
	{
		_image = image_color.clone();
	}
	else{
		_image = image.clone();
	}
}

void Image::getSubImage(cv::Mat& _image, int size, int off_x, int off_y)
{
	_image.release();
	cv::Size img_size(2 * size + 1, 2 * size + 1);
	cv::Point2f center(off_x + size, off_y + size);
	cv::getRectSubPix(image, img_size, center, _image);
}

void Image::getSubImage(cv::Mat& _image, int size, double x, double y)
{
	_image.release();
	cv::Size img_size(2 * size + 1, 2 * size + 1);
	cv::Point2f center(x, y);
	cv::getRectSubPix(image, img_size, center, _image);
}

void Image::setImage(cv::Mat& _image, bool _color)
{
	image.release();
	if (_color)
	{
		colorImage_set = COLOR_ORIGINAL;
		image_color.release();
		image_color = _image.clone();
		cvtColor(image_color, image, CV_RGB2GRAY);
	}
	else{
		colorImage_set = GRAY;
		image = _image.clone();
	}

	width = image.cols;
	height = image.rows;
	image_reset = true;
}

void Image::setImage(QString imageFileName)
{	
	image.release();
	cv::Mat imageTMP;
	imageTMP = cv::imread(imageFileName.toAscii().data(), CV_LOAD_IMAGE_ANYCOLOR | CV_LOAD_IMAGE_ANYDEPTH);

	if (imageTMP.channels()> 1){
		colorImage_set = COLOR_ORIGINAL;

		if (imageTMP.depth() == CV_16U)
		{
			imageTMP.convertTo(image_color, CV_8UC3, 1.0 / 256.0);
		}
		else
		{
			image_color = imageTMP.clone();
		}
		cvtColor(image_color, image, CV_RGB2GRAY);
	}
	else
	{
		colorImage_set = GRAY;
		if (imageTMP.depth() == CV_16U)
		{
			imageTMP.convertTo(image, CV_8U, 1.0 / 256.0);
		}
		else
		{
			image = imageTMP.clone();
		}
	}

	if (Project::getInstance()->getFlipImages()){
		cv::flip(image, image, 1);
		if (colorImage_set)cv::flip(image_color, image_color, 1);
	}
	image_reset = true;
}

void Image::loadTexture()
{
	if (!textureLoaded || image_reset)
	{
		if (!textureLoaded)((QGLContext*) (GLSharedWidget::getInstance()->getQGLContext()))->makeCurrent();

		if (colorImage_set == GRAY)
		{
			image_color.create(image.rows, image.cols, CV_8UC(3));
			cvtColor(image, image_color, CV_GRAY2RGB);
			colorImage_set = COLOR_CONVERTED;
		}

		glEnable(GL_TEXTURE_2D);

		if (!textureLoaded)
		{
			glGenTextures(1, &texture);
		}

		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Set texture clamping method
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.cols, image.rows,
			0, GL_BGR, GL_UNSIGNED_BYTE, image_color.ptr());

		textureLoaded = true;
		image_reset = false;
	}
}

void Image::bindTexture()
{
	loadTexture();
	glBindTexture(GL_TEXTURE_2D, texture);
}

void Image::deleteTexture()
{
	if (textureLoaded)
	{
		GLSharedWidget::getInstance()->makeCurrent();
		glDeleteTextures(1, &texture);
	}
	textureLoaded = false;
}

void Image::save(QString filename)
{
	if (colorImage_set > COLOR_CONVERTED)
	{
		if (Project::getInstance()->getFlipImages())
		{
			cv::Mat image_tmp;
			cv::flip(image_color, image_tmp, 1);
			cv::imwrite(filename.toAscii().data(), image_tmp);
		}
		else{
			cv::imwrite(filename.toAscii().data(), image_color);
		}
	}
	else{
		if (Project::getInstance()->getFlipImages())
		{
			cv::Mat image_tmp;
			cv::flip(image, image_tmp, 1);
			cv::imwrite(filename.toAscii().data(), image_tmp);
		}
		else{
			cv::imwrite(filename.toAscii().data(), image);
		}
	}
}

