#pragma once

#include <string>
#include <unordered_map>
#include <map>

typedef std::unordered_map<std::string, std::string> string_map;

#include "messages.h"

#include <FL/Enumerations.H>
static const std::map<char, Fl_Color> text_colors = {
	{ 'r', FL_RED },
	{ 'g', FL_GREEN },
	{ 'b', FL_BLUE },
	{ 'w', FL_WHITE },
	{ 'B', FL_BLACK },
	{ 'y', FL_YELLOW },
	{ 'G', FL_GRAY }
};

class Util
{
public:
	static const std::string& homeDir();
	static std::string cardset_dir();
	static string_map& config();
	static string_map& stats();

	static void load_values_from_file(std::ifstream &if_, string_map &values_, const std::string& id_);
	static void load_config();
	static void load_stats();
	static const std::string& message(const Message m_);

	static void draw_color_text(const std::string &text_, int x_, int y_,
                               const std::map<char, Fl_Color> &colors_ = text_colors);
};
