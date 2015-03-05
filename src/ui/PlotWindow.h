#ifndef PLOTWINDOW_H_
#define PLOTWINDOW_H_

#include <QDockWidget>
#include "ui/State.h"
#include "external/QCustomPlot/qcustomplot.h"
#include "ui/PointsDockWidget.h"

class QComboBox;
class QLabel;
class QFrame;
class QPushButton;

namespace Ui {
	class PlotWindow;
}

namespace xma{
	class PlotWindow : public QDockWidget
	{
		Q_OBJECT

	public:
		virtual ~PlotWindow();
		static PlotWindow* getInstance();

		void updateMarkers(bool rememberSelection);
		void draw();
		void resetRange();

	private:
		void plot2D(int idx1);
		void plot3D(int idx1);
		void plotDistance(int idx1, int idx2);
		void plotBackProjectionError(int idx1);
		void plotRigidBody(int idx);

	protected:
		bool eventFilter(QObject *target, QEvent *event) override;
		void closeEvent(QCloseEvent *event) override;

	private:
		PlotWindow(QWidget *parent = 0);
		static PlotWindow* instance;
		void deleteData();

		Ui::PlotWindow *dock;

		QCPItemLine *frameMarker;
		QCPItemRect *selectionMarker; 

		int startFrame;
		int endFrame;

		bool updating;

		public slots:
		void on_comboBoxCamera_currentIndexChanged(int idx);
		void on_comboBoxPlotType_currentIndexChanged(int idx);
		void on_comboBoxMarker1_currentIndexChanged(int idx);
		void on_comboBoxMarker2_currentIndexChanged(int idx);
		void on_pushButton_Reset_clicked();
		void on_pushButtonUpdate_clicked();

		void on_comboBoxRigidBody_currentIndexChanged(int idx);
		void on_comboBoxRigidBodyTransPart_currentIndexChanged(int idx);
		void on_comboBoxRigidBodyTransType_currentIndexChanged(int idx);

		void activeTrialChanged(int activeTrial);
		void workspaceChanged(work_state workspace);
		void activePointChanged(int idx);
		void activeRigidBodyChanged(int idx);
	};
}


#endif /* PROGRESSDIALOG_H_ */
