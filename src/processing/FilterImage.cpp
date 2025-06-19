//  ----------------------------------
//  XMALab -- Copyright Â(c) 2015, Brown University, Providence, RI.
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
//  PROVIDED â€œAS ISâ€�, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
//  FOR ANY PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR ANY 
//  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR FOR ANY DAMAGES WHATSOEVER RESULTING 
//  FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
//  OTHER TORTIOUS ACTION, OR ANY OTHER LEGAL THEORY, ARISING OUT OF OR IN CONNECTION 
//  WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
//  ----------------------------------
//  
///\file FilterImage.cpp
///\author Benjamin Knorlein
///\date 10/15/2019

#pragma once

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "FilterImage.h"
#include <opencv2/imgproc/types_c.h>
#include <opencv2/imgproc/imgproc.hpp>
//#include <opencv2/contrib/contrib.hpp>
#include "core/Settings.h"

using namespace xma;

cv::Mat FilterImage::run(cv::Mat& ImageIn)
{
	int krad = Settings::getInstance()->getIntSetting("VisualFilter_krad");
	krad = 2 * krad + 1;
	float gsigma = Settings::getInstance()->getFloatSetting("VisualFilter_gsigma");
	float img_wt = Settings::getInstance()->getFloatSetting("VisualFilter_img_wt");
	float blur_wt = Settings::getInstance()->getFloatSetting("VisualFilter_blur_wt");
	float gamma = Settings::getInstance()->getFloatSetting("VisualFilter_gamma");
	// Make blur mask
	cv::Mat img_gblur;
	GaussianBlur(ImageIn, img_gblur, cv::Size(krad, krad), gsigma);

	// Subtract blur from original to produce sharp
	cv::Mat img_addwt;
	addWeighted(ImageIn, img_wt, img_gblur, blur_wt, 0, img_addwt);

	// Gamma correction to enhance contrast
	cv::Mat img_gamma;
	unsigned char lut[256];
	for (int i = 0; i < 256; i++)
	{
		lut[i] = cv::saturate_cast<uchar>(pow((float)(i / 255.0), gamma) * 255.0f);
	}

	img_gamma = img_addwt.clone();
	const int channels = img_gamma.channels();
	switch (channels)
	{
		case 1:
		{
			cv::MatIterator_<uchar> it, end;
			for (it = img_gamma.begin<uchar>(), end = img_gamma.end<uchar>(); it != end; it++)
				*it = lut[(*it)];
			break;
		}
		case 3:
		{
			cv::MatIterator_<cv::Vec3b> it, end;
			for (it = img_gamma.begin<cv::Vec3b>(), end = img_gamma.end< cv::Vec3b>(); it != end; it++)
			{
				(*it)[0] = lut[((*it)[0])];
				(*it)[1] = lut[((*it)[1])];
				(*it)[2] = lut[((*it)[2])];
			}
			break;
		}
	}
	return img_gamma;
}
