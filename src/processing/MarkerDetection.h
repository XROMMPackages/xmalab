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
///\file MarkerDetection.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef MARKERDETECTION_H
#define MARKERDETECTION_H

#include <QFutureWatcher>
#include <QObject>

#include <opencv2/opencv.hpp>

namespace xma
{
	class Image;

	class MarkerDetection : public QObject
	{
		Q_OBJECT;

	public:
		MarkerDetection(int camera, int trial, int frame, int marker, double searcharea = 30.0, bool refinementAfterTracking = false);
		virtual ~MarkerDetection();
		void detectMarker();

		static bool isRunning()
		{
			return (nbInstances > 0);
		}

		static cv::Point2d detectionPoint(Image* image, int method, cv::Point2d center, int searchArea, int masksize, double threshold = 8, double* size = NULL, std::vector <cv::Mat> * images = NULL, bool drawCrosshairs = false);

		static bool refinePointPolynomialFit(cv::Point2d& pt, double& radius, bool darkMarker, int camera, int trial);

		signals:
		void detectMarker_finished();

	private slots:
		void detectMarker_threadFinished();

	private:
		void detectMarker_thread();
		QFutureWatcher<void>* m_FutureWatcher;
		static int nbInstances;

		int m_camera;
		int m_frame;
		int m_trial;
		int m_marker;
		int m_method;
		double m_x, m_y;
		double m_size;
		double m_input_size;
		uchar m_thresholdOffset;

		int m_searchArea;
		bool m_refinementAfterTracking;
	};
}
#endif // MARKERDETECTION_H


