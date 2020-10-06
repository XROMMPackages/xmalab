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
///\file HelperFunctions.cpp
///\author Benjamin Knorlein
///\date 11/20/2015

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "core/HelperFunctions.h"
#include <QFileInfo>
#include <QDir>
#include <iostream>

namespace littleHelper
{
	bool compareNames(const QString& s1, const QString& s2)
	{
		// ignore common prefix..
		int i = 0;
		while ((i < s1.length()) && (i < s2.length()) && (s1.at(i).toLower() == s2.at(i).toLower()))
			++i;
		++i;
		// something left to compare?
		if ((i < s1.length()) && (i < s2.length()))
		{
			// get number prefix from position i - doesnt matter from which string
			int k = i - 1;
			//If not number return native comparator
			if (!s1.at(k).isNumber() || !s2.at(k).isNumber())
			{
				//Two next lines
				//E.g. 1_... < 12_...
				if (s1.at(k).isNumber())
					return false;
				if (s2.at(k).isNumber())
					return true;
				return QString::compare(s1, s2, Qt::CaseSensitive) < 0;
			}
			QString n = "";
			k--;
			while ((k >= 0) && (s1.at(k).isNumber()))
			{
				n = s1.at(k) + n;
				--k;
			}
			// get relevant/signficant number string for s1
			k = i - 1;
			QString n1 = "";
			while ((k < s1.length()) && (s1.at(k).isNumber()))
			{
				n1 += s1.at(k);
				++k;
			}

			// get relevant/signficant number string for s2
			//Decrease by
			k = i - 1;
			QString n2 = "";
			while ((k < s2.length()) && (s2.at(k).isNumber()))
			{
				n2 += s2.at(k);
				++k;
			}

			// got two numbers to compare?
			if (!n1.isEmpty() && !n2.isEmpty())
				return (n + n1).toInt() < (n + n2).toInt();
			else
			{
				// not a number has to win over a number.. number could have ended earlier... same prefix..
				if (!n1.isEmpty())
					return false;
				if (!n2.isEmpty())
					return true;
				return s1.at(i) < s2.at(i);
			}
		}
		else
		{
			// shortest string wins
			return s1.length() < s2.length();
		}
	}

	std::istream& safeGetline(std::istream& is, std::string& t)
	{
		t.clear();

		// The characters in the stream are read one-by-one using a std::streambuf.
		// That is faster than reading them one-by-one using the std::istream.
		// Code that uses streambuf this way must be guarded by a sentry object.
		// The sentry object performs various tasks,
		// such as thread synchronization and updating the stream state.

		std::istream::sentry se(is, true);
		std::streambuf* sb = is.rdbuf();

		for (;;)
		{
			int c = sb->sbumpc();
			switch (c)
			{
			case '\n':
				return is;
			case '\r':
				if (sb->sgetc() == '\n')
					sb->sbumpc();
				return is;
			case EOF:
				// Also handle the case when the last line has no line ending
				if (t.empty())
				{
					is.setstate(std::ios::eofbit);
				}
				return is;
			default:
				t += (char)c;
			}
		}
	}

	std::istream& comma(std::istream& in)
	{
		if ((in >> std::ws).peek() != std::char_traits<char>::to_int_type(','))
		{
			in.setstate(std::ios_base::failbit);
		}
		return in.ignore();
	}

	QString adjustPathToOS(QString filename)
	{
		QString tmp = filename;
		tmp.replace("\\", OS_SEP);
		tmp.replace("/", OS_SEP);
		return tmp;
	}

	void copyPath(QString src, QString dst)
	{
		QDir dir(src);
		if (!dir.exists())
			return;

		foreach(QString d, dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
			QString dst_path = dst + QDir::separator() + d;
			dir.mkpath(dst_path);
			copyPath(src + QDir::separator() + d, dst_path);
		}

		foreach(QString f, dir.entryList(QDir::Files)) {
			QFile::copy(src + QDir::separator() + f, dst + QDir::separator() + f);
		}
	}
}

