#ifndef MARKERTREEWIDGET_H_
#define MARKERTREEWIDGET_H_

#include <QTreeWidget>

namespace xma{
	static const int MARKER = QTreeWidgetItem::UserType;
	static const int RIGID_BODY = QTreeWidgetItem::UserType + 1;

	class DigitizingObject;

	class MarkerTreeWidget : public QTreeWidget
	{
		Q_OBJECT

	public:
		MarkerTreeWidget(QWidget * parent = 0);

	private:
		QAction * action_ChangeDescription;
		QAction * action_CreateRigidBody;
		QAction * action_AddToRigidBody;
		QAction * action_ResetPoints;
		QAction * action_DeletePoints;
		QAction * action_ChangeDetectionMethod;

		QTreeWidgetItem* item_contextMenu;

		private slots:
		void onCustomContextMenuRequested(const QPoint& pos);
		void showContextMenu(QTreeWidgetItem* item, const QPoint& globalPos);

		void action_ChangeDescription_triggered();
		void action_CreateRigidBody_triggered();
		void action_AddToRigidBody_triggered();
		void action_ResetPoints_triggered();
		void action_DeletePoints_triggered();
		void action_ChangeDetectionMethod_triggered();

		public slots:
		void onItemCollapsed(QTreeWidgetItem * item);
		void onItemExpanded(QTreeWidgetItem * item);

	protected:
		void dropEvent(QDropEvent * event);
	};
}


#endif /* RIGIDBODYREEWIDGETITEM_H_ */
