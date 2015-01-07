#ifndef WORKSPACENAVIGATIONFRAME_H_
#define WORKSPACENAVIGATIONFRAME_H_

#include <QFrame>
#include "ui/State.h"

namespace Ui {
	class WorkspaceNavigationFrame;
}
namespace xma{
	class WorkspaceNavigationFrame : public QFrame{

		Q_OBJECT

	private:
		Ui::WorkspaceNavigationFrame *frame;
		WorkspaceNavigationFrame(QWidget *parent = 0);
		static WorkspaceNavigationFrame* instance;
		int currentComboBoxWorkspaceIndex;
		void setTrialVisible(bool visible);
	protected:

	public:
		virtual ~WorkspaceNavigationFrame();

		static WorkspaceNavigationFrame* getInstance();

		void setUndistortion(bool hasUndistortion);
		void addCamera(int idx, QString name);
		void addTrial(QString name);
		void removeCamera(int idx);

		void setWorkState(work_state workspace);

		public slots:

		void workspaceChanged(work_state workspace);
		void displayChanged(ui_state display);
		void activeCameraChanged(int activeCamera);
		void activeTrialChanged(int activeTrial);

		void on_comboBoxWorkspace_currentIndexChanged(QString value);
		void on_comboBoxTrial_currentIndexChanged(int idx);
		void on_comboBoxViewspace_currentIndexChanged(QString value);

		void on_toolButtonAddTrial_clicked();
	};
}


#endif /* WORKSPACENAVIGATIONFRAME_H_ */
