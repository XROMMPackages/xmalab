#ifndef PLOTWINDOW_H_
#define PLOTWINDOW_H_

#include <QGridLayout>
#include <QDockWidget>
#include "external/QCustomPlot/qcustomplot.h"
#include "ui/State.h"
#include "ui/PointsDockWidget.h"

class QComboBox;
class QLabel;
class QFrame;
class QPushButton;

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

	protected:
		bool eventFilter(QObject *target, QEvent *event);

	private:
		PlotWindow(QWidget *parent = 0);
		static PlotWindow* instance;
		void deleteData();
		QGridLayout *layout;

		QLabel *labelPlotType;
		QComboBox *comboBoxPlotType;

		QLabel *labelCamera;
		QComboBox *comboBoxCamera;

		QLabel *labelMarker1;
		QComboBox *comboBoxMarker1;
		QLabel *labelMarker2;
		QComboBox *comboBoxMarker2;
		QPushButton *pushButton_Reset;

		QFrame *frame;

		QCustomPlot *plotWidget;
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

		void activeTrialChanged(int activeTrial);
		void workspaceChanged(work_state workspace);
		void activePointChanged(int idx);
	};
}


#endif /* PROGRESSDIALOG_H_ */
