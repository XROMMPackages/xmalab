#ifndef HELPERFUNCTIONS_H
#define HELPERFUNCTIONS_H

#include <fstream>

namespace littleHelper
{
	std::istream &safeGetline(std::istream& is, std::string& t);

	std::istream &comma(std::istream& in);
}

#endif  //VIDEOSTREAM_H

