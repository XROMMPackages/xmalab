//  ----------------------------------
//  XMALab -- Copyright (c) 2015, Brown University, Providence, RI.
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
//  PROVIDED "AS IS", INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
//  FOR ANY PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR ANY 
//  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR FOR ANY DAMAGES WHATSOEVER RESULTING 
//  FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
//  OTHER TORTIOUS ACTION, OR ANY OTHER LEGAL THEORY, ARISING OUT OF OR IN CONNECTION 
//  WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
//  ----------------------------------
//  
///\file WorkspaceNavigationFrame.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef WORKSPACENAVIGATIONFRAME_H_
#define WORKSPACENAVIGATIONFRAME_H_

#include <QFrame>
#include "ui/State.h"

namespace Ui
{
	class WorkspaceNavigationFrame;
}

namespace xma
{
	class WorkspaceNavigationFrame : public QFrame
	{
		Q_OBJECT

	private:
		Ui::WorkspaceNavigationFrame* frame;
		WorkspaceNavigationFrame(QWidget* parent = 0);
		static WorkspaceNavigationFrame* instance;
		int currentComboBoxWorkspaceIndex;
		

		bool updating;
	protected:

	public:
		virtual ~WorkspaceNavigationFrame();

		static WorkspaceNavigationFrame* getInstance();

		void setUndistortionCalibration(bool hasUndistortion,bool hasCalibration);
		void addTrial(QString name);
		void removeTrial(QString name);
		void closeProject();
		void setTrialVisible(bool visible);
		void setWorkState(work_state workspace);

	public slots:

		void workspaceChanged(work_state workspace);
		void displayChanged(ui_state display);
		void activeTrialChanged(int activeTrial);
		void changeDenoiseTrialDataAfterDenoise();

			   void on_comboBoxWorkspace_currentIndexChanged(int idx);
		void on_comboBoxTrial_currentIndexChanged(int idx);
			   void on_comboBoxViewspace_currentIndexChanged(int idx);
		void on_toolButtonAddTrial_clicked();
		void on_toolButtonTrialSettings_clicked();
		void on_toolButtonCameraSettings_clicked();
	};
}


#endif /* WORKSPACENAVIGATIONFRAME_H_ */

