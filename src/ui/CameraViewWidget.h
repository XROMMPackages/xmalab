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
///\file CameraViewWidget.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef CAMERAVIEWWIDGET_H
#define CAMERAVIEWWIDGET_H

#include <QWidget>
#include <QString>
#include "ui/State.h"

//forward declarations
namespace Ui
{
	class CameraViewWidget;
}

namespace xma
{
	class Camera;
	class CalibrationInfoFrame;
	class UndistortionInfoFrame;
	class DigitizationInfoFrame;

	class CameraViewWidget : public QWidget
	{
		Q_OBJECT

	public:
		explicit CameraViewWidget(Camera* camera, QWidget* parent = nullptr);
		~CameraViewWidget() override;

		void updateCamera();
		void setMinimumWidthGL(bool set);

		void draw();
		void updateInfo();

		void setImageName(const QString& name);

		void setBias(double value);
		void setScale(double value);
		void setTransparency(double value);
		void setRenderTransparentModels(bool value);

		void centerViewToPoint();

		bool isVisible() const;
		void setIsVisible(bool value);

	protected:
		bool eventFilter(QObject* obj, QEvent* event) override;

	public slots :
		void on_toolButtonFitZoom_clicked(bool checked);
		void on_spinBoxZoom_valueChanged(int value);
		void on_toolButtonInfo_clicked(bool checked);

		void autozoomChanged(bool on);
		void zoomChanged(int zoom);
		void transparencyChanged(double value);

		void workspaceChanged(work_state workspace);
		void activeFrameCalibrationChanged(int activeFrame);
		void activeCameraChanged(int activeCamera);

	private:

		bool m_visible;
		UndistortionInfoFrame* undistortionFrame;
		CalibrationInfoFrame* calibrationFrame;
		DigitizationInfoFrame* digitizationFrame;
		Camera* camera;

		Ui::CameraViewWidget* widget;

		QString cameraName;
	};
}

#endif // CAMERAVIEWWIDGET_H


