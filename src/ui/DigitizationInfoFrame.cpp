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
///\file DigitizationInfoFrame.cpp
///\author Benjamin Knorlein
///\date 01/08/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/DigitizationInfoFrame.h"
#include "ui_DigitizationInfoFrame.h"

#include "core/Camera.h"
#include "ui/CameraViewWidget.h"

using namespace xma;

DigitizationInfoFrame::DigitizationInfoFrame(QWidget* parent) :
	QFrame(parent),
	frame(new Ui::DigitizationInfoFrame)
{

	frame->setupUi(this);
	cameraWidget = ((CameraViewWidget *) parent);
}

void DigitizationInfoFrame::reset()
{
	frame->horizontalSliderTransparency->setValue(50.0);
	frame->horizontalSliderBias->setValue(0.0);
	frame->horizontalSliderScale->setValue(100.0);
	frame->doubleSpinBoxTransparency->setValue(0.5);
	frame->doubleSpinBoxBias->setValue(0.0);
	frame->doubleSpinBoxScale->setValue(1.0);
	frame->checkBoxTransparentModels->setChecked(true);
	cameraWidget->setScale(1.0);
	cameraWidget->setBias(0.0);
	cameraWidget->setTransparency(0.5);
	cameraWidget->setRenderTransparentModels(true);
}

DigitizationInfoFrame::~DigitizationInfoFrame()
{
	delete frame;
}


void DigitizationInfoFrame::on_doubleSpinBoxBias_valueChanged(double value)
{
	if (frame->horizontalSliderBias->value() != value * 100)
	{
		frame->horizontalSliderBias->setValue(value * 100);
		cameraWidget->setBias(value);
	}
}

void DigitizationInfoFrame::on_horizontalSliderBias_valueChanged(int value)
{
	if (frame->doubleSpinBoxBias->value() * 100 != value)
	{
		frame->doubleSpinBoxBias->setValue(0.01 * value);
		cameraWidget->setBias(0.01 * value);
	}
}

void DigitizationInfoFrame::on_doubleSpinBoxScale_valueChanged(double value)
{
	if (frame->horizontalSliderScale->value() != value * 100)
	{
		frame->horizontalSliderScale->setValue(value * 100);
		cameraWidget->setScale(value);
	}
}

void DigitizationInfoFrame::on_horizontalSliderScale_valueChanged(int value)
{
	if (frame->doubleSpinBoxScale->value() * 100 != value)
	{
		frame->doubleSpinBoxScale->setValue(0.01 * value);
		cameraWidget->setScale(0.01 * value);
	}
}

void DigitizationInfoFrame::on_doubleSpinBoxTransparency_valueChanged(double value)
{
	if (frame->horizontalSliderTransparency->value() != value * 100)
	{
		frame->horizontalSliderTransparency->setValue(value * 100);
		cameraWidget->setTransparency(value);
	}
}

void DigitizationInfoFrame::on_horizontalSliderTransparency_valueChanged(int value)
{
	if (frame->doubleSpinBoxTransparency->value() * 100 != value)
	{
		frame->doubleSpinBoxTransparency->setValue(0.01 * value);
		cameraWidget->setTransparency(0.01 * value);
	}
}

void DigitizationInfoFrame::on_checkBoxTransparentModels_clicked()
{
	cameraWidget->setRenderTransparentModels(frame->checkBoxTransparentModels->isChecked());
}

void DigitizationInfoFrame::on_pushButtonReset_clicked()
{
	reset();
}
