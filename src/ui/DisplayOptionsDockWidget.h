//  ----------------------------------
//  XMALab -- Copyright © 2015, Brown University, Providence, RI.
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
//  PROVIDED “AS IS”, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
//  FOR ANY PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR ANY 
//  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR FOR ANY DAMAGES WHATSOEVER RESULTING 
//  FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
//  OTHER TORTIOUS ACTION, OR ANY OTHER LEGAL THEORY, ARISING OUT OF OR IN CONNECTION 
//  WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
//  ----------------------------------
//  
///\file DisplayOptionsDockWidget.h
///\author Benjamin Knorlein
///\date 05/01/2015

#ifndef DISPLAYOPTIONSDOCKWIDGET_H_
#define DISPLAYOPTIONSDOCKWIDGET_H_

#include <QDockWidget>

namespace Ui
{
	class DisplayOptionsDockWidget;
}

namespace xma
{
	class DisplayOptionsDockWidget : public QDockWidget
	{
		Q_OBJECT

	public:
		virtual ~DisplayOptionsDockWidget();
		static DisplayOptionsDockWidget* getInstance();

	protected:
		void closeEvent(QCloseEvent* event) override;

	private:
		static DisplayOptionsDockWidget* instance;
		DisplayOptionsDockWidget(QWidget* parent = 0);

		Ui::DisplayOptionsDockWidget* dock;

		void toggleEnabled(bool enabled);
	public slots:

		void on_checkBoxHide_clicked();
		void on_checkBoxMarkers_clicked();
		void on_checkBoxMarkerIds_clicked();
		void on_checkBoxEpipolar_clicked();
		void on_checkBoxRigidBodyConstellation_clicked();
		void on_checkBoxRigidBodyMeshmodels_clicked();
		void on_checkBox_DrawFiltered_clicked();

		void toggleHideAll();
	};
}


#endif /* DETAILVIEWDOCKWIDGET_H_ */

