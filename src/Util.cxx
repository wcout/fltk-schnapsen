#include "Util.h"
#include <FL/filename.H>
#include <FL/fl_ask.H>
#include <FL/fl_draw.H>

#include <fstream>

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
		if (access(cardDir.c_str(), R_OK ) == 0)
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
			if (access((home + cardDir).c_str(), R_OK) == 0)
			{
				LOG("use installed home dir: " << home << "\n");
			}
			else
			{
				fl_message("%s", "Required resources not in place!\nAborting...");
				exit(EXIT_FAILURE);
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
	std::ofstream cfg(Util::homeDir() + APPLICATION + ".cfg");
	save_values_to_file(cfg, ::config, "cfg");
}

void Util::save_stats()
{
	std::ofstream stat(homeDir() + APPLICATION + ".sta");
	save_values_to_file(stat, ::stats, "stat");
}

const std::string& Util::message(const Message m_)
{
	std::string lang = ::config["lang"];
	if (lang.empty())
	{
		static std::string locale_name;
		if (locale_name.empty())
		{
			std::locale system_locale("");
			locale_name = system_locale.name().substr(0, 2);
			DBG("locale_name: '" << locale_name << "'\n");
		}
		lang = locale_name;
	}
	auto &m = lang.empty() || lang == "de" ? messages_de : messages_en;
	return m[m_];
}

/*static*/
void Util::draw_color_text(const std::string &text_, int x_, int y_,
                           const std::map<char, Fl_Color> &colors_/* = text_colors*/)
{
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
				fl_draw(t.c_str(), x_, y_);
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
			fl_draw(text.c_str(), x_, y_);
			break;
		}
	}
}
