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
///\file MarkerFrameDataCam.h
///\author Benjamin Knorlein
///\date 6/30/2017

#pragma once

#ifndef MARKERFRAMEDATACAM_H
#define MARKERFRAMEDATACAM_H
#include <opencv2/core/core.hpp>

enum markerStatus
{
	UNTRACKABLE = -30,
	DELETED = -20,
	LOST = -10,
	UNDEFINED = 0,
	PREDICTED = 10,
	INTERPOLATED = 20,
	TRACKED = 40,
	TRACKED_AND_OPTIMIZED = 45,
	SET = 50,
	SET_AND_OPTIMIZED = 55,
	MANUAL = 60,
	MANUAL_AND_OPTIMIZED = 65
};

enum interpolationMethod
{
	NONE = 0,
	REPEAT = 10,
	LINEAR = 20,
	CUBIC = 30
};

namespace xma
{
	class MarkerFrameDataCam
	{
		public:

	public:
		const cv::Point2d& position() const
		{
			return position_;
		}

		void set_position(const cv::Point2d& position)
		{
			position_ = position;
		}

		const cv::Point2d& projected() const
		{
			return projected_;
		}

		void set_projected(const cv::Point2d& projected)
		{
			projected_ = projected;
		}

		const markerStatus& status() const
		{
			return status_;
		}

		void set_status(markerStatus status)
		{
			status_ = status;
		}

		const double& error() const
		{
			return error_;
		}

		void set_error(double error)
		{
			error_ = error;
		}

		const interpolationMethod& interpolation() const
		{
			return interpolation_;
		}

		void set_interpolation(interpolationMethod interpolation)
		{
			interpolation_ = interpolation;
		}

	private:
			cv::Point2d  position_;
			cv::Point2d  projected_;
			markerStatus status_;
			double error_;
			interpolationMethod interpolation_;
	};
}
#endif // MARKERFRAMEDATACAM_H
