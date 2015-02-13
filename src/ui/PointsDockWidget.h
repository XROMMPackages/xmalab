#ifndef Points_dockwidget_H
#define Points_dockwidget_H

#include <QDockWidget>
#include <QTreeWidgetItem>
#include <QString>
#include <opencv/cv.h>

//forward declarations
namespace Ui {
	class PointsDockWidget;
}

namespace xma{
	class PointsDockWidget : public QDockWidget{

		Q_OBJECT

	public:
		
		~PointsDockWidget();
		static PointsDockWidget* getInstance();

		Ui::PointsDockWidget *dock;

		void addPointToList(int idx);
		
		bool selectPoint(int idx);
		bool selectBody(int idx);

		void reloadListFromObject();
	private:
		explicit PointsDockWidget(QWidget *parent = 0);
		static PointsDockWidget* instance;

	protected:
		public slots:

			void on_treeWidgetPoints_currentItemChanged(QTreeWidgetItem * current, QTreeWidgetItem * previous);
			void on_pushButtonSetNumberMarkers_clicked();
			void on_pushButtonSetNumberRigidBodies_clicked();
			void on_pushButtonImportExport_clicked();
			void activeTrialChanged(int activeCamera);
			void selectNextPoint();
			void selectPrevPoint();
		signals:
			void activePointChanged(int idx);
		
	};
}

#endif  // Points_dockwidget_H
