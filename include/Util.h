#pragma once

#include <string>
#include <unordered_map>
#include <map>
#include <fstream>

typedef std::unordered_map<std::string, std::string> string_map;

#include "messages.h"

#include <FL/Enumerations.H>
class Fl_Shared_Image;
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


class Fl_Window;

class Util
{
public:
	static const std::string& home_dir();
	static std::string rsc_dir();
	static std::string cfg_file();
	static std::string sta_file();
	static std::string cardset_dir(std::string name_ = "");
	static string_map& config();
	static string_map& stats();

	static const std::string& config(const std::string &id_);
	static int config_as_int(const std::string &id_);
	static void config(const std::string &id_, const std::string &value_);
	static const std::string& stats(const std::string &id_);
	static int stats_as_int(const std::string &id_);
	static void stats(const std::string &id_, const std::string &value_);

	static void load_values_from_file(std::ifstream &if_, string_map &values_, const std::string& id_);
	static void load_config();
	static void load_stats();

	static void save_values_to_file(std::ofstream &of_, const string_map &values_, const std::string &id_);
	static void save_config();
	static void save_stats();

	static const std::string& message(const Message m_);

	static void draw_color_text(const std::string &text_, int x_, int y_,
	                            [[maybe_unused]]bool shadow_ = false,
	                            const std::map<char, Fl_Color> &colors_ = text_colors);
	static void draw_string(const std::string &text, int x_, int y_, bool shadow_ = false);
	static int string_size(const std::string &text_, int &w_, int &h_);
	static int string_width(const std::string &text_);

	static Fl_Shared_Image *get_shared_image(const std::string &name_, int w_ = 0, int h_ = 0, bool proportional_ = false);

	static std::ostream& logstream();

	static std::string filename(const std::string &pathname_);
	static std::string dirname(const std::string &pathname_, bool absolute_ = false);

	static int run(Fl_Window &win_);
	static std::string_view trim(std::string_view str_);
};
