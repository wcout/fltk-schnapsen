#pragma once

#include <string>
#include <sstream>

class Args
{
public:
	static bool parse(int argc_, char *argv_[]);
	static std::string arg0;
};
