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
///\file EventData.h
///\author Benjamin Knorlein
///\date 06/12/2017

#ifndef EVENTDATA_H_
#define EVENTDATA_H_

#include <vector>
#include <QString>
#include <QColor>


namespace xma
{
	class EventData
	{
	public:
		EventData(QString name, QColor color);
		virtual ~EventData();
		void init(int size);

		void loadData(QString name);
		void saveData(QString name);

		QString getName();
		QColor getColor();
		void setColor(QColor c);
		std::vector<bool> &getData();
		void setDraw(bool draw);
		bool getDraw();

	private:
		QString name_;
		QColor color_;
		std::vector<bool> data_;
		bool draw_;
	};
}

#endif //EVENTDATA_H_
