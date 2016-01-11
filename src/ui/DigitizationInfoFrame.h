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
///\file DigitizationInfoFrame.h
///\author Benjamin Knorlein
///\date 01/08/2015

#ifndef DIGITIZATIONINFOFRAME_H_
#define DIGITIZATIONINFOFRAME_H_

#include <QFrame>

namespace Ui
{
	class DigitizationInfoFrame;
}

namespace xma
{;
	class CameraViewWidget;

	class DigitizationInfoFrame : public QFrame
	{
		Q_OBJECT

	public:
		virtual ~DigitizationInfoFrame();
		DigitizationInfoFrame(QWidget* parent = 0);

		void reset();

	private:
		Ui::DigitizationInfoFrame* frame;
		CameraViewWidget * cameraWidget;
	public slots:
		void on_doubleSpinBoxBias_valueChanged(double value);
		void on_horizontalSliderBias_valueChanged(int value);
		void on_doubleSpinBoxScale_valueChanged(double value);
		void on_horizontalSliderScale_valueChanged(int value);
		void on_pushButtonReset_clicked();
	};
}


#endif /* DIGITIZATIONINFOFRAME_H_ */

