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
///\file WizardDigitizationFrame.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef WIZARDDIGITIZATIONFRAME_H_
#define WIZARDDIGITIZATIONFRAME_H_

#include <QFrame>
#include "ui/State.h"

namespace Ui
{
	class WizardDigitizationFrame;
}

namespace xma
{
	class WizardDigitizationFrame : public QFrame
	{
		Q_OBJECT

	public:
		virtual ~WizardDigitizationFrame();
		WizardDigitizationFrame(QWidget* parent = 0);

		void addDigitizationPoint(int camera, double x, double y);
		void selectDigitizationPoint(int camera, double x, double y);
		void moveDigitizationPoint(int camera, double x, double y, bool noDetection);

		void setDialog();
		void stopTracking();

		void trackSelectedPointToNextFrame();
		void trackSelectedPointToPrevFrame();
		void goToLastTrackedFrame();
		void goToFirstTrackedFrame();
	private:
		Ui::WizardDigitizationFrame* frame;

		int trackID; //id for point or rb
		int tmptrackID;
		int trackType; //1 point, 2 rb, 2 all
		int trackDirection; // -2 back -1 prev, 0 none , 1 next, 2 forward
		bool singleTrack;
		//bool isTracking;

		void trackSinglePoint();
		void trackRB();
		void trackAll();

		void uncheckTrackButtons();

	public slots:
		void activeCameraChanged(int activeCamera);
		void activeFrameTrialChanged(int activeFrame);
		void workspaceChanged(work_state workspace);

		void on_pushButton_clicked();
		void on_pushButton_PointNext_clicked();
		void on_pushButton_PointPrev_clicked();
		void on_pushButton_PointForw_clicked(bool checked);
		void on_pushButton_PointBack_clicked(bool checked);

		void on_pushButton_RBNext_clicked();
		void on_pushButton_RBPrev_clicked();
		void on_pushButton_RBForw_clicked(bool checked);
		void on_pushButton_RBBack_clicked(bool checked);

		void on_pushButton_AllNext_clicked();
		void on_pushButton_AllPrev_clicked();
		void on_pushButton_AllForw_clicked(bool checked);
		void on_pushButton_AllBack_clicked(bool checked);

		void trackSinglePointFinished();
		void trackRBFinished();
		void trackAllFinished();

		void checkIfValid();
		void track();
	};
}


#endif /* WIZARDDIGITIZATIONFRAME_H_ */

