#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "core/HelperFunctions.h"

namespace littleHelper{

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

		for (;;) {
			int c = sb->sbumpc();
			switch (c) {
			case '\n':
				return is;
			case '\r':
				if (sb->sgetc() == '\n')
					sb->sbumpc();
				return is;
			case EOF:
				// Also handle the case when the last line has no line ending
				if (t.empty()){
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
		if ((in >> std::ws).peek() != std::char_traits<char>::to_int_type(',')) {
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
}
