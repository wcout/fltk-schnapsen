#pragma once

#include <string>
#include <sstream>

class Args
{
	static void list_decks(std::ostringstream &os_);
	static std::string make_help(const string_map &la_, const string_map &sa_);
	static bool process(const std::string &arg_, const std::string &value_);
public:
	static bool parse(int argc_, char *argv_[]);
	static std::string arg0;
};
