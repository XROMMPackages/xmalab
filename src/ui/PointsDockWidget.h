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
///\file PointsDockWidget.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef Points_dockwidget_H
#define Points_dockwidget_H

#include <QDockWidget>
#include <QTreeWidgetItem>
#include <QString>
#include <opencv2/opencv.hpp>

//forward declarations
namespace Ui
{
	class PointsDockWidget;
}

namespace xma
{
	class PointsDockWidget : public QDockWidget
	{
		Q_OBJECT

	public:
		~PointsDockWidget() override;
		static PointsDockWidget* getInstance();

		Ui::PointsDockWidget* dock;

		void addPointToList(int idx);
		std::vector<int> getSelectedPoints();
		bool selectPoint(int idx);
		bool selectBody(int idx);

		void reloadListFromObject();
		void updateColor();
		void reset();
	private:
		explicit PointsDockWidget(QWidget* parent = nullptr);
		static PointsDockWidget* instance;

	protected:
	public slots:

		void on_treeWidgetPoints_currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);
		void on_pushButtonSetNumberMarkers_clicked();
		void on_pushButtonSetNumberRigidBodies_clicked();
		void on_pushButtonImportExport_clicked();
		
		void on_checkBoxShowMarkerStatus_clicked();
		void activeTrialChanged(int activeCamera);
		void selectNextPoint();
		void selectPrevPoint();
		void selectAllPoints();
		void on_checkBoxIsFromMaster_clicked();
		void on_pushButtonApply_clicked();

		signals:
		void activePointChanged(int idx);
		void activeRigidBodyChanged(int idx);
	};
}

#endif // Points_dockwidget_H


