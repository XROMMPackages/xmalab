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
///\file EventDockWidget.h
///\author Benjamin Knorlein
///\date 06/12/2015

#ifndef EVENTDOCKWIDGET_H_
#define EVENTDOCKWIDGET_H_

#include "ui/State.h"
#include <QDockWidget>
#include <QColor>
#include <QSignalMapper>

namespace Ui
{
	class EventDockWidget;
}

class QToolButton;
class QCheckBox;

struct event_entry
{
	QToolButton * button;
	QCheckBox * checkbox;
	QString name;
	QColor color;
};

namespace xma
{

	class EventDockWidget : public QDockWidget
	{
		Q_OBJECT

	public:
		virtual ~EventDockWidget();
		static EventDockWidget* getInstance();

		void addEvent(QString name, QColor color,bool draw);
		void deleteEvent(int idx);

	protected:
		void closeEvent(QCloseEvent* event) override;

	private:
		Ui::EventDockWidget* dock;
		static EventDockWidget* instance;
		EventDockWidget(QWidget* parent = 0);
		
		std::vector<event_entry> entries;

		QSignalMapper* mapperColor;
		QSignalMapper* mapperCheckBox;

		int getIndex(QString name);
		void clear();
		void populateActive();

	public slots:

		void on_pushButtonAdd_clicked();
		void on_pushButtonDelete_clicked();
		void on_pushButtonSet_clicked();
		void on_pushButtonUnset_clicked();
		void changeColor(QString name);
		void checkbox_clicked(QString name);

		void workspaceChanged(work_state workspace);
		void activeTrialChanged(int);
	};
}


#endif /* EVENTDOCKWIDGET_H_ */

