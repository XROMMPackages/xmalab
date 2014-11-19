/*
 * ProgressDialog.h
 *
 *  Created on: Nov 19, 2013
 *      Author: ben
 */

#ifndef WORKSPACENAVIGATIONFRAME_H_
#define WORKSPACENAVIGATIONFRAME_H_

#include <QFrame>
#include "ui/State.h"

namespace Ui {
	class WorkspaceNavigationFrame;
}

class WorkspaceNavigationFrame : public QFrame{

	Q_OBJECT

	private:
		Ui::WorkspaceNavigationFrame *frame;
		WorkspaceNavigationFrame(QWidget *parent = 0);
		static WorkspaceNavigationFrame* instance;
		int currentComboBoxWorkspaceIndex;

	protected:
		
	public:
		~WorkspaceNavigationFrame();
		
		static WorkspaceNavigationFrame* getInstance();

		void setUndistortion(bool hasUndistortion);
		void addCamera(int idx, QString name);
		void removeCamera(int idx);

		void setWorkState(work_state workspace);

	public slots:

		void workspaceChanged(work_state workspace);
		void displayChanged(ui_state display);
		void activeCameraChanged(int activeCamera);

		void on_comboBoxWorkspace_currentIndexChanged(QString value);
		void on_comboBoxViewspace_currentIndexChanged(QString value);
};



#endif /* WORKSPACENAVIGATIONFRAME_H_ */
