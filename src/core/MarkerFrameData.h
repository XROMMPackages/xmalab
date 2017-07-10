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
///\file MarkerFrameData.h
///\author Benjamin Knorlein
///\date 6/30/2017

#pragma once

#ifndef MARKERFRAMEDATA_H
#define MARKERFRAMEDATA_H
#include <memory>

#include "MarkerFrameDataCam.h"

namespace xma
{
	class MarkerFrameData
	{
	public:
		//no default constructor
		MarkerFrameData() = delete;
		//constructor
		explicit MarkerFrameData(int number_cameras);
		// copy constructor
		MarkerFrameData(const MarkerFrameData& other);
		// move constructor
		MarkerFrameData(MarkerFrameData&& other);

		// copy operator
		MarkerFrameData& operator=(MarkerFrameData& other);
		// move operator
		MarkerFrameData& operator=(MarkerFrameData&& other);
		
		//begin and end to allow for range loops
		MarkerFrameDataCam* begin() { return data_per_camera_.get(); }
		MarkerFrameDataCam* end() { return data_per_camera_.get() + number_cameras_ * sizeof(MarkerFrameDataCam); }
		MarkerFrameDataCam* operator [](unsigned int idx) { return (idx < number_cameras_) ? &data_per_camera_.get()[idx] : nullptr; }

		//getter and setter
		const cv::Point3d& position3_d() const
		{
			return position3d_;
		}

		void set_position3_d(const cv::Point3d& position3_d)
		{
			position3d_ = position3_d;
		}

		const markerStatus& status3_d() const
		{
			return status3d_;
		}

		void set_status3_d(markerStatus status3_d)
		{
			status3d_ = status3_d;
		}

		const int& number_cameras() const
		{
			return number_cameras_;
		}

	private:

		//Dynamic allocation for the data per camera
		int number_cameras_;
		std::unique_ptr <MarkerFrameDataCam> data_per_camera_;

		//marker location in 3d
		cv::Point3d position3d_;

		//status of 3d position of the marker
		markerStatus status3d_;
	};
}
#endif // MARKERFRAMEDATA_H
