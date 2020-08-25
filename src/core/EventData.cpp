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
///\file EventData.cpp
///\author Benjamin Knorlein
///\date 06/12/2017

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "core/EventData.h"
#include "core/HelperFunctions.h"
#include <fstream>
#include <sstream>

using namespace xma;

EventData::EventData(QString name, QColor color) :name_(name), color_(color), draw_(false)
{
	
}

EventData::~EventData()
{
	

}

void EventData::init(int size)
{
	data_.assign(size, 0);
}

void EventData::loadData(QString name)
{
	std::ifstream fin;
	fin.open(name.toStdString());
	std::istringstream in;
	std::string line;
	//read first line 
	int linecount = 0;
	while (!littleHelper::safeGetline(fin, line).eof())
	{
		in.clear();
		in.str(line);
		std::vector<int> tmp;
		for (double value; in >> value; littleHelper::comma(in))
		{
			tmp.push_back(value);
		}
		if (tmp.size() > 0)
		{
			if ((int)data_.size() <= linecount) 
				data_.push_back(bool(tmp[0]));
			else
				data_[linecount] = bool(tmp[0]);
		}
		line.clear();
		linecount++;
	}
	fin.close();
}

void EventData::saveData(QString name)
{
	if (!name.isEmpty())
	{
		std::ofstream outfile(name.toStdString());
		for (unsigned int i = 0; i < data_.size(); i++)
		{
			outfile << data_[i] << std::endl;
		}
		outfile.close();
	}
}

QString EventData::getName()
{
	return name_;
}

QColor EventData::getColor()
{
	return color_;
}

void EventData::setColor(QColor c)
{
	color_ = c;
}

std::vector<bool>& EventData::getData()
{
	return data_;
}

void EventData::setDraw(bool draw)
{
	draw_ = draw;
}

bool EventData::getDraw()
{
	return draw_;
}
