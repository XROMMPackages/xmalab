#ifndef PLOTWINDOW_H_
#define PLOTWINDOW_H_

#include <QGridLayout>
#include <QDockWidget>
#include "external/QCustomPlot/qcustomplot.h"
#include "ui/State.h"

class QComboBox;
class QFrame;

namespace xma{
	class PlotWindow : public QDockWidget
	{
		Q_OBJECT

	public:
		virtual ~PlotWindow();
		static PlotWindow* getInstance();

		void plot(int idx1, int idx2);
		void updateMarkers(bool rememberSelection);

	protected:
		bool eventFilter(QObject *target, QEvent *event);

	private:
		PlotWindow(QWidget *parent = 0);
		static PlotWindow* instance;

		QGridLayout *layout;
		QComboBox *comboBoxMarker1;
		QComboBox *comboBoxMarker2;
		QFrame *frame;

		QCustomPlot  *plotWidget;
		QCPItemLine *frameMarker;
		
		bool updating;

		public slots:
		void on_comboBoxMarker1_currentIndexChanged(int idx);
		void on_comboBoxMarker2_currentIndexChanged(int idx);

		void activeTrialChanged(int activeTrial);
		void activeFrameTrialChanged(int activeFrame);
		void workspaceChanged(work_state workspace);

	};
}


#endif /* PROGRESSDIALOG_H_ */
