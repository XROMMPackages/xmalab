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
///\file WizardDockWidget.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef WIZARDDOCKWIDGET_H_
#define WIZARDDOCKWIDGET_H_

#include <QDockWidget>
#include "ui/State.h"

namespace Ui
{
	class WizardDockWidget;
}

class QLabel;

namespace xma
{
	class WizardUndistortionFrame;
	class WizardCalibrationCubeFrame;
	class WizardDigitizationFrame;

	class WizardDockWidget : public QDockWidget
	{
		Q_OBJECT

	public:
		virtual ~WizardDockWidget();

		void updateWizard();
		static WizardDockWidget* getInstance();
		bool manualCalibrationRunning();
		void addCalibrationReference(double x, double y);
		void addDigitizationPoint(int camera, double x, double y);
		void selectDigitizationPoint(int camera, double x, double y);
		void moveDigitizationPoint(int camera, double x, double y, bool noDetection);
		void draw();
		bool checkForPendingChanges();
		void updateDialog();
		void stop();

	private:
		Ui::WizardDockWidget* dock;
		static WizardDockWidget* instance;
		WizardDockWidget(QWidget* parent = 0);

		WizardUndistortionFrame* undistortionFrame;
		WizardCalibrationCubeFrame* calibrationFrame;
		WizardDigitizationFrame* digitizationFrame;

	public slots:
		void workspaceChanged(work_state workspace);
		void trackSelectedPointForward();
		void trackSelectedPointBackward();
		void goToLastTrackedFrame();
		void goToFirstTrackedFrame();
		void interpolateActive();
		void interpolateAll();
	};
}


#endif /* WIZARDUNDISTORTIONFRAME_H_ */

