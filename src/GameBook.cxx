#include "GameBook.h"
#include <sstream>

std::string GameBook::str() const
{
	std::ostringstream os;
	for (auto &[player, ai] : *this)
	{
		os << player << " " << ai << ",";
	}
	std::string s(os.str());
	s.pop_back(); // remove last ','
	return s;
}
