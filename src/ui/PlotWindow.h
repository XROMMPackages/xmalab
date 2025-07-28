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
///\file PlotWindow.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef PLOTWINDOW_H_
#define PLOTWINDOW_H_

#include <QDockWidget>
#include "ui/State.h"
#include "external/QCustomPlot/qcustomplot.h"
#include "ui/PointsDockWidget.h"
#include <algorithm> // for std::clamp
#include <cmath>     // for std::lround

class QComboBox;
class QLabel;
class QFrame;
class QPushButton;

namespace Ui
{
	class PlotWindow;
}

namespace xma
{
	class Marker;
	class RigidBody;

	class PlotWindow : public QDockWidget
	{
		Q_OBJECT

	public:
		public:
		~PlotWindow() override;
		static PlotWindow* getInstance();

		void updateMarkers(bool rememberSelection);
		void draw();
		void resetRange(bool recreateStatus = true);
		void updateTimeCheckBox();

		void ShiftPressed();
		void ShiftReleased();

	private:
		void plot2D(int idx1);
		void plot3D(int idx1);
		void plotDistance(int idx1, int idx2);
		void plotReprojectionError(int idx1);
		void plotRigidBody(int idx);
		void plotRigidBodyError(int idx);
		void installEventFilterToChildren(QObject* widget);
		void saveData();

		bool isFrameAboveError(Marker * marker, int frame);
		bool isFrameAboveError(RigidBody* body, int frame);

		// Helper to convert plot x coordinate to frame index (Qt6 safe)
		int getFrameFromPlotX(double x) const;

	protected:
		bool eventFilter(QObject* target, QEvent* event) override;
		void closeEvent(QCloseEvent* event) override;

	private:
		explicit PlotWindow(QWidget* parent = nullptr);
		static PlotWindow* instance;
		void deleteData();
		void drawStatus(int idx);
		void drawEvents(int idx);
		Ui::PlotWindow* dock;
		QCPItemLine* frameMarker;
		QCPItemLine* frameMarkerExtra;
		QCPItemRect* selectionMarker;
		
		// Error bars for QCustomPlot 2.1.1
		QCPErrorBars* errorBars0;
		QCPErrorBars* errorBars1;
		
		std::vector<QVector<double> > extraData;
		std::vector<double> extraPos;

		std::vector<std::vector<QCPItemRect *> > marker_status;
		std::vector<std::vector<QCPItemRect *> > events;
		std::vector<QCPItemRect *> interpolation_status;
		
		int startFrame;
		int endFrame;
		int tmpStartFrame;
		bool shiftPressed;
		bool noSelection;

		bool updating;
		
	public slots:
		void on_comboBoxCamera_currentIndexChanged(int idx);
		void on_comboBoxPlotType_currentIndexChanged(int idx);
		void on_comboBoxMarker1_currentIndexChanged(int idx);
		void on_comboBoxMarker2_currentIndexChanged(int idx);
		void on_pushButton_Reset_clicked();
		void on_pushButtonUpdate_clicked();
		void on_pushButtonSave_clicked();

		void on_comboBoxRigidBody_currentIndexChanged(int idx);
		void on_comboBoxRigidBodyTransPart_currentIndexChanged(int idx);
		void on_comboBoxRigidBodyError_currentIndexChanged(int idx);
		void on_comboBoxRigidBodyTransType_currentIndexChanged(int idx);
		void on_checkBoxTime_clicked();
		void on_checkBoxStatus_clicked();
		void on_toolButtonExtraPlot_clicked();

		void doubleSpinBoxError_valueChanged(double value);
		void doubleSpinBoxErrorRB_valueChanged(double value);

		void activeTrialChanged(int activeTrial);
		void workspaceChanged(work_state workspace);
		void activePointChanged(int idx);
		void activeRigidBodyChanged(int idx);
		void activeFrameTrialChanged(int frame);

		void goToNextAboveError();
		void goToPrevAboveError();
		void deleteAllAboveError();

		void updateExtraPlot();
		void setInterpolation();
		void setUntrackable();
		void setEventOn();
		void setEventOff();
	};
}


#endif /* PROGRESSDIALOG_H_ */

