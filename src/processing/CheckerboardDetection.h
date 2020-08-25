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
///\file CheckerboardDetection.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef CHECKERBOARDDETECTION_H
#define CHECKERBOARDDETECTION_H

#include <QFutureWatcher>
#include <QObject>

#include <opencv2/opencv.hpp>

namespace xma
{
	class CheckerboardDetection : public QObject
	{
		Q_OBJECT;

	public:
		CheckerboardDetection(int camera, int image);
		CheckerboardDetection(int camera, int image, cv::Point2d references[4]);
		virtual ~CheckerboardDetection();
		void detectCorner();

		int m_camera;
		//if -1 undistortion;
		int m_image;
		std::vector<cv::Point2d> tmpPoints;
		std::vector<cv::Point2f> selectedPoints;
		std::vector<cv::Point2f> gridPoints;

		bool viaHomography;

		static bool isRunning()
		{
			return (nbInstances > 0);
		}

		signals:
		void detectCorner_finished();

	private slots:
		void detectCorner_threadFinished();

	private:
		void detectCorner_thread();
		QFutureWatcher<void>* m_FutureWatcher;
		static int nbInstances;
	};
}
#endif // BLOBDETECTION_H


