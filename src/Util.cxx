#include "Util.h"
#include <FL/filename.H>
#include <FL/fl_draw.H>
#include <FL/Fl_SVG_Image.H>
#include <FL/Fl.H>

#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <cstdlib> // atoi


const std::string &cardDir = "svg_cards";

// config values (from fltk-schnapsen.cfg)
string_map config = {};

// statistic values (from fltk-schnapsen.sta)
string_map stats = {};


/*static*/
const std::string& Util::homeDir()
{
	static std::string home;
	if (home.empty())
	{
		char home_path[FL_PATH_MAX];
		if (std::filesystem::exists(cardDir))
		{
			home = "./";
			LOG("use local home dir: " << home << "\n");
		}
		else
		{
#ifdef WIN32
			fl_filename_expand(home_path, "$APPDATA/");
#else
			fl_filename_expand(home_path, "$HOME/.");
#endif
			home = home_path;
			home += APPLICATION;
			home += "/";
			if (std::filesystem::exists(home + cardDir))
			{
				LOG("use installed home dir: " << home << "\n");
			}
			else
			{
				throw std::runtime_error("Required resources not in place!\nAborting...");
			}
		}
	}
	return home;
}

/*static*/
std::string Util::cardset_dir()
{
	std::string dir = homeDir() + cardDir + "/";
	std::string cardset = ::config["cardset"];
	if (cardset.empty())
		cardset = "English_pattern";
	dir += cardset + "/";
	return dir;
}

/*static*/
string_map& Util::config()
{
	return ::config;
}

/*static*/
string_map& Util::stats()
{
	return ::stats;
}

const std::string& Util::config(const std::string &id_)
{
	return config()[id_];
}

void Util::config(const std::string &id_, const std::string &value_)
{
	if (value_.empty())
		config().erase(id_);
	else
		config()[id_] = value_;
}

const std::string& Util::stats(const std::string &id_)
{
	return stats()[id_];
}

void Util::stats(const std::string &id_, const std::string &value_)
{
	if (value_.empty())
		stats().erase(id_);
	else
		stats()[id_] = value_;
}

void Util::load_values_from_file(std::ifstream &if_, string_map &values_, const std::string& id_)
{
	std::string line;
	while (std::getline(if_, line))
	{
		if (line.size() && line.back() == '\r')
			line.pop_back();
		size_t pos = line.find("=");
		if (pos != std::string::npos)
		{
			std::string name = line.substr(0, pos);
			std::string value = line.substr(pos + 1);
			if (name.size() && value.size() && values_[name] == "")
			{
				DBG("[load " << id_ << "] " << name << " = " << value << "\n");
				values_[name] = value;
			}
		}
	}
}

void Util::load_config()
{
	std::ifstream cfg(homeDir() + APPLICATION + ".cfg");
	load_values_from_file(cfg, ::config, "cfg");
}

void Util::load_stats()
{
	std::ifstream stat(homeDir() + APPLICATION + ".sta");
	load_values_from_file(stat, ::stats, "stat");
}

void Util::save_values_to_file(std::ofstream &of_, const string_map &values_, const std::string &id_)
{
	for (const auto &[name, value] : values_)
	{
		if (value.empty()) continue;
		DBG("[save " << id_ << "] " << name << " = " << value << "\n");
		of_ << name << "=" << value << "\n";
	}
}

void Util::save_config()
{
	Util::config("cards", std::string()); // don't save cards string!
	std::ofstream cfg(Util::homeDir() + APPLICATION + ".cfg", std::ios::binary);
	save_values_to_file(cfg, ::config, "cfg");
}

void Util::save_stats()
{
	std::ofstream stat(homeDir() + APPLICATION + ".sta", std::ios::binary);
	save_values_to_file(stat, ::stats, "stat");
}

const std::string& Util::message(const Message m_)
{
	std::string lang = ::config["lang"];
#ifndef WIN32
	// TODO: Fix for WIN32
	if (lang.empty())
	{
		static std::string locale_name;
		if (locale_name.empty())
		{
			std::locale system_locale("");
			locale_name = system_locale.name();
			if (locale_name.size() >= 2)
				locale_name.erase(2);
			DBG("locale_name: '" << locale_name << "'\n");
		}
		lang = locale_name;
	}
#endif
	auto &m = lang.empty() || lang == "de" ? messages_de : messages_en;
	return m[m_];
}

