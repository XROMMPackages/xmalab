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
///\file Shortcuts.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef SHORTCUTS_H
#define SHORTCUTS_H

#include <QWidget>

namespace xma
{
	class Shortcuts : public QWidget
	{
		Q_OBJECT

	public:
		static Shortcuts* getInstance();
		virtual ~Shortcuts();

		void bindApplicationShortcuts();

		void installEventFilterToChildren(QObject* object);
		void removeEventFilterToChildren(QObject* object);
		bool checkShortcut(QObject* target, QEvent* event);

	protected:
		bool eventFilter(QObject* target, QEvent* event) override;

	private:
		Shortcuts();
		static Shortcuts* instance;

	public slots:
		void trackPointNext();
		void trackPointPrev();
		void trackPointForw();
		void trackPointBack();
		void trackSelectedNext();
		void trackSelectedPrev();
		void trackSelectedForw();
		void trackSelectedBack();
	};
}


#endif // STATE_H


