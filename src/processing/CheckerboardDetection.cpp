//  ----------------------------------
//  XMA Lab -- Copyright © 2015, Brown University, Providence, RI.
//  
//  All Rights Reserved
//   
//  Use of the XMA Lab software is provided under the terms of the GNU General Public License version 3 
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
///\file CheckerboardDetection.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "processing/CheckerboardDetection.h" 

#include "ui/ProgressDialog.h"
#include "ui/MainWindow.h"

#include "core/Project.h"
#include "core/Camera.h"
#include "core/Image.h"
#include "core/CalibrationImage.h"
#include "core/CalibrationObject.h"
#include "core/Settings.h"

#include <QtCore>

using namespace xma;

int CheckerboardDetection::nbInstances = 0;

CheckerboardDetection::CheckerboardDetection(int camera, int image) : QObject()
{
	m_camera = camera;
	m_image = image;
	nbInstances++;
}

CheckerboardDetection::~CheckerboardDetection()
{
}

void CheckerboardDetection::detectCorner()
{
	m_FutureWatcher = new QFutureWatcher<void>();
	connect(m_FutureWatcher, SIGNAL(finished()), this, SLOT(detectCorner_threadFinished()));

	QFuture<void> future = QtConcurrent::run(this, &CheckerboardDetection::detectCorner_thread);
	m_FutureWatcher->setFuture(future);

	ProgressDialog::getInstance()->showProgressbar(0, 0, "Detect Corner");
}

void CheckerboardDetection::detectCorner_thread()
{
	tmpPoints.clear();
	cv::Mat image;

	Project::getInstance()->getCameras()[m_camera]->getCalibrationImages()[m_image]->getImage()->getImage(image);

	findChessboardCorners(image, cv::Size(CalibrationObject::getInstance()->getNbHorizontalSquares(), CalibrationObject::getInstance()->getNbVerticalSquares()), tmpPoints,
	                      cv::CALIB_CB_ADAPTIVE_THRESH);

	image.release();
}

void CheckerboardDetection::detectCorner_threadFinished()
{
	if (tmpPoints.size() == CalibrationObject::getInstance()->getNbHorizontalSquares() * CalibrationObject::getInstance()->getNbVerticalSquares())
		Project::getInstance()->getCameras()[m_camera]->getCalibrationImages()[m_image]->setDetectedPoints(tmpPoints);

	tmpPoints.clear();
	delete m_FutureWatcher;
	nbInstances--;
	if (nbInstances == 0)
	{
		ProgressDialog::getInstance()->closeProgressbar();
		MainWindow::getInstance()->redrawGL();
		emit detectCorner_finished();
	}
	delete this;
}

