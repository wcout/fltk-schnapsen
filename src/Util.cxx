//
// Part of "Schnapsen for 2" card game.
//
// (c) 2026 Christian Grabner
//
// Various comfort methods needed for the game.
//

#ifdef STANDALONE
constexpr char APPLICATION[] = "DrawTest";
#endif
#include "debug.h"
#include "Util.h"
#include <FL/filename.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_Window.H>
#include <FL/Fl.H>

#include "Card.h"

#include <fstream>
#include <filesystem>
#include <locale>
#include <stdexcept>
#include <cstdlib> // atoi(), getenv()
#include <cmath>

// config values (from fltk-schnapsen.cfg)
string_map config = {};

// statistic values (from fltk-schnapsen.sta)
string_map stats = {};


/*static*/
const std::string& Util::home_dir()
{
	static std::string home;
	if (home.empty())
	{
		char home_path[FL_PATH_MAX];
		if (std::filesystem::exists(Card::cardDir))
		{
			home = "./";
			LOG("use local home dir: " << home << "\n");
		}
		else
		{
#ifdef _WIN32
			fl_filename_expand(home_path, "$APPDATA/");
#else
			fl_filename_expand(home_path, "$HOME/.");
#endif
			home = home_path;
			home += APPLICATION;
			home += "/";
			if (std::filesystem::exists(home + Card::cardDir))
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
Fl_Shared_Image *Util::get_shared_image(const std::string &name_, int w_/* = 0*/, int h_/* = 0*/, bool proportional_/* = false*/)
{
	std::string name(name_);
	if (dirname(name).empty())
	{
		// use file from game 'rsc' dir, when no path is given
		name = Util::rsc_dir() + name;
	}
	Fl_Shared_Image *img = Fl_Shared_Image::get(name.c_str());
	if (img && w_ && h_)
	{
		img->scale(w_, h_, proportional_, 1);
		if (img->refcount() > 1) // don't completely release - keep cached
			img->release();
	}
	return img;
}

/*static*/
std::string Util::rsc_dir()
{
	return home_dir() + "rsc/";
}

/*static*/
std::string Util::cfg_file()
{
	return home_dir() + APPLICATION + ".cfg";
};

/*static*/
std::string Util::sta_file()
{
	return home_dir() + APPLICATION + ".sta";
};

/*static*/
std::string Util::cardset_dir(std::string name_/* = "*/)
{
	std::string dir = home_dir() + Card::cardDir + "/";
	std::string cardset = name_.empty() ? ::config["cardset"] : name_;
	if (cardset.empty() || cardset == "default")
	{
		cardset = "English_pattern";
		config("cardset", cardset);
	}
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

/*static*/
const std::string& Util::config(const std::string &id_)
{
	return config()[id_];
}

/*static*/
int Util::config_as_int(const std::string &id_)
{
	return atoi(config()[id_].c_str());
}

/*static*/
std::optional<const std::string> Util::config_value(const std::string &id_)
{
	if (config(id_).empty())
		return {};
	return config(id_);
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

int Util::stats_as_int(const std::string &id_)
{
	return atoi(stats()[id_].c_str());
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
	std::ifstream cfg(cfg_file());
	load_values_from_file(cfg, ::config, "cfg");
}

void Util::load_stats()
{
	std::ifstream stat(sta_file());
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
	std::ofstream cfg(cfg_file(), std::ios::binary);
	save_values_to_file(cfg, ::config, "cfg");
}

void Util::save_stats()
{
	std::ofstream stat(sta_file(), std::ios::binary);
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
			try
			{
				std::locale system_locale("");
				locale_name = system_locale.name();
				if (locale_name.size() >= 2)
					locale_name.erase(2);
			}
			catch (std::runtime_error &e_)
			{
				locale_name = "de";
				std::cerr << e_.what() << "\n";
				WNG("system locale not found - default to 'de'!");
			}
			DBG("locale_name: '" << locale_name << "'\n");
		}
		lang = locale_name;
		config("lang", lang);
	}
	auto &m = lang.empty() || lang == "de" ? messages_de : messages_en;
	return m[m_];
}

/*static*/
void Util::draw_color_text(const std::string &text_, int x_, int y_,
                           [[maybe_unused]]bool shadow_/* = false*/,
                           const std::map<char, Fl_Color> &colors_/* = text_colors*/)
{
	Fl_Color def_color = fl_color();
	auto draw_text = [&](const char *text_, int x_, int y_) -> void
	{
#if defined(_WIN32) || defined(USE_IMAGE_TEXT)
		if (shadow_)
		{
			// not suitable when emojis are in the text string!
			static const bool text_shadow = atoi(::config["text-shadow"].c_str());
			uchar r, g, b;
			Fl::get_color(def_color, r, g, b);
			// no shadow with too dark colors (looks bad)
			if (text_shadow && (r > 64 || g > 64 || b > 64))
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

static int is_mono_font()
{
	if (fl_width(' ') == fl_width('M'))
		return ceil(fl_width(' '));
	return 0;
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
		int mono_width = is_mono_font();
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
			Fl_Image *img = get_shared_image(image_name, fl_height() - fl_descent() / 2, fl_height() - fl_descent(), true);
			if (img != nullptr)
			{
//				DBG("Symbol " << image_name << ": " << img->w() << "x" << img->h() << " fl_height=" << fl_height() << ", fl_descent=" << fl_descent() << "\n");
				img->draw(x_ + dx, y_ - fl_height() + (fl_height() - img->h()) / 2 + fl_descent());
				int w = img->w();
				if (mono_width)
				{
					w += mono_width - 1;
					w = (w / mono_width) * mono_width;
				}
				dx += w;
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
		int mono_width = is_mono_font();
		std::string text;
		while ((image_pos = line.find("^|")) != std::string::npos)
		{
			text += line.substr(0, image_pos);
			line.erase(0, image_pos + 2);
			size_t end_image = line.find('|');
			if (end_image == std::string::npos) continue;
			std::string image_name = line.substr(0, end_image);
			image_name += ".svg";
			Fl_Image *img = get_shared_image(image_name, fl_height() - fl_descent() / 2, fl_height() - fl_descent(), true);
			if (img != nullptr)
			{
				int dx = img->w();
				if (mono_width)
				{
					dx += mono_width - 1;
					dx = (dx / mono_width) * mono_width;
				}
				w += dx;
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
int Util::string_width(const std::string &text_)
{
	int W, H;
	return string_size(text_, W, H);
}

/*static*/
std::ostream& Util::logstream()
{
	auto temp_dir = [&]() -> std::string
	{
#ifdef _WIN32
		char *temp_dir = std::getenv("TEMP");
		return temp_dir ? temp_dir : ".";
#else
		return "/tmp";
#endif
	};

	static std::ofstream ofs((temp_dir() + "/fltk-schnapsen.log").c_str(), std::ios::binary);
	return ofs;
}

/*static*/
std::string Util::filename(const std::string &pathname_)
{
	const auto f = std::filesystem::path{ pathname_ }.filename().string();
	return f;
}

/*static*/
std::string Util::dirname(const std::string &pathname_, bool absolute_/* = false*/)
{
	auto d = std::filesystem::path{ pathname_ }.parent_path();
	if (absolute_)
	{
		std::error_code ec;
		d = std::filesystem::absolute(d);
		auto dc = std::filesystem::canonical(d, ec);
		if (!ec)
		{
			d = dc;
		}
	}
	return d.string();
}

/*static*/
int Util::run(Fl_Window &win_)
{
	win_.show();
	win_.wait_for_expose();
	while (win_.shown())
	{
		Fl::wait();
	}
	delete &win_;
	return 0;
}

/*static*/
std::string_view Util::trim(std::string_view str_)
{
	const std::string_view whitespace = " \t\n\r\f\v";
	// trim begin
	const auto start = str_.find_first_not_of(whitespace);
	if (start == std::string_view::npos) return {}; // all whitespace
	// trim end
	const auto end = str_.find_last_not_of(whitespace);
	// return [begin, end]
	return str_.substr(start, end - start + 1);
}

#ifdef STANDALONE
#undef STANDALONE
#include <FL/Fl_Double_Window.H>
class TestWin : public Fl_Double_Window
{
public:
	TestWin(int w_, int h_) : Fl_Double_Window(w_, h_, "DrawTest") {}
	void draw() override
	{
		Fl_Double_Window::draw();
		fl_font(FL_HELVETICA, 30);
		fl_color(FL_BLACK);
		Util::draw_string("Das ist ein Test: ^|bum| ^|1f3c6| ...", 100, 100);
		Util::draw_string("Das ist ein Test: ^|1f4ad| 💭 ...", 100, 200);
		fl_font(FL_HELVETICA, 40);
		Util::draw_string("Das ist ein Test: ^|1f4ad| 💭 ...", 100, 300);
		Util::draw_string("👍 ^|1f44d| Congrats, your game!", 100, 400);
	}
};

int main()
{
	fl_register_images();
	std::cout << "filename: " << Util::filename("/usr/local/bin/fluid") << "\n";
	std::cout << "filename: " << Util::filename("./fluid") << "\n";
	std::cout << "filename: " << Util::filename("/usr/local/bin/") << "\n";
	std::cout << "dirname : " << Util::dirname("/usr/local/bin/fluid") << "\n";
	std::cout << "dirname : " << Util::dirname("./fluid") << "\n";
	std::cout << "dirname : " << Util::dirname("svg_cards/back/back.svg") << "\n";
	std::cout << "dirname : " << Util::dirname("./svg_cards/back/back.svg", true) << "\n";
	TestWin win(800, 600);
	win.show();
	return Fl::run();
}
#endif
