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
///\file MarkerFrameData.cpp
///\author Benjamin Knorlein
///\date 6/30/2017

#pragma once

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "MarkerFrameData.h"

using namespace xma;

MarkerFrameData::MarkerFrameData(int number_cameras)
: number_cameras_(number_cameras), data_per_camera_(new MarkerFrameDataCam[number_cameras_])
{
}

MarkerFrameData::MarkerFrameData(const MarkerFrameData& other) :
number_cameras_(other.number_cameras_), data_per_camera_(new MarkerFrameDataCam[other.number_cameras_])
, position3d_(std::move(other.position3d_)), status3d_(other.status3d_)
{
	std::copy(other.data_per_camera_.get(), other.data_per_camera_.get() + other.number_cameras_ * sizeof(MarkerFrameDataCam), data_per_camera_.get());
}

MarkerFrameData::MarkerFrameData(MarkerFrameData&& other):
number_cameras_(other.number_cameras_), data_per_camera_(std::move(other.data_per_camera_))
, position3d_(other.position3d_), status3d_(other.status3d_)
{
}

MarkerFrameData& MarkerFrameData::operator=(MarkerFrameData& other)
{
	if (&other == this)
		return *this;

	position3d_ = other.position3d_;
	status3d_ = other.status3d_;

	// reuse storage when possible
	if (number_cameras_ != other.number_cameras_)
	{
		number_cameras_ = other.number_cameras_;
		data_per_camera_.reset(new MarkerFrameDataCam[number_cameras_]);
	}

	//copy Data
	std::copy(other.data_per_camera_.get(), other.data_per_camera_.get() + other.number_cameras_ * sizeof(MarkerFrameDataCam), data_per_camera_.get());
	return *this;
}

MarkerFrameData& MarkerFrameData::operator=(MarkerFrameData&& other)
{
	if (&other == this)
		return *this;

	position3d_ = std::move(other.position3d_);
	status3d_ = other.status3d_;
	number_cameras_ = other.number_cameras_;
	data_per_camera_ = std::move(other.data_per_camera_);

	return *this;
}
