#ifndef HELPERFUNCTIONS_H
#define HELPERFUNCTIONS_H

#include <fstream>
#include <QSTring>

#ifdef WIN32
	#define OS_SEP "\\"
#else
	#define OS_SEP "/"
#endif

namespace littleHelper
{
	std::istream &safeGetline(std::istream& is, std::string& t);

	std::istream &comma(std::istream& in);

	QString adjustPathToOS(QString filename);
}

#endif  //VIDEOSTREAM_H

