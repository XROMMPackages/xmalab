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
///\file ConsoleDockWidget.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef CONSOLEDOCKWIDGET_H_
#define CONSOLEDOCKWIDGET_H_

#include <QDockWidget>
#include <QMutex>
#include "ui/State.h"

#define BUFFERSIZE 655360

namespace Ui
{
	class ConsoleDockWidget;
}

class QLabel;

namespace xma
{
	class ConsoleDockWidget : public QDockWidget
	{
		Q_OBJECT

	public:
		~ConsoleDockWidget() override;
		static ConsoleDockWidget* getInstance();

		void writeLog(const QString& message, unsigned int level = 0);
		void save(const QString& file);
		void load(const QString& file);
		void clear();
		void afterLoad();
		void prepareSave();
	protected:
		void closeEvent(QCloseEvent* event) override;

	private:
		Ui::ConsoleDockWidget* dock;
		static ConsoleDockWidget* instance;
		explicit ConsoleDockWidget(QWidget* parent = nullptr);
		QMutex mutex;

		char errorBuffer[BUFFERSIZE];
		char outputBuffer[BUFFERSIZE];

		QString LoadText;

		QTimer* timer;
	public slots:
		void logTimer();
	};
}


#endif /* WIZARDUNDISTORTIONFRAME_H_ */

