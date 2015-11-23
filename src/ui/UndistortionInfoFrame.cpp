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
///\file UndistortionInfoFrame.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/UndistortionInfoFrame.h"
#include "ui_UndistortionInfoFrame.h"

#include "core/Camera.h"
#include "core/UndistortionObject.h"

using namespace xma;

UndistortionInfoFrame::UndistortionInfoFrame(QWidget* parent) :
	QFrame(parent),
	frame(new Ui::UndistortionInfoFrame)
{
	frame->setupUi(this);
}

UndistortionInfoFrame::~UndistortionInfoFrame()
{
	delete frame;
}

void UndistortionInfoFrame::update(Camera* camera)
{
	if (camera->hasUndistortion() && camera->getUndistortionObject() && camera->getUndistortionObject()->isComputed())
	{
		if (camera->getUndistortionObject()->isUpdateInfoRequired())
		{
			QString inlier;
			QString error;

			getInfo(camera, inlier, error);
			frame->label_Error->setText(error);
			frame->label_NbPoints->setText(inlier);
			camera->getUndistortionObject()->setUpdateInfoRequired(false);
		}
	}
	else
	{
		frame->label_Error->setText("");
		frame->label_NbPoints->setText("");
	}
}

void UndistortionInfoFrame::getInfo(Camera* camera, QString& inlier_string, QString& error_string)
{
	inlier_string.clear();
	error_string.clear();

	int count = 0;
	double mean = 0;
	for (unsigned int i = 0; i < camera->getUndistortionObject()->getInlier().size(); i++)
	{
		if (camera->getUndistortionObject()->getInlier()[i])
		{
			count ++;
			mean += camera->getUndistortionObject()->getError()[i];
		}
	}
	if (count > 0) mean = mean / count;

	count = 0;
	double sd = 0;
	for (unsigned int i = 0; i < camera->getUndistortionObject()->getInlier().size(); i++)
	{
		if (camera->getUndistortionObject()->getInlier()[i])
		{
			count ++;
			sd += pow(camera->getUndistortionObject()->getError()[i] - mean, 2);
		}
	}

	if (count > 0) sd = sqrt(sd / (count - 1));

	inlier_string = QString::number(count) + " of " + QString::number(camera->getUndistortionObject()->getInlier().size());
	error_string = QString::number(mean, 'f', 2) + " +/- " + QString::number(sd, 'f', 2);
}

