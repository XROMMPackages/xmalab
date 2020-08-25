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
///\file Calibration.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef CALIBRATION_H
#define CALIBRATION_H

#include "processing/ThreadedProcessing.h"

#include <QFutureWatcher>
#include <QObject>

#include <opencv2/opencv.hpp>

namespace xma
{
	class Calibration : public ThreadedProcessing
	{
		Q_OBJECT;

	public:
		Calibration(int camera, bool planar = false);
		virtual ~Calibration();

	protected:
		void process() override;
		void process_finished() override;

	private:
		int m_camera;
		bool m_planar;

		void setInitialByReferences();

		QFutureWatcher<void>* m_FutureWatcher;
		static int nbInstances;

		cv::Mat distortion_coeffs;
		cv::Mat intrinsic_matrix;

		std::vector<std::vector<cv::Point3f> > object_points;
		std::vector<std::vector<cv::Point2f> > image_points;

		cv::Mat rotationvector;
		cv::Mat translationvector;

		std::vector<cv::Point2d> projectedPointsUndistorted;
		std::vector<double> error;
	};
}
#endif // CALIBRATION_H


