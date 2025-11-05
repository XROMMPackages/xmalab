//  ----------------------------------
//  XMALab -- Copyright (c) 2015, Brown University, Providence, RI.
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
//  PROVIDED "AS IS", INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
//  FOR ANY PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR ANY 
//  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR FOR ANY DAMAGES WHATSOEVER RESULTING 
//  FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
//  OTHER TORTIOUS ACTION, OR ANY OTHER LEGAL THEORY, ARISING OUT OF OR IN CONNECTION 
//  WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
//  ----------------------------------
//  
///\file MarkerTracking.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef MARKERTRACKING_H
#define MARKERTRACKING_H

#include <QFutureWatcher>
#include <QObject>

#include <opencv2/opencv.hpp>

namespace xma
{
	class MarkerTracking : public QObject
	{
		Q_OBJECT;

	public:
		MarkerTracking(int camera, int trial, int frame_from, int frame_to, int marker, bool forward);
		virtual ~MarkerTracking();
		void trackMarker();

		static bool isRunning()
		{
			return (nbInstances > 0);
		}

		signals:
		void trackMarker_finished();

	private slots:
		void trackMarker_threadFinished();

	private:
		void trackMarker_thread();
		QFutureWatcher<void>* m_FutureWatcher;
		static int nbInstances;

		int m_camera;
		int m_frame_from;
		int m_frame_to;
		int m_trial;
		int m_marker;
		bool m_forward;

		double x_from;
		double y_from;

		double x_to;
		double y_to;

		int size;
		int searchArea;
		int maxPenalty;

		cv::Mat templ;
		cv::UMat templ_umat;
		cv::UMat roi_buffer;
		cv::UMat result_buffer;
		cv::UMat springforce_buffer;
	};
}
#endif // MARKERTRACKING_H


