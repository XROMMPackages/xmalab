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
///\file CameraViewDetailWidget.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef CAMERAVIEWDETAILWIDGET_H
#define CAMERAVIEWDETAILWIDGET_H

#include <QWidget>
#include <QString>
#include "ui/State.h"

//forward declarations
namespace Ui
{
	class CameraViewDetailWidget;
}

namespace xma
{
	class Camera;

	class CameraViewDetailWidget : public QWidget
	{
		Q_OBJECT

	public:
		explicit CameraViewDetailWidget(Camera* camera, QWidget* parent = nullptr);
		~CameraViewDetailWidget() override;

		void updateCamera();
		void setMinimumWidthGL(bool set);
		void draw();
		void centerViews();

		bool getIsVisible() const;
		void setIsVisible(bool value);

	public slots:
		void on_doubleSpinBoxBias_valueChanged(double value);
		void on_horizontalSliderBias_valueChanged(int value);
		void on_doubleSpinBoxScale_valueChanged(double value);
		void on_horizontalSliderScale_valueChanged(int value);

		void workspaceChanged(work_state workspace);
		void activeFrameTrialChanged(int);
		void activeTrialChanged(int);
		void activePointChanged(int);

	private:
		Camera* camera;

		Ui::CameraViewDetailWidget* widget;

		bool m_visible;
	};
}

#endif // CAMERAVIEWDETAILWIDGET_H