/*static*/
void Util::draw_color_text(const std::string &text_, int x_, int y_,
                           bool shadow_/* = false*/,
                           const std::map<char, Fl_Color> &colors_/* = text_colors*/)
{
	auto draw_text = [&](const char *text_, int x_, int y_) -> void
	{
#if defined(WIN32) || defined(USE_IMAGE_TEXT)
		if (shadow_)
		{
			// not suitable when emojis are in the text string!
			static const bool text_shadow = atoi(::config["text-shadow"].c_str());
			if (text_shadow)
			{
				// draws a text "shadow" by drawing text with offset first in GRAY
				int delta = fl_height() / 30 + 1;
				Fl_Color save = fl_color();
				fl_color(fl_rgb_color(64, 64, 64));
				fl_draw(text_, x_ + delta, y_ + delta);
				fl_color(save);
			}
		}
#endif
		fl_draw(text_, x_, y_);
	};
	std::string text(text_);
	Fl_Color def_color = fl_color();
	while (text.size())
	{
		size_t pos = text.find('^');
		if (pos != std::string::npos)
		{
			std::string t = text.substr(0, pos);
			if (t.size())
			{
				draw_text(t.c_str(), x_, y_);
				x_ += fl_width(t.c_str());
			}
			if (pos + 1 >= text.size()) break;
			char c = text[pos + 1];
			auto it = colors_.find(c);
			if (it == colors_.end())
				fl_color(def_color);
			else
				fl_color(it->second);
			text = text.substr(pos + 2);
		}
		else
		{
			draw_text(text.c_str(), x_, y_);
			break;
		}
	}
}

/*static*/
void Util::draw_string(const std::string &text_, int x_, int y_, bool shadow_/*= false*/)
{
	// NOTE: fl_draw(str, x, y) does not handle control characters under WIN32
	//       so we need to split lines at '\n' ourselves...

	auto draw_line = [&](std::string line) -> void
	{
		size_t image_pos;
		int dx = 0;
		while ((image_pos = line.find("^|")) != std::string::npos)
		{
			std::string sub = line.substr(0, image_pos);
			draw_color_text(sub, x_ + dx, y_, shadow_);
			size_t pos;
			while ((pos = sub.find('^')) != std::string::npos)
				sub.erase(pos, 2);
			dx += fl_width(sub.c_str());
			line.erase(0, image_pos + 2);
			size_t end_image = line.find('|');
			if (end_image == std::string::npos) continue;
			std::string image_name = line.substr(0, end_image);
			image_name += ".svg";
			if (image_name.find('/') == std::string::npos)
				image_name = homeDir() + "rsc/" + image_name;
			Fl_SVG_Image svg(image_name.c_str());
			if (svg.w() > 0 && svg.h() > 0)
			{
				svg.normalize();
				svg.scale(fl_height(), fl_height(), 1, 1);
				svg.draw(x_ + dx, y_ - fl_height() + fl_descent());
				dx += svg.w();
			}
			line.erase(0, end_image + 1);
		}
		if (line.size())
		{
			draw_color_text(line, x_ + dx, y_, shadow_);
		}
	};

	size_t pos;
	std::string text(text_);
	while ((pos = text.find('\n')) != std::string::npos)
	{
		draw_line(text.substr(0, pos));
		text.erase(0, pos + 1);
		y_ += fl_height();
	}
	if (text.size())
	{
		draw_line(text.c_str());
	}
}

/*static*/
int Util::string_size(const std::string &text_, int &w_, int &h_)
{
	w_ = 0;
	h_ = 0;
	auto parse_line = [&](std::string line) -> int
	{
		size_t image_pos;
		int w = 0;
		std::string text;
		while ((image_pos = line.find("^|")) != std::string::npos)
		{
			text += line.substr(0, image_pos);
			line.erase(0, image_pos + 2);
			size_t end_image = line.find('|');
			if (end_image == std::string::npos) continue;
			std::string image_name = line.substr(0, end_image);
			image_name += ".svg";
			if (image_name.find('/') == std::string::npos)
				image_name = homeDir() + "rsc/" + image_name;
			Fl_SVG_Image svg(image_name.c_str());
			if (svg.w() > 0 && svg.h() > 0)
			{
				svg.normalize();
				svg.scale(fl_height(), fl_height(), 1, 1);
				w += svg.w();
			}
			line.erase(0, end_image + 1);
		}
		text += line;
		size_t pos;
		while ((pos = text.find('^')) != std::string::npos)
			text.erase(pos, 2);
		w += fl_width(text.c_str());
		return w;
	};

	size_t pos;
	std::string text(text_);
	while ((pos = text.find('\n')) != std::string::npos)
	{
		int w = parse_line(text.substr(0, pos));
		if (w > w_) w_ = w;
		text.erase(0, pos + 1);
		h_ += fl_height();
	}
	if (text.size())
	{
		int w = parse_line(text.c_str());
		if (w > w_) w_ = w;
		h_ += fl_height();
	}
	return w_;
}

/*static*/
int Util::string_size(const std::string &text_)
{
	int W, H;
	return string_size(text_, W, H);
}
