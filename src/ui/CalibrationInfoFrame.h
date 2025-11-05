//  ----------------------------------
//  XMALab -- Copyright � 2015, Brown University, Providence, RI.
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
//  PROVIDED �AS IS�, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
//  FOR ANY PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR ANY 
//  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR FOR ANY DAMAGES WHATSOEVER RESULTING 
//  FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
//  OTHER TORTIOUS ACTION, OR ANY OTHER LEGAL THEORY, ARISING OUT OF OR IN CONNECTION 
//  WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
//  ----------------------------------
//  
///\file CalibrationInfoFrame.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef CALIBRATIONINFOFRAME_H_
#define CALIBRATIONINFOFRAME_H_

#include <QFrame>

namespace Ui
{
	class CalibrationInfoFrame;
}

namespace xma
{
	class Camera;
	class CameraViewWidget;
	
	class CalibrationInfoFrame : public QFrame
	{
		Q_OBJECT

	public:
		~CalibrationInfoFrame() override;
		explicit CalibrationInfoFrame(QWidget* parent = nullptr);

		void update(Camera* camera);
		void updateFrame(Camera* camera);
		void reset();
		
	public slots:
		void on_doubleSpinBoxBias_valueChanged(double value);
		void on_horizontalSliderBias_valueChanged(int value);
		void on_doubleSpinBoxScale_valueChanged(double value);
		void on_horizontalSliderScale_valueChanged(int value);
		
	private:
		Ui::CalibrationInfoFrame* frame;
		void getCameraInfo(Camera* camera, QString& CameraCenter, QString& FocalLength, QString& Distortion, QString& FramesCalibrated, QString& ErrorAllDist, QString& ErrorAllUndist);
		void getInfoFrame(Camera* camera, int frame, QString& ErrorCurrentDist, QString& ErrorCurrentUndist, QString& RotationVector, QString& TranslationVector);
		QString getInfoInlier(Camera* camera, int frame);

		CameraViewWidget * cameraWidget;
	};
}


#endif /* CALIBRATIONINFOFRAME_H_ */

