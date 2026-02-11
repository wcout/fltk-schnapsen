#pragma once

#include <string>
#include <unordered_map>
#include <map>

typedef std::unordered_map<std::string, std::string> string_map;

#include "messages.h"

#include <FL/Enumerations.H>
const Fl_Color GRAY = fl_rgb_color(128, 128, 128);
// NOTE: FL_GRAY takes value from FL_BACKGROUND, which is maybe changed by application

static const std::map<char, Fl_Color> text_colors = {
	{ 'r', FL_RED },
	{ 'g', FL_GREEN },
	{ 'b', FL_BLUE },
	{ 'w', FL_WHITE },
	{ 'B', FL_BLACK },
	{ 'y', FL_YELLOW },
	{ 'c', FL_CYAN },
	{ 'G', GRAY }
};

class Util
{
public:
	static const std::string& homeDir();
	static std::string cardset_dir();
	static string_map& config();
	static string_map& stats();

	static const std::string& config(const std::string &id_);
	static void config(const std::string &id_, const std::string &value_);
	static const std::string& stats(const std::string &id_);
	static void stats(const std::string &id_, const std::string &value_);

	static void load_values_from_file(std::ifstream &if_, string_map &values_, const std::string& id_);
	static void load_config();
	static void load_stats();

	static void save_values_to_file(std::ofstream &of_, const string_map &values_, const std::string &id_);
	static void save_config();
	static void save_stats();

	static const std::string& message(const Message m_);

	static void draw_color_text(const std::string &text_, int x_, int y_,
	                            bool shadow_ = false,
	                            const std::map<char, Fl_Color> &colors_ = text_colors);
	static void draw_string(const std::string &text, int x_, int y_, bool shadow_ = false);
	static int string_size(const std::string &text_, int &w_, int &h_);
	static int string_size(const std::string &text_);
};
