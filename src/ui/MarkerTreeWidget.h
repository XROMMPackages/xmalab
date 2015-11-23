//  ----------------------------------
//  XMA Lab -- Copyright © 2015, Brown University, Providence, RI.
//  
//  All Rights Reserved
//   
//  Use of the XMA Lab software is provided under the terms of the GNU General Public License version 3 
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
///\file MarkerTreeWidget.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef MARKERTREEWIDGET_H_
#define MARKERTREEWIDGET_H_

#include <QTreeWidget>

namespace xma
{
	static const int MARKER = QTreeWidgetItem::UserType;
	static const int RIGID_BODY = QTreeWidgetItem::UserType + 1;

	class DigitizingObject;

	class MarkerTreeWidget : public QTreeWidget
	{
		Q_OBJECT

	public:
		MarkerTreeWidget(QWidget* parent = 0);

	private:
		QAction* action_ChangeDescription;
		QAction* action_CreateRigidBody;
		QAction* action_AddToRigidBody;
		QAction* action_ResetPoints;
		QAction* action_DeletePoints;
		QAction* action_ChangeDetectionMethod;
		QAction* action_RefinePointsPolynomialFit;

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
		void action_RefinePointsPolynomialFit_triggered();

	public slots:
		void onItemCollapsed(QTreeWidgetItem* item);
		void onItemExpanded(QTreeWidgetItem* item);

	protected:
		void dropEvent(QDropEvent* event);
	};
}


#endif /* RIGIDBODYREEWIDGETITEM_H_ */

