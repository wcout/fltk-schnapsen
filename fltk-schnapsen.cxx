//
//	Schnapsen for 2 card game for FLTK.
//
// Copyright 2025 Christian Grabner.
//
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_SVG_Image.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_Tiled_Image.H>
#include <FL/Fl_Input.H>
#include <FL/fl_draw.H>
#include <FL/fl_utf8.h>
#include <FL/fl_ask.H>
#include <FL/filename.H>
#include <FL/names.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <deque>
#include <map>
#include <functional>
#include <filesystem>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cassert>
#include <optional>
#include <utility>
#include <locale>
#include <math.h>
#ifdef USE_MINIAUDIO
#define MA_IMPLEMENTATION
#define MA_NO_ENCODING
#define MA_NO_FLAC
#define MA_NO_WAV
#define MA_NO_OPENSL
#define MA_NO_WEBAUDIO
#define MA_NO_CUSTOM
#define MA_NO_GENERATION
#include "miniaudio.h"
#endif

#if ( defined APPLE ) || ( defined __linux__ ) || ( defined __MING32__ )
#include <unistd.h>
#include <sys/types.h>
#endif

#ifdef WIN32
#include <windows.h>
#include <io.h>	// _access
#define random rand
#define srandom srand
#endif

// fallback Windows native
#ifndef R_OK
#define R_OK	4
#endif
#define _access access

typedef std::unordered_map<std::string, std::string> string_map;

// config values (from fltk-schnapsen.cfg)
string_map config = {};

// statistic values (from fltk-schnapsen.sta)
string_map stats = {};

// Very simple logging interface
std::string DBG_PREFIX = "\033[38;5;240m";  // light grey
std::string LOG_PREFIX = "\033[1m";         // bold
//std::string LOG_PREFIX = "\033[38;5;0m";    // black
std::string RESET_ATTR = "\033[0m";
#define OUT(x) { std::cout << x; }
#define LOG(x) { if (atoi(config["loglevel"].c_str()) > 0) std::cout << LOG_PREFIX << x << RESET_ATTR; }
#define DBG(x) { if (atoi(config["loglevel"].c_str()) > 1) std::cout << DBG_PREFIX << x << RESET_ATTR; }

constexpr char APPLICATION[] = "fltk-schnapsen";
constexpr char VERSION[] = "1.0";


enum class Player
{
	PLAYER,
	AI
};

enum class Closed
{
	NOT,
	BY_PLAYER,
	BY_AI,
	AUTO
};

enum class CardState
{
	NONE,
	MOVING,
	ON_TABLE
};

enum class CardFace
{
	TEN,
	JACK,
	QUEEN,
	KING,
	ACE,
	NrOfFaces,
	NO_FACE
};

enum class CardSuite
{
	CLUB,
	DIAMOND,
	HEART,
	SPADE,
	NrOfSuites,
	ANY_SUITE,
	NO_SUITE
};

enum class Message
{
	NO_MESSAGE,
	YOU_CHANGED,
	YOU_CLOSED,
	YOUR_GAME,
	YOUR_TRICK,
	YOUR_TURN,
	YOU_LEAD,
	YOU_NOT_ENOUGH,
	AI_CHANGED,
	AI_CLOSED,
	AI_GAME,
	AI_TRICK,
	AI_TURN,
	AI_LEADS,
	AI_NOT_ENOUGH,
	TRUMP,
	TITLE,
	GAMEBOOK,
	GB_HEADLINE,
	YOU_WIN,
	YOU_LOST,
	INVALID_SUITE,
	MUST_TRICK_WITH_SUITE,
	MUST_TRICK_WITH_TRUMP,
	NO_CLOSE,
	NO_CHANGE,
	REDEAL,
	WELCOME,
	GAMES_WON,
	MATCHES_WON,
	YOU_MARRIAGE_20,
	YOU_MARRIAGE_40,
	AI_MARRIAGE_20,
	AI_MARRIAGE_40,
	SHUFFLE
};

enum class Marriage
{
	NO_MARRIAGE,
	MARRIAGE_20,
	MARRIAGE_40
};

using enum CardSuite;
using enum CardFace;
using enum Player;
using enum CardState;
using enum Message;
using enum Closed;
using enum Marriage;

// TODO: make const?
std::map<CardFace, std::string> card_names = { {TEN, "10"}, {JACK, "jack"}, {QUEEN, "queen"}, {KING, "king"}, {ACE, "ace"} };
std::map<CardFace, std::string> card_abbr = { {TEN, "T"}, {JACK, "J"}, {QUEEN, "Q"}, {KING, "K"}, {ACE, "A"} };
std::map<CardSuite, std::string> suite_names = { {CLUB, "clubs"}, {DIAMOND, "diamonds"}, {HEART, "hearts"}, {SPADE, "spades"} };
std::map<CardFace, int> card_value = { {TEN, 10}, {JACK, 2}, {QUEEN, 3}, {KING, 4}, {ACE, 11} };
std::map<CardSuite, int> suite_weights = { {SPADE, 4}, {HEART,3}, {DIAMOND,2}, {CLUB,1} };
std::map<CardSuite, std::string> suite_symbols = { {SPADE, "♠"}, {HEART, "♥"}, {DIAMOND, "♦"}, {CLUB, "♣"} };
std::map<CardSuite, std::string> suite_symbols_image = { {SPADE, "laub.svg"}, {HEART, "herz.svg"}, {DIAMOND, "schelle.svg"}, {CLUB, "eichel.svg"} };

const std::map<char, Fl_Color> text_colors = {
	{ 'r', FL_RED },
	{ 'g', FL_GREEN },
	{ 'b', FL_BLUE },
	{ 'w', FL_WHITE },
	{ 'B', FL_BLACK },
	{ 'y', FL_YELLOW },
	{ 'G', FL_GRAY }
};

std::map<Message, std::string> messages_de = {
	{NO_MESSAGE, ""},
	{YOU_CHANGED, "Du hast den Buben getauscht"},
	{YOU_CLOSED, "Du hast zugedreht"},
	{YOUR_GAME, "Gratuliere, dein Spiel!"},
	{YOUR_TRICK, "Dein Stich"},
	{YOUR_TURN, "Du bist dran"},
	{YOU_LEAD, "Du spielst aus"},
	{YOU_NOT_ENOUGH, "Du hast nicht genug"},
	{AI_CHANGED, "AI hat den Buben getauscht"},
	{AI_CLOSED, "AI hat zugedreht"},
	{AI_GAME, "AI gewinnt das Spiel!"},
	{AI_TRICK, "AI hat gestochen"},
	{AI_TURN, "AI ist am Spiel"},
	{AI_LEADS, "AI spielt aus"},
	{AI_NOT_ENOUGH, "AI hat nicht genug"},
	{TRUMP, "Trumpf"},
	{TITLE, "Schnapsen zu zweit"},
	{GAMEBOOK, "**Spielebuch**"},
	{GB_HEADLINE, "  DU      AI"},
	{YOU_WIN, "♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦\n"
	          "♥                                  ♠\n"
	          "♣           Glückwunsch!           ♥\n"
	          "♦                                  ♣\n"
	          "♠                                  ♦\n"
	          "♥   Du hast die Partie gewonnen!   ♠\n"
	          "♣                                  ♥\n"
	          "♦                                  ♣\n"
	          "♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦"},
	{YOU_LOST, "\n\nDie AI hat dir ein\nBummerl angehängt!\n\n"},
	{INVALID_SUITE, "Du must Farbe geben"},
	{MUST_TRICK_WITH_SUITE, "Du must mit Farbe stechen"},
	{MUST_TRICK_WITH_TRUMP, "Du must mit Atout stechen"},
	{NO_CLOSE, "Zudrehen nicht mehr erlaubt"},
	{NO_CHANGE, "Tauschen nicht mehr erlaubt"},
	{REDEAL, "MISCHEN"},
	{WELCOME, "Servas Oida!\n\nHast Lust auf\na Bummerl?"},
	{GAMES_WON, "Spiele gewonnen (PL/AI): "},
	{MATCHES_WON, "Partien: "}
};

std::map<Message, std::string> messages_en = {
	{YOU_CHANGED, "You changed the jack"},
	{YOU_CLOSED, "You closed the game"},
	{YOUR_GAME, "Congrats, your game!"},
	{YOUR_TRICK, "Your trick"},
	{YOUR_TURN, "Your turn"},
	{YOU_LEAD, "Your lead"},
	{AI_CHANGED, "AI has changed the jack"},
	{AI_CLOSED, "AI has closed"},
	{AI_GAME, "AI wins the game!"},
	{AI_TRICK, "AI has made trick"},
	{AI_TURN, "AI is to play"},
	{AI_LEADS, "AI is to lead"},
	{TRUMP, "Trump"},
	{TITLE, "Schnapsen for two"},
	{GAMEBOOK, "**Game book**"},
	{GB_HEADLINE, "  YOU     AI"},
	{YOU_WIN, "♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦\n"
	          "♥                                  ♠\n"
	          "♣       Wow, your lucky day!       ♥\n"
	          "♦                                  ♣\n"
	          "♠                                  ♦\n"
	          "♥   You have won the whole lot!    ♠\n"
	          "♣                                  ♥\n"
	          "♦                                  ♣\n"
	          "♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦"},
	{YOU_LOST, "\n\nYou got the bummerl!\n\n"},
	{INVALID_SUITE, "You must give suite"},
	{MUST_TRICK_WITH_SUITE, "You must trick with suite"},
	{MUST_TRICK_WITH_TRUMP, "You must trick with trump"},
	{NO_CLOSE, "You can't close any more"},
	{NO_CHANGE, "You can't change any more"},
	{REDEAL, "REDEAL"},
	{WELCOME, "Hey dude!\n\nDo you want\na 'bummerl'?"},
	{GAMES_WON, "Games won (PL/AI): "},
	{MATCHES_WON, "Matches: "}
};

std::map<Message, std::string> sound = {
	{NO_MESSAGE, "ding"},
	{YOU_CHANGED, "change"},
	{AI_CHANGED, "change"},
	{YOU_CLOSED, "close"},
	{AI_CLOSED, "close"},
	{YOU_MARRIAGE_20, "marriage"},
	{YOU_MARRIAGE_40, "marriage"},
	{AI_MARRIAGE_20, "marriage"},
	{AI_MARRIAGE_40, "marriage"},
	{YOUR_GAME, "your_game"},
	{AI_GAME, "ai_game"},
	{YOU_WIN, "you_win"},
	{YOU_LOST, "you_lost"},
	{NO_CLOSE, "not_allowed"},
	{NO_CHANGE, "not_allowed"},
	{INVALID_SUITE, "not_allowed"},
	{MUST_TRICK_WITH_TRUMP, "not_allowed"},
	{MUST_TRICK_WITH_SUITE, "not_allowed"},
	{WELCOME, "welcome"},
	{SHUFFLE, "shuffle"}
};

// only for testing
bool debug = false;

const std::string &cardDir = "svg_cards";

// for readability of std::vector/dequeu index results
constexpr size_t NO_MOVE = (size_t)-1;

// score points count down for "bummerl"
constexpr int MATCH_SCORE = 7; // or 11?

// helpers
const std::string &homeDir()
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

void load_values_from_file(std::ifstream &if_, string_map &values_, const std::string& id_)
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

void load_config()
{
	std::ifstream cfg(homeDir() + APPLICATION + ".cfg");
	load_values_from_file(cfg, config, "cfg");
}

void load_stats()
{
	std::ifstream stat(homeDir() + APPLICATION + ".sta");
	load_values_from_file(stat, stats, "stat");
}

const std::string &message(const Message m_)
{
	std::string lang = config["lang"];
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

std::string readinFile(const std::string &name_)
{
	std::ifstream ifs(name_.c_str());
	std::stringstream buf;
	buf << ifs.rdbuf();
	return buf.str();
}

Fl_RGB_Image *rotate_90_CCW(const Fl_RGB_Image &svg_)
{
	int w = svg_.data_w();
	int h = svg_.data_h();
	int d = svg_.d();
	assert(w > 0 && h > 0 && d >= 3);
	uchar alpha = 0;
	uchar *rot_data = new uchar[w * h * d];

	const auto data = svg_.data()[0];
	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			auto get_offset = [](int w, [[maybe_unused]]int h, int x, int y, int d) -> size_t
			{
				return w * y * d + x * d;
			};
			size_t offset = get_offset(w, h, x, y, d);
			unsigned char r = data[offset];
			unsigned char g = data[offset + 1];
			unsigned char b = data[offset + 2];
			if (d > 3)
				alpha = data[offset + 3];

			offset = get_offset(h, w, h - y - 1, x, d);
			rot_data[offset]     = r;
			rot_data[offset + 1] = g;
			rot_data[offset + 2] = b;
			if (d > 3)
				rot_data[offset + 3] = alpha;
		}
	}
	Fl_RGB_Image *rotated_image = new Fl_RGB_Image(rot_data, h, w, d);
	rotated_image->alloc_array = 1;
	return rotated_image;
}

void draw_color_text(const std::string &text_, int x_, int y_, const std::map<char, Fl_Color> &colors_)
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

class Cmd_Input : public Fl_Input
{
public:
	Cmd_Input(int x_, int y_, int w_, int h_) : Fl_Input(x_, y_, w_, h_)
	{
		textsize(h() / 3 * 2);
	}
public:
	void draw() { Fl_Input::draw_box(); Fl_Input::draw(); }
	void resize(int x_, int y_, int w_, int h_)
	{
		Fl_Input::resize(x_, y_, w_, h_);
		textsize(h() / 3 * 2);
	}
};

class Button : public Fl_Button
{
public:
	Button(int x_, int y_, int w_, int h_, const char *l_ = nullptr) :
		Fl_Button(x_, y_, w_, h_, l_)
	{
		labelfont(FL_HELVETICA_BOLD);
		labelsize(h() / 2);
		visible_focus(0);
	}
	void draw() { Fl_Button::draw_box(); Fl_Button::draw(); }
	void resize(int x_, int y_, int w_, int h_)
	{
		Fl_Button::resize(x_, y_, w_, h_);
		labelsize(h() / 2);
	}
};

struct Rect
{
	Rect(int x_, int y_, int w_, int h_) : x(x_), y(y_), w(w_), h(h_) {}
	int x;
	int y;
	int w;
	int h;
	Rect (const Fl_Widget& wgt_, Fl_Boxtype box_ = FL_NO_BOX) :
		x(wgt_.x() + Fl::box_dx(box_)),
		y(wgt_.y() + Fl::box_dy(box_)),
		w(wgt_.w() - Fl::box_dw(box_)),
		h(wgt_.h() - Fl::box_dh(box_))
	{}
	bool includes(int x_, int y_) const { return x_ >= x && y_ >= y && x_ < x + w && y_ < y + h; }
	std::pair<int, int> center() const { return std::make_pair((x + w) / 2, (y + h) / 2); }
};

class CardImage
{
public:
	CardImage& image(const std::string &id_, const std::string &pathname_)
	{
		assert(_images.find(id_) == _images.end());
		std::string pathname(pathname_);
		Fl_RGB_Image *svg = new Fl_SVG_Image(pathname.c_str());
		if ((!svg || svg->w() <= 0 || svg->h() <= 0))
		{
			delete svg;
			pathname.erase(pathname.size() - 3);
			pathname.append("png");
			svg = new Fl_PNG_Image(pathname.c_str());
		}
		if ((!svg || svg->w() <= 0 || svg->h() <= 0))
		{
			fl_alert("Card image '%s' not found!", pathname_.c_str());
			exit(EXIT_FAILURE);
		}
		assert(svg && svg->w() > 0 && svg->h() > 0);
		_images[id_] = svg;
		_last_id = id_;
		return *this;
	}
	Fl_RGB_Image *image(const std::string &id_) const
	{
		auto image = _images.find(id_);
		if (image == _images.end())
			return nullptr;
		return image->second;
	}
	Fl_RGB_Image *image() const
	{
		assert(_last_id.size());
		return image(_last_id);
	}
	Fl_RGB_Image *skewed_image(const std::string &id_)
	{
		Fl_RGB_Image *svg = image(id_);
		assert(svg && svg->w() > 0 && svg->h() > 0);
		Fl_RGB_Image *skewed_image = _images[id_ + "_skewed"];
		if (svg->as_svg_image())
			svg->as_svg_image()->resize(svg->w(), svg->h());
		if (!skewed_image
			|| skewed_image->w() != svg->w() || skewed_image->h() != svg->h() / 3)
		{
			delete skewed_image;
			skewed_image = static_cast<Fl_RGB_Image *>(svg->copy());
			skewed_image->scale(svg->w(), svg->h() / 3, 0, 1);
			_images[id_ + "_skewed"] = skewed_image;
		}
		assert(skewed_image);
		return skewed_image;
	}
	Fl_RGB_Image *quer_image(const std::string &id_)
	{
		Fl_RGB_Image *svg = image(id_);
		assert(svg && svg->w() > 0 && svg->h() > 0);
		Fl_RGB_Image *quer_image = _images[id_ + "_quer"];
		if (svg->as_svg_image())
			svg->as_svg_image()->resize(svg->w(), svg->h());
		if (!quer_image
			|| quer_image->w() != svg->h() || quer_image->h() != svg->w())
		{
#if 0
// only for debugging
			if (!quer_image)
			{
				printf("no quer image '%s'\n", id_.c_str());
			}
			else
			{
				printf("quer image '%s' has %dx%d, should have: %dx%d\n",
					id_.c_str(), quer_image->w(), quer_image->h(), svg->w(), svg->h());
			}
#endif
			delete quer_image;
			DBG("rotate image '" << id_ << "'\n");
			_images[id_ + "_quer"] = rotate_90_CCW(*svg);
		}
		quer_image = _images[id_ + "_quer"];
		quer_image->scale(svg->h(), svg->w(), 0, 1);
		assert(quer_image);
		return quer_image;
	}
	Fl_RGB_Image *quer_image()
	{
		assert(_last_id.size());
		return quer_image(_last_id);
	}
	Fl_RGB_Image *skewed_image()
	{
		assert(_last_id.size());
		return skewed_image(_last_id);
	}
private:
	std::string _last_id;
	static std::unordered_map<std::string, Fl_RGB_Image *> _images;
};
/*static*/std::unordered_map<std::string, Fl_RGB_Image *> CardImage::_images;

std::string cardset_dir()
{
	std::string dir = homeDir() + cardDir + "/";
	std::string cardset = config["cardset"];
	if (cardset.empty())
		cardset = "English_pattern";
	dir += cardset + "/";
	return dir;
}

class Card
{
public:
	Card() : _f(NO_FACE), _s(NO_SUITE), _rect(0,0,0,0) {}
	explicit Card(CardFace f_, CardSuite s_) :
		_f(f_),
		_s(s_),
		_rect(0,0,0,0)
	{
		if (!_images.image(name()))
		{
			std::string root = cardset_dir();
			_pathname = root + filename();
			DBG("load '" << _pathname << "'\n");
			_images.image(name(), _pathname);
		}
	}
	Fl_RGB_Image *image() const { return _images.image(name()); }
	Fl_RGB_Image *quer_image() { return _images.quer_image(name()); }
	void rect(const Rect &rect_) { _rect = rect_; }
	CardSuite suite() const { return _s; }
	CardFace face() const { return _f; }
	const Rect &rect() const { return _rect; }
	int value() const { return card_value[face()]; }
	std::string face_name() const { return card_names[face()]; }
	std::string face_abbr() const { return card_abbr[face()]; }
	std::string suite_name() const { return suite_names[suite()]; }
	std::string name() const { return face_name() + " of " + suite_name(); }
	std::string filename() const { return face_name() + "_of_" + suite_name() + ".svg"; }
	int suite_weight() const { return suite_weights[suite()]; }
	std::string suite_symbol() const { return suite_symbols[suite()]; }
	static std::string suite_symbol(CardSuite suite_) { return suite_symbols[suite_]; }
	bool is_black_suite() const { return _s == SPADE || _s == CLUB; }
	bool is_red_suite() const { return !is_black_suite(); }
	bool includes(int x_, int y_) const { return rect().includes(x_, y_); }
	virtual std::ostream &printOn(std::ostream &os_) const
	{
		std::string abbr = face_abbr();
		os_ << abbr << suite_symbol();
		return os_;
	}
private:
	CardFace _f;
	CardSuite _s;
	std::string _pathname;
	CardImage _images;
	Rect _rect;
};
inline std::ostream &operator << (std::ostream &os_, const Card &c_)
{
	return c_.printOn(os_);
}


typedef std::deque<Card> Cards_;
typedef std::deque<CardSuite> Suites;

class Cards : public Cards_
{
public:
	Cards() {}
	Cards(const Cards_ &cards_)
	{
		*this = cards_;
	}
	Cards operator += (const Cards &c_)
	{
		for (auto c : c_) push_back(c);
		return *this;
	}
	Cards operator + (const Cards &c_) const
	{
		Cards res(*this);
		for (auto c : c_) res.push_back(c);
		return res;
	}
	Cards operator -= (const Cards &c_)
	{
		for (auto &c : c_)
		{
			auto i = find_pos(c);
			if (i)
			{
				erase(begin() + i.value());
			}
		}
		return *this;
	}
	Cards operator - (const Cards &c_) const
	{
		Cards res(*this);
		for (auto &c : c_)
		{
			auto i = res.find_pos(c);
			if (i)
			{
				res.erase(res.begin() + i.value());
			}
		}
		return res;
	}
	Cards operator += (const Card &c_)
	{
		push_back(c_);
		return *this;
	}
	Cards operator + (const Card &c_) const
	{
		Cards res(*this);
		res.push_back(c_);
		return res;
	}
	Cards operator -= (const Card &c_)
	{
		auto i = find_pos(c_);
		if (i)
		{
			erase(begin() + i.value());
		}
		return *this;
	}
	Cards operator - (const Card &c_) const
	{
		Cards res(*this);
		auto i = res.find_pos(c_);
		if (i)
		{
			res.erase(res.begin() + i.value());
		}
		return res;
	}
	void check()
	{
		assert(find(Card(ACE,HEART)));
		assert(find(Card(TEN,HEART)));
		assert(find(Card(KING,HEART)));
		assert(find(Card(QUEEN,HEART)));
		assert(find(Card(JACK,HEART)));

		assert(find(Card(ACE,SPADE)));
		assert(find(Card(TEN,SPADE)));
		assert(find(Card(KING,SPADE)));
		assert(find(Card(QUEEN,SPADE)));
		assert(find(Card(JACK,SPADE)));

		assert(find(Card(ACE,CLUB)));
		assert(find(Card(TEN,CLUB)));
		assert(find(Card(KING,CLUB)));
		assert(find(Card(QUEEN,CLUB)));
		assert(find(Card(JACK,CLUB)));

		assert(find(Card(ACE,DIAMOND)));
		assert(find(Card(TEN,DIAMOND)));
		assert(find(Card(KING,DIAMOND)));
		assert(find(Card(QUEEN,DIAMOND)));
		assert(find(Card(JACK,DIAMOND)));
	}
	std::optional<size_t> find_face(CardFace f_) const
	{
		for (size_t i=0; i < size(); i++)
		{
			if (at(i).face() == f_)
				return i;
		}
		return {};
	}
	std::optional<size_t> find_pos(const Card &c_) const
	{
		for (size_t i = 0; i < size(); i++)
		{
			if (at(i).face() == c_.face() && at(i).suite() == c_.suite())
				return i;
		}
		return {};
	}
	std::optional<Card> find(const Card &c_) const
	{
		auto card = find_pos(c_);
		if (card)
			return at(card.value());
		return {};
	}
	Cards parse_cards(const std::string &s_)
	{
		// parse card-string in format: '|T♣|Q♦|T♦|Q♣|J♦|Q♠|T♠|Q♥|J♠|A♦|K♥|J♣|K♠|J♥|T♥|A♥|A♣|A♠|K♣|K♦|'
		Cards cards;
		std::string s(s_);
		while (s.size())
		{
			assert(s[0] == '|');
			s.erase(0, 1);
			size_t next_card = s.find("|");
			if (next_card == std::string::npos) break;
			std::string c = s.substr(0, next_card);
			assert(c.size() > 1);
			std::string face_str = c.substr(0, 1);
			std::string suite_str = c.substr(1);
			s.erase(0, next_card);
			for (auto sym : suite_symbols)
			{
				if (sym.second == suite_str)
				{
					CardSuite suite = sym.first;
					for (auto f : card_abbr)
					{
						if (f.second == face_str)
						{
							CardFace face = f.first;
							cards.push_back(Card(face, suite));
							break;
						}
					}
				}
			}
		}
		return cards;
	}
	void shuffle()
	{
		LOG("shuffle\n");
		assert(size());
		std::string cards = config["cards"];
		if (cards.size() == 101)
		{
			*this = parse_cards(cards);
			LOG("Using predefind card set!\n");
		}
		else
		{
			for (int i = 0; i < rand() % 100 + 100; i++)
			{
				size_t idx1 = random() % size();
				size_t idx2 = random() % size();
				Card c = at(idx1);
				erase(begin() + idx1);
				insert(begin() + idx2, c);
			}
		}
		check();
	}
	void sort()
	{
		auto sortRuleCards = [] (Card const &c1_, Card const &c2_) -> bool
		{
			if (c1_.suite() == c2_.suite()) return c1_.value() > c2_.value();
			return c1_.suite_weight() > c2_.suite_weight();
		};
		std::sort(begin(), end(), sortRuleCards);
	}
	void sort(const CardSuite trump_)
	{
		auto sortRuleCards = [&] (Card const &c1_, Card const &c2_) -> bool
		{
			if (c1_.suite() == c2_.suite()) return c1_.value() > c2_.value();
			int sw1 = c1_.suite_weight();
			int sw2 = c2_.suite_weight();
			if (c1_.suite() == trump_) sw1 *= 100;
			if (c2_.suite() == trump_) sw2 *= 100;
			return sw1 > sw2;
		};
		std::sort(begin(), end(), sortRuleCards);
	}
	void sort_by_value()
	{
		auto sortRuleCards = [] (Card const &c1_, Card const &c2_) -> bool
		{
			return c1_.value() > c2_.value();
		};
		std::sort(begin(), end(), sortRuleCards);
	}

	int value() const
	{
		int value = 0;
		for (auto &c : *this)
			value += c.value();
		return value;
	}

	static Cards fullcards()
	{
		Cards cards;
		for (int s = 0; s < (int)NrOfSuites; s++)
		{
			for (int f = 0; f < (int)NrOfFaces; f++)
			{
				cards.push_back(Card((CardFace)f, CardSuite(s)));
			}
		}
		assert(cards.size() == 20);
		return cards;
	}

	virtual std::ostream &printOn(std::ostream &os_) const
	{
		if (size())
			os_ << "|";
		for (auto c : *this)
		{
			os_ << c << "|";
		}
		return os_;
	}
};
inline std::ostream &operator<<(std::ostream &os_, const Cards &cards_)
{
	return cards_.printOn(os_);
}

class Deck;

class Welcome : public Fl_Double_Window
{
public:
	Welcome(const Deck &deck_);
	~Welcome();
	static void redraw_timer(void *d_);
	int handle(int e_);
	void draw_stats();
	void draw();
private:
	const Deck &_deck;
};

#ifdef USE_MINIAUDIO
class Audio
{
public:
	Audio()
	{
		// Initialize the engine
		if (ma_engine_init(NULL, &_engine) != MA_SUCCESS)
		{
			OUT("Failed to initialize audio engine.\n");
		}
	}
	void play(const std::string &filename_)
	{
		// Play the sound asynchronously.
		// This function returns immediately.
		ma_engine_play_sound(&_engine, filename_.c_str(), NULL);
	}
	~Audio()
	{
		ma_engine_stop(&_engine);
	}
private:
	ma_engine _engine;
};
#endif

struct GameData
{
	GameData() : score(0), pending(false),
	             message(NO_MESSAGE), deck_info(false), games_won(0),
	             matches_won(0), display_score(false), move_state(NONE) {}
	Cards   cards;    // hand
	Cards   deck;     // stack
	Card    card;     // card that is moved to play
	int     score;    // current score
	int     pending;  // from 20/40 will score *after* won trick
	Suites  s20_40;
	Message message;
	bool    deck_info;
	int     games_won;
	int     matches_won;
	bool    display_score;
	CardState move_state;
	Card    last_drawn;
};

typedef void (Deck::*DeckMemberFn)();

class Deck : public Fl_Double_Window
{
public:
	Deck() : Fl_Double_Window(800, 600),
		_trump(HEART),
		_marriage(NO_MARRIAGE),
		_closed(NOT),
		_move(PLAYER),
		_error_message(NO_MESSAGE),
		_disabled(false),
		_card_template(QUEEN, SPADE),
		_CW(0),
		_CH(0),
		_cmd_input(nullptr),
		_redeal_button(nullptr),
		_welcome(nullptr),
		_grayout(false),
		_animate_xy(std::make_pair(-1, -1)),
		_animate(nullptr)
	{
		_player.games_won = atoi(stats["player_games_won"].c_str());
		_ai.games_won = atoi(stats["ai_games_won"].c_str());
		_player.matches_won = atoi(stats["player_matches_won"].c_str());
		_ai.matches_won = atoi(stats["ai_matches_won"].c_str());
		copy_label(message(TITLE).c_str());
		fl_register_images();
		std::string root = homeDir() + cardDir;
		std::string cardback = config["cardback"];
		if (cardback.empty())
			cardback = "Card_back_red.svg";
		_back.image("card_back", root + "/back/" + cardback);
		_shadow.image("card_shadow", homeDir() + cardDir + "/Card_shadow.svg");
		_outline.image("card_outline", homeDir() + cardDir + "/Card_outline.svg");
		_cards = Cards::fullcards();
		_cards.check();
		_card_template = _cards[0];
		unit_tests();
		default_cursor(FL_CURSOR_HAND);
		Fl_RGB_Image *icon = Card(QUEEN, HEART).image();
		icon->normalize();
		default_icon(icon);
		_redeal_button = new Button(w() - 100, h() - 40, 100, 40, message(REDEAL).c_str());
		_redeal_button->color(FL_YELLOW);
		_redeal_button->selection_color(fl_darker(FL_YELLOW));
		_redeal_button->visible_focus(0);
		_redeal_button->hide();
		resizable(this);
		size_range(400, 300, 0, 0, 0, 0, 1);
		int width = atoi(config["width"].c_str());
		int height = atoi(config["height"].c_str());
		if (width > 400 && height > 300)
			size(width, height);
		end();
		_redeal_button->callback([](Fl_Widget *wgt_, void *)
		{
			static_cast<Deck *>(wgt_->window())->init();
		});
		if (config["fullscreen"] == "1")
		{
			toggle_fullscreen();
		}
	}

	~Deck()
	{
		delete _redeal_button;
		delete _cmd_input;
	}

	void ai_message(Message m_, bool bell_ = false)
	{
		if (bell_) bell(m_);
		_ai.message = m_;
		std::string m(message(m_));
		DBG("ai_message(" << m << ")\n")
	}

	void player_message(Message m_, bool bell_ = false)
	{
		if (bell_) bell(m_);
		_player.message = m_;
		std::string m(message(m_));
		DBG("player_message(" << m << ")\n")
	}

	void error_message(Message m_, bool bell_ = false)
	{
		if (bell_) bell(m_);
		_error_message = m_;
		std::string m(message(m_));
		DBG("error_message(" << m << ")\n")
	}

	size_t find(const Card &c_, const Cards &cards_) const
	{
		auto i = cards_.find_pos(c_);
		if (i) return i.value();
		return NO_MOVE;
	}

	Suites have_40(const Cards &cards_)
	{
		Suites result;
		auto trump_queen = cards_.find(Card(QUEEN, _trump));
		auto trump_king = cards_.find(Card(KING, _trump));
		if (trump_queen && trump_king)
		{
			result.push_back(_trump);
		}
		// trump or empty()
		return result;
	}

	Suites have_20(const Cards &cards_)
	{
		Suites result;
		auto find_20 = [&] (CardSuite suite) -> bool
		{
			if (cards_.find(Card(QUEEN, suite)) && cards_.find(Card(KING, suite)))
			{
				result.push_back(suite);
				return true;
			}
			return false;
		};
		if (_trump != SPADE) find_20(SPADE);
		if (_trump != HEART) find_20(HEART);
		if (_trump != DIAMOND) find_20(DIAMOND);
		if (_trump != CLUB) find_20(CLUB);
		// list of suites or empty
		return result;
	}

	bool test_change()
	{
		if (_cards.size() && _cards.back().face() != JACK &&
		    _player.card.face() == JACK && _player.card.suite() == _cards.back().suite())
		{
			if (_cards.size() < 4)
			{
				error_message(NO_CHANGE, true);
				return false;
			}
			// make change
			LOG("Player changes jack for " << _cards.back() << "\n");
			Card c = _cards.back();
			_cards.pop_back();
			_cards.push_back(_player.card);
			_player.card = c;
			player_message(YOU_CHANGED, true);
			return true;
		}
		return false;
	}

	bool test_change(Cards &cards_)
	{
		if (_cards.size() < 4 || _closed != NOT) return false;
		auto i = cards_.find_pos(Card(JACK, _cards.back().suite()));
		if (!i) return false;

		// make change
		LOG("AI changes jack for " << _cards.back() << "\n");
		Card c = _cards.back();
		_cards.pop_back();
		Card jack = cards_[i.value()];
		cards_.erase(cards_.begin() + i.value());
		_cards.push_back(jack);
		cards_.push_back(c);
		cards_.sort();
		ai_message(AI_CHANGED, true);
		redraw();
		wait(1.5);
		return true;
	}

	bool ai_test_close()
	{
		// test if ai should close
		if (_closed == NOT && _player.move_state == NONE && _ai.move_state == MOVING &&
		    _cards.size() >= 4)
		{
			int maybe_score = highest_cards_in_hand(_ai.cards).value() + _ai.score + _ai.pending;
			DBG("maybe_score: " << maybe_score << "\n")
			if (maybe_score >= 60)
			{
				LOG("closed by AI!\n");
				_closed = BY_AI;
				ai_message(AI_CLOSED, true);
				redraw();
				wait(1.5);
				return true;
			}
		}
		return false;
	}

	bool idle() const
	{
		return _player.move_state == NONE && _ai.move_state == NONE;
	}

	bool test_close(int x_, int y_)
	{
		// test if player wants close and is allowed to
		if (_closed == NOT && idle() &&
		    _cards.front().rect().includes(x_, y_))
		{
		   if (_cards.size() < 4)
			{
				error_message(NO_CLOSE, true);
			}
			else
			{
				LOG("closed by player!\n");
				_closed = BY_PLAYER;
				player_message(YOU_CLOSED, true);
				return true;
			}
		}
		return false;
	}

	bool test_20_40(int x_, int y_)
	{
		_marriage = NO_MARRIAGE;
		if (_player.card.face() == QUEEN || _player.card.face() == KING)
		{
			for (auto &c : _player.cards)
			{
				if (c.suite() == _player.card.suite() &&
				   (c.face() == QUEEN || c.face() == KING) &&
					(c.includes(x_, y_)))
				{
					if (c.suite() == _trump)
					{
						_marriage = MARRIAGE_40;
					}
					else
					{
						_marriage = MARRIAGE_20;
					}
					return true;
				}
			}
		}
		return false;
	}

	bool has_suite(const Cards &cards_, CardSuite suite_) const
	{
		for (auto &c : cards_)
			if (c.suite() == suite_) return true;
		return false;
	}

	Cards suites_in_hand(CardSuite suite_, const Cards &cards_) const
	{
		Cards res;
		for (auto &c : cards_)
			if (c.suite() == suite_) res.push_front(c);
		return res;
	}

	Cards count_played_suite(CardSuite suite_) const
	{
		// counts all cards of suite 'suite_', that are
		// "knowable" by AI
		Cards res;
//		for (size_t i = 0; i < _ai.cards.size(); i++)
//			if (_ai.cards[i].suite() == suite_) res.push_front(_ai.cards[i]);
		for (auto c : _ai.deck)
			if (c.suite() == suite_) res.push_front(c);
		// allowed to ask player deck, because AI could have memorized
		// the tricks
		for (auto c : _player.deck)
			if (c.suite() == suite_) res.push_front(c);
/*
		if (include_pack_ == true)
		{
			// include visible trump of pack (if still there)
			if (_cards.size() && _cards.back().suite() == suite_)
				res.push_front(_cards.back());
		}
*/
		return res;
	}

	int cards_in_play(CardSuite suite_)
	{
		// TODO: review/test!!!
		return 5 - count_played_suite(suite_).size();
	}

	int max_cards_player(CardSuite suite_)
	{
		// TODO: review/test!!!
		return cards_in_play(suite_) - suites_in_hand(suite_, _ai.cards).size();
	}

	int max_trumps_player()
	{
		// TODO: review/test!!!
		return cards_in_play(_trump) - suites_in_hand(_trump, _ai.cards).size();
	}

	bool valid_move(const Card &card_)
	{
		// check closed game and AI has card on table
		if (_closed != NOT)
		{
			if (_ai.move_state == ON_TABLE)
			{
				// construct player cards from cards+played card
				Cards temp = _player.cards;
				temp.push_back(card_);
				if (has_suite(temp, _ai.card.suite()))
				{
					// if player cards has suite it must use
					if (card_.suite() != _ai.card.suite())
					{
						error_message(INVALID_SUITE, true);
						return false;
					}
					// if player can trick with suite, he must
					if (can_trick_with_suite(_ai.card, temp) && !card_tricks(card_, _ai.card))
					{
						error_message(MUST_TRICK_WITH_SUITE, true);
						return false;
					}
					// otherwise a lower card of suite is accepted
					return true;
				}
				// if player can trick (with trump now), he must
				if (can_trick(_ai.card, temp) && !card_tricks(card_, _ai.card))
				{
					error_message(MUST_TRICK_WITH_TRUMP, true);
					return false;
				}
			}
		}
		// player can't trick and has no suite color
		return true;
	}

	bool can_trick(const Card &c_, const Cards &cards_) const
	{
		for (auto &c : cards_)
		{
			if (card_tricks(c, c_)) return true;
		}
		return false;
	}

	bool can_trick_with_suite(const Card &c_, const Cards &cards_) const
	{
		for (auto &c : cards_)
		{
			if (c.suite() != c_.suite()) continue;
			if (card_tricks(c, c_)) return true;
		}
		return false;
	}

	bool card_tricks(const Card &c1_, const Card &c2_) const
	{
		// does card c1 trick card c2?
		bool result( false );
		if (c1_.suite() == c2_.suite())
		{
			if (c1_.value() > c2_.value()) result = true;
		}
		else if (c1_.suite() == _trump)
		{
			result = true;
		}
		return result;
	}

	bool must_give_color(const Card &c_, const Cards &cards_) const
	{
		return has_suite(cards_, c_.suite());
	}

	size_t best_trick_card(const Card &c_, Cards &tricks_) const
	{
		size_t move = NO_MOVE;
		assert(tricks_.size());

		// try to find a game winning trick
		for (auto &c : tricks_)
		{
			if (c.value() + c_.value() + _ai.score + _ai.pending >= 66)
			{
				move = find(c, tricks_);
				break;
			}
		}
		if (move == NO_MOVE)
		{
			// try to find a trick that pushes score beyond 32
			for (auto c : tricks_)
			{
				if (c.value() + c_.value() + _ai.score + _ai.pending >= 33)
				{
					move = find(c, tricks_);
					break;
				}
			}
		}
		if (move == NO_MOVE)
		{
			move = lowest_card_that_tricks(c_, tricks_);
		}
		assert(move != NO_MOVE);
		DBG("best_trick_card: " << tricks_ << " - " << c_ << " => " << tricks_[move] << "\n")
		return move;
	}

	size_t must_give_color_or_trick(const Card &c_, Cards &cards_) const
	{
		Cards same_suite;
		Cards trump_suite;
		for (auto &c : cards_)
		{
			if (c.suite() == c_.suite())
				same_suite.push_back(c);
		}
		if (same_suite.empty())
		{
			// we don't have this suite, maybe trick with trump?
			if (c_.suite() != _trump)
			{
				for (auto c : cards_)
				{
					if (c.suite() == _trump)
						trump_suite.push_back(c);
				}
				if (trump_suite.empty())
				{
					// we don't even have a trump
					return lowest_card(cards_);
				}
				Card best_trick = trump_suite[best_trick_card(c_, trump_suite)];
				return find(best_trick, cards_);
			}
			return lowest_card(cards_);
		}
		// we have this suite
		Cards tricks;
		for (auto &c : same_suite)
		{
			if (card_tricks(c, c_))
				tricks.push_back(c);
		}
		if (tricks.empty())
		{
			// we can't trick, so return lowest card of suite
			size_t i = lowest_card(same_suite);
			return find(same_suite[i], cards_);
		}
		Card best_trick = tricks[best_trick_card(c_, tricks)];
		return find(best_trick, cards_);
	}

	size_t lowest_card(Cards &cards_, bool no_trump_ = true) const
	{
		// return the lowest card, but no trump if possible
		int lowest_value = 20;
		int lowest_value_trump = 20;
		size_t lowest = NO_MOVE;
		size_t lowest_trump = NO_MOVE;
		for (size_t i=0; i < cards_.size(); i++)
		{
			if (cards_[i].suite() == _trump)
			{
				if (cards_[i].value() < lowest_value_trump)
				{
					lowest_value_trump = cards_[i].value();
					lowest_trump = i;
				}
			}
			else if (cards_[i].value() < lowest_value)
			{
				lowest_value = cards_[i].value();
				lowest = i;
			}
		}
		if (lowest == NO_MOVE)
			lowest = lowest_trump;
		if (no_trump_ == true && lowest != NO_MOVE)
			return lowest;
		return lowest_trump;
	}

	size_t lowest_card_that_tricks(const Card &c_, const Cards &cards_) const
	{
		int lowest_value = 999;
		size_t lowest = NO_MOVE;
		for (size_t i = 0; i < cards_.size(); i++)
		{
			if (card_tricks(cards_[i], c_))
			{
				int value = cards_[i].value();
				if (cards_[i].suite() == _trump)
					value += 100;
				if (value < lowest_value)
				{
					lowest_value = value;
					lowest = i;
				}
			}
		}
		return lowest;
	}

	Cards cards_to_claim(CardSuite suite_ = ANY_SUITE) const
	{
		Cards res;
		// in use at end game playout ("allowed" to use _player.cards)
		Cards player_cards = Cards::fullcards() - _player.deck - _ai.deck - _ai.cards;
		if (_cards.size())
			player_cards -= _cards.back(); // open trump is certainly not in player cards
		player_cards.sort();
		DBG("assumed player cards: " << player_cards << "\n")
		for (const auto &c : _ai.cards)
		{
			if (suite_ != ANY_SUITE && c.suite() != suite_) continue;
			for (size_t i = 0; i < player_cards.size(); i++)
			{
				const Card &pc = player_cards[i];
				if (pc.suite() != c.suite()) continue;
				if (card_tricks(pc, c)) break;
				res.push_back(c);
				player_cards.erase(player_cards.begin() + i);
				break;
			}
		}
		// TODO: sort by value or trump?
		res.sort_by_value();
		DBG("cards_to_claim: " << res << "\n")
		return res;
	}

	Cards trumps_to_claim() const
	{
		return cards_to_claim(_trump);
	}

	size_t highest_card_that_tricks(const Card &c_, const Cards &cards_) const
	{
		int highest_value = 0;
		size_t highest = NO_MOVE;
		for (size_t i = 0; i < cards_.size(); i++)
		{
			if (card_tricks(cards_[i], c_))
			{
				int value = cards_[i].value();
				if (cards_[i].suite() == _trump)
					value -= 1;
				if (value > highest_value)
				{
					highest_value = value;
					highest = i;
				}
			}
		}
		return highest;
	}

	Cards highest_cards_of_suite_in_hand(const Cards &cards_, CardSuite suite_)
	{
		Cards res;
		Cards played_suites(suites_in_hand(suite_, _ai.deck + _player.deck));
		Cards suites(suites_in_hand(suite_, cards_));
		for (auto c : suites)
		{
			Cards temp(played_suites);
			temp += suites; // include self
			temp.erase(temp.begin() + find(c, temp)); // but not current card!
			if (c.face() == ACE) res.push_back(c);
			if (c.face() == TEN && temp.find_face(ACE)) res.push_back(c); // ACE already played (or in own hand)
			if (c.face() == KING && temp.find_face(TEN) && temp.find_face(ACE)) res.push_back(c); // TEN & ACE already played (or in own hand)
			if (c.face() == QUEEN && temp.find_face(KING) && temp.find_face(TEN) && temp.find_face(ACE)) res.push_back(c);
			if (c.face() == JACK && temp.find_face(QUEEN) && temp.find_face(KING) && temp.find_face(TEN) && temp.find_face(ACE)) res.push_back(c);
		}
		res.sort();
		DBG("highest_cards_of_suite_in_hand " << suite_symbols[suite_] << ": "<< res << "\n")
		return res;
	}

	Cards highest_cards_in_hand(const Cards &cards_)
	{
		Cards res;
		res += highest_cards_of_suite_in_hand(cards_, HEART);
		res += highest_cards_of_suite_in_hand(cards_, SPADE);
		res += highest_cards_of_suite_in_hand(cards_, DIAMOND);
		res += highest_cards_of_suite_in_hand(cards_, CLUB);
		res.sort();
		DBG("highest_cards_in_hand :" << res << "\n")
		return res;
	}

	Cards all_cards_that_trick(const Card &c_, const Cards &cards_) const
	{
		Cards res;
		Cards cards(cards_);
		cards.sort(_trump); // sort with trumps in first place
		for (auto c : cards)
		{
			if (card_tricks(c, c_))
			{
				res.push_front(c); // valuable trump tricks go to end
			}
		}
		DBG("all_cards_that_trick: " << cards_ << " - " << c_ << " => " << res << "\n");
		return res;
	}

	size_t ai_play_20_40()
	{
		size_t move = NO_MOVE;
		Suites suites = have_40(_ai.cards);
		if (suites.size())
		{
			// ai has 40, play out queen
			_marriage = MARRIAGE_40;
			bell(AI_MARRIAGE_40);
			size_t trump_queen = find(Card(QUEEN, _trump), _ai.cards);
			move = trump_queen;
			LOG("AI declares 40 with " << _ai.cards[move] << "\n");
			_ai.s20_40.push_front(_ai.cards[move].suite());
			if (_ai.deck.empty())
			{
				_ai.pending += 40;
			}
			else
			{
				_ai.score += 40;
			}
		}
		else if ((suites = have_20(_ai.cards)).size())
		{
			_marriage = MARRIAGE_20;
			bell(AI_MARRIAGE_20);
			size_t first_suite_queen = find(Card(QUEEN, suites[0]), _ai.cards);
			move = first_suite_queen;
			LOG("AI declares 20 with " << _ai.cards[move] << "\n");
			_ai.s20_40.push_front(_ai.cards[move].suite());
			if (_ai.deck.empty())
			{
				_ai.pending += 20;
			}
			else
			{
				_ai.score += 20;
			}
		}
		return move;
	}

	size_t ai_move_closed_follow()
	{
		// end game, player has moved, ai to follow
		size_t move = must_give_color_or_trick(_player.card, _ai.cards);
		assert(move != NO_MOVE);
		return move;
	}

	size_t ai_move_closed_lead()
	{
		// end game, ai plays out
		size_t move = NO_MOVE;

		size_t m = ai_play_20_40();
		if (m != NO_MOVE)
		{
			move = m;
		}
		else
		{
			Cards trump_claim = cards_to_claim(_trump);
			if (trump_claim.size() && (int)trump_claim.size() >= max_trumps_player())
			{
				move = find(trump_claim[0], _ai.cards);
			}
			else
			{
				Cards claim = cards_to_claim();
				if (claim.size())
				{
					move = find(claim[0], _ai.cards);
				}
			}
		}
		if (ai_test_close())
		{
			if (_marriage == NO_MARRIAGE)
			{
				move = find(highest_cards_in_hand(_ai.cards)[0], _ai.cards);
			}
		}
		return move;
	}

	size_t ai_move_follow()
	{
		// normal game, player has moved, ai to follow
		size_t move = NO_MOVE;
		Suites s20 = have_20(_ai.cards);
		Suites s40 = have_40(_ai.cards);
		if (s20.size() || s40.size() || _player.card.value() >= 10)
		{
			// clumsily try to not destroy a 40 by tricking
			// TODO: better method for such things...
			Cards temp = _ai.cards;
			if (s40.size())
			{
				temp.erase(temp.begin() + find(Card(QUEEN,_trump), temp));
				temp.erase(temp.begin() + find(Card(KING,_trump), temp));
			}
			size_t m = lowest_card_that_tricks(_player.card, temp);
			if (m != NO_MOVE)
				move = m;
		}
		else
		{
			Cards tricks = all_cards_that_trick(_player.card, _ai.cards);
			if (tricks.size())
			{
				size_t m = find(tricks[best_trick_card(_player.card, tricks)], _ai.cards);
				const Card &c = _ai.cards[m];
				// trick or not trick?
				bool trick(false);
				int score = _ai.cards[m].value() + _player.card.value() + _ai.pending;
				if (_ai.score < 33 && _ai.score + score >= 33)
					trick = true;
				else if (_ai.score >= 33 && _ai.score + score >= 66)
					trick = true;
				else if (_cards.size() <= 2 && _ai.score <= 50 && _ai.score + score >= 60)
					trick = true;
				else if (_player.card.suite() != _trump && c.suite() != _trump)
					trick = true;
				if (trick)
					move = m;
			}
		}
		return move;
	}

	size_t ai_move_lead()
	{
		// normal game, ai plays out
		size_t move = NO_MOVE;

		test_change(_ai.cards);

		size_t m = ai_play_20_40();
		if (m != NO_MOVE)
			move = m;

		if (ai_test_close())
		{
			if (_marriage == NO_MARRIAGE)
			{
				move = find(highest_cards_in_hand(_ai.cards)[0], _ai.cards);
			}
		}

		return move;
	}

	size_t ai_move()
	{
		_marriage = NO_MARRIAGE;
		assert(_ai.cards.size());
		size_t move = lowest_card(_ai.cards); // default move lowest card
		assert(move != NO_MOVE);

		if (_closed != NOT && _player.move_state == ON_TABLE)
		{
			size_t m = ai_move_closed_follow();
			if (m != NO_MOVE)
				move = m;
		}
		else if (_closed != NOT)
		{
			size_t m = ai_move_closed_lead();
			if (m != NO_MOVE)
				move = m;
		}
		else
		{
			if (_player.move_state == ON_TABLE)
			{
				size_t m = ai_move_follow();
				if (m != NO_MOVE)
					move = m;
			}
			else
			{
				size_t m = ai_move_lead();
				if (m != NO_MOVE)
					move = m;
			}
		}
		assert(move != NO_MOVE);
		_ai.card = _ai.cards[move];

		_ai.cards.erase(_ai.cards.begin() + move);

		animate_ai_move();

		_ai.move_state = ON_TABLE;
		redraw();
		LOG("AI move: " << _ai.card << "\n");
		return move;
	}

	void handle_move()
	{
		if (_player.move_state != NONE && _ai.move_state == NONE)
		{
			if (_move == PLAYER)
			{
				test_20_40(Fl::event_x(), Fl::event_y());
			}
		}
		else
		{
			_player.deck_info = _player.deck.size() &&
			                    _player.deck.front().rect().includes(Fl::event_x(), Fl::event_y());
			_ai.deck_info = _ai.deck.size() &&
			                _ai.deck.front().rect().includes(Fl::event_x(), Fl::event_y());
		}
		redraw();
	}

	void handle_key()
	{
		if (Fl::event_key('q') && !_disabled) // just for testing -> redeal
		{
			init();
		}
		else if (Fl::event_key('d')) // just for testing -> debug
		{
			debug();
		}
		else if (Fl::event_key(FL_F + 1) && idle())
		{
			welcome();
		}
		else if (Fl::event_key(FL_F + 12) && ::debug) // just for testing -> cmd
		{
			toggle_cmd_input();
		}
		else if (Fl::event_key(FL_F + 10)) // toggle fullscreen
		{
			toggle_fullscreen();
		}
	}

	void handle_click()
	{
		if (_disabled)
		{
			_disabled = false;
			return;
		}
		_marriage = NO_MARRIAGE;
		if (test_close(Fl::event_x(), Fl::event_y()) == true)
		{
			return;
		}
		if (_player.move_state != NONE)
		{
			if (_player.move_state == MOVING &&
				(Fl::event_button() > 1 || !valid_move(_player.card)))
			{
					// withdraw move, put card back to hand
				LOG("withdraw or invalid " <<  _player.card << "\n");
				_player.cards.push_back(_player.card);
				_player.move_state = NONE;
				_player.cards.sort();
				return;
			}
			if (_ai.move_state == NONE)
			{
				test_20_40(Fl::event_x(), Fl::event_y());
			}

			if (_closed == NOT && _cards.size() && _ai.move_state == NONE &&
		    _cards.back().rect().includes(Fl::event_x(), Fl::event_y()))
			{
				test_change();
				return;
			}
			// beware of unwanted (unsucessfull) click on change
			if (Fl::event_x() < w() / 2)
				return;

			// make accepted move
			if (_player.move_state == MOVING)
			{
				if (_marriage == MARRIAGE_20)
				{
					LOG("Player declares 20 with " << _player.card << "\n");
					_player.s20_40.push_front(_player.card.suite());
					bell(YOU_MARRIAGE_20);
					if (_player.deck.empty())
					{
						// must make at least one trick, before
						// 20/40 is counted!
						_player.pending += 20;
					}
					else
					{
						_player.score += 20;
					}
				}
				if (_marriage == MARRIAGE_40)
				{
					LOG("Player declares 40 with " << _player.card << "\n");
					_player.s20_40.push_front(_player.card.suite());
					bell(YOU_MARRIAGE_40);
					if (_player.deck.empty())
					{
						// must make at least one trick, before
						// 20/40 is counted!
						_player.pending += 40;
					}
					else
					{
						_player.score += 40;
					}
				}
				_player.move_state = ON_TABLE; // _player.card is on table
				return;
			}
		}
		assert(_player.cards.size());
		for (size_t i = 0; i < _player.cards.size(); i++)
		{
			const Card &c = _player.cards[i];
			if (c.rect().includes(Fl::event_x(), Fl::event_y()))
			{
				_player.card = _player.cards[i];
				_player.cards.erase(_player.cards.begin() + i);
				_player.move_state = MOVING;
				LOG("PL move: " << _player.card << "\n");
			}
		}
	}

	int handle(int e_)
	{
		if (e_ == FL_NO_EVENT)
		{
			// NOTE: ignore FL_NO_EVENT which is received massively
			//       during an XWayland session => CPU load 100%!!
			return 1;
		}
		int ret = Fl_Double_Window::handle(e_);
		if (e_ == FL_MOVE )
		{
			// reduce excessive FL_MOVE event processing...
			static std::chrono::time_point<std::chrono::system_clock> start =
				std::chrono::system_clock::now();
			std::chrono::time_point<std::chrono::system_clock> end =
				std::chrono::system_clock::now();
			std::chrono::duration<double> diff = end - start;
			if (diff.count() < 1./20) return ret;
			start = end;
			handle_move();
		}
		else if (e_ == FL_KEYDOWN)
		{
			handle_key();
		}
		else if (e_ == FL_PUSH)
		{
			error_message(NO_MESSAGE);
			_move == AI ? _ai.last_drawn = Card() : _player.last_drawn = Card();
			handle_click();
			return 1;
		}
		return ret;
	}

	void draw_gamebook(int x_, int y_)
	{
		fl_color(fl_lighter(fl_lighter(FL_YELLOW)));
		x_ -= _CW / 2;
		y_ -= _CH / 2;
		fl_rectf(x_, y_, _CW, _CH);
		fl_color(FL_GRAY);
		fl_rect(x_, y_, _CW, _CH);
		fl_color(FL_BLACK);
		fl_font(FL_COURIER, _CH / 14);
		int X = x_ + _CW / 20;
		int Y = y_ + fl_descent() + fl_height();
		draw_color_text(message(GAMEBOOK), X, Y, text_colors);
		fl_line_style(FL_SOLID, 2);
		fl_line(X, Y + fl_descent(), X + _CW - _CW / 10, Y + fl_descent());
		Y += _CH / 10;
		draw_color_text(message(GB_HEADLINE), X, Y, text_colors);
		int H = _CH - _CH / 5;
		fl_line_style(FL_SOLID, 1);
		int W = _CW - _CW / 10;
		fl_line(X, Y + fl_descent(), X + W, Y + fl_descent());
		fl_line(X + W / 2, Y - fl_height(), X + W / 2, Y + H - fl_descent());
		Y += fl_descent();

		int player_score = 0;
		int ai_score = 0;

		auto draw_score = [&](std::pair<int, int> s) -> void
		{
			char buf[50];
			char pbuf[20];
			char abuf[20];
			if (MATCH_SCORE - player_score <= 0 || MATCH_SCORE - ai_score <= 0) return;
			if (!s.first && !s.second)
			{
				snprintf(pbuf, sizeof(pbuf), "%d", MATCH_SCORE - player_score);
				snprintf(abuf, sizeof(abuf), "%d", MATCH_SCORE - ai_score);
			}
			else
			{
				snprintf(pbuf, sizeof(pbuf), "%d", MATCH_SCORE - player_score);
				if (!s.first || pbuf[0] == '0') pbuf[0] = '-';
				snprintf(abuf, sizeof(abuf), "%d", MATCH_SCORE - ai_score);
				if (!s.second || abuf[0] == '0') abuf[0] = '-';
			}
			snprintf(buf, sizeof(buf),"  %2s      %2s", pbuf, abuf);
			Y += _CH / 12;
			draw_color_text(buf, X, Y, text_colors);
		};

		// limit display to last 8 scores
		size_t first = _gamebook.size() > 8 ? _gamebook.size() - 8 : 0;
		if (first == 0)
			draw_score(std::make_pair(0, 0));
		for (size_t i = 0; i < _gamebook.size(); i++)
		{
			auto s = _gamebook[i];
			player_score += s.first;
			ai_score += s.second;
			if (i < first) continue;
			draw_score(s);
		}
		if (ai_score >= MATCH_SCORE || player_score >= MATCH_SCORE)
		{
			// draw "bummerl"
			char buf[40];
			Y += _CH / 12;
			snprintf(buf, sizeof(buf),"   %s       %s",
				(ai_score >= MATCH_SCORE ? "●" : " "), (player_score >= MATCH_SCORE ? "●" : " "));
			draw_color_text(buf, X, Y, text_colors);
		}
	}

	void draw_deck_info(int x_, int y_, const Cards &deck_, int max_tricks_ = 8)
	{
		fl_color(fl_lighter(fl_lighter(FL_YELLOW)));
		fl_rectf(x_, y_, w() / 10, w() / 7);
		fl_color(FL_GRAY);
		fl_rect(x_, y_, w() / 10, w() / 7);
		fl_color(FL_BLACK);
		fl_font(FL_COURIER, w() / 50);
		for (size_t i = 0; i < deck_.size(); i +=2)
		{
			max_tricks_--;
			if (max_tricks_ < 0) break;
			std::ostringstream os;
			os << " ";
			if (deck_[i].is_red_suite())
				os << "^r";
			else
				os << "^B";
			os << deck_[i];
			os << "^G";
			os << "|";
			if (deck_[i + 1].is_red_suite())
				os << "^r";
			else
				os << "^B";
			os << deck_[i + 1];
			std::string s = os.str();
			draw_color_text(s, x_ + w() / 80, y_ + w() / 50 + i * w() / 100, text_colors);
		}
	}

	void draw_player_deck_info(int x_, int y_)
	{
		draw_deck_info(x_, y_, _player.deck);
	}

	void draw_ai_deck_info(int x_, int y_)
	{
		draw_deck_info(x_, y_, _ai.deck, 1);
	}

	void draw_blob(const char *text_, Fl_Color c_, int x_, int y_)
	{
		if (!(_player.move_state != NONE || _ai.move_state == ON_TABLE))
			return;

		if (_player.move_state == ON_TABLE)
		{
			Rect r(_player.card.rect());
			x_ = r.x + r.w / 2 ;
			y_ = r.y + r.h / 2;
		}
		else if (_ai.move_state == ON_TABLE)
		{
			Rect r(_ai.card.rect());
			x_ = r.x + r.w / 2 ;
			y_ = r.y + r.h / 2;
		}
		int D = h() / 10;
		fl_color(c_);
		fl_pie(x_-D / 2, y_- D / 2, D, D, 0., 360.);
		fl_color(FL_WHITE);
		fl_font(FL_HELVETICA|FL_BOLD, D / 2);
		fl_draw(text_, x_ - fl_width(text_) / 2, y_ + fl_height() / 2 - fl_descent());
	}

	void draw_suite_symbol(CardSuite suite_, int x_, int y_)
	{
		fl_font(FL_HELVETICA, _CH / 7);
		fl_color(FL_BLACK);
		Card c(ACE, suite_);
		std::ostringstream os;
		if (c.is_red_suite())
			os << "^r";
		else
			os << "^B";

		std::string symbol_image = cardset_dir() + suite_symbols_image[c.suite()];
		if (std::filesystem::exists(symbol_image))
		{
			Fl_SVG_Image svg(symbol_image.c_str());
			svg.resize(svg.w(), svg.h());
			svg.scale(fl_width("M"), fl_height() - 2 * fl_descent(), 0, 1);
			svg.draw(x_ - svg.w(), y_ - fl_height() + fl_descent());
		}
		else
		{
			os << c.suite_symbol();
			std::string text = os.str();
			draw_color_text(text, x_ - fl_width(text.c_str()) / 2, y_, text_colors);
		}
	}

	std::string background_image()
	{
		std::string def_image(homeDir() + "rsc/deck.gif");
		std::string image = config["background"];
		if (image == "NONE") return "";
		if (image == "") return def_image;
		return image;
	}

	Fl_Color background_color()
	{
		Fl_Color def_color(FL_CYAN);
		std::string color = config["background"];
		if (color.empty() || !isdigit(color[0]))
			return def_color;
		// return color at index in FLTK palette
		return Fl_Color(atoi(color.c_str()));
	}

	void draw_table()
	{
		static Fl_Shared_Image *bg(Fl_Shared_Image::get(background_image().c_str()));
		if (bg && bg->w() && bg->h())
		{
			Fl_Tiled_Image tbg(bg, w(), h());
			tbg.draw(0, 0, w(), h());
		}
		else
		{
			fl_rectf(0, 0, w(), h(), background_color());
		}
	}

	void draw_messages()
	{
		if (_player.message != NO_MESSAGE)
		{
			std::string player_message = message(_player.message);
			fl_font(FL_HELVETICA, h()/(player_message.back() == '!' ? 15 : 25));
			fl_color(FL_RED);
			fl_draw(player_message.c_str(), w() / 4 - fl_width(player_message.c_str()) / 2, h() - h() / 8);
		}
		if (_ai.message != NO_MESSAGE)
		{
			std::string ai_message = message(_ai.message);
			fl_font(FL_HELVETICA, h() / (ai_message.back() == '!' ? 15 : 25));
			fl_color(FL_RED);
			fl_draw(ai_message.c_str(), w() / 4 - fl_width(ai_message.c_str()) / 2, h() / 8);
		}
		if (_error_message != NO_MESSAGE)
		{
			std::string error_message = message(_error_message);
			fl_color(FL_RED);
			fl_rectf(0, h() - h() / 40, w(), h() / 40);
			fl_font(FL_HELVETICA|FL_BOLD, h() / 50);
			fl_color(FL_WHITE);
			fl_draw(error_message.c_str(), w() / 2 - fl_width(error_message.c_str()) / 2, h() - fl_descent());
		}
		if (_closed != NOT && _ai.display_score == false)
		{
			fl_font(FL_HELVETICA, _CH / 7);
			fl_color(FL_GRAY);
			static const std::string closed_sym = "⛒";
			if (_closed == BY_AI)
			{
				int X = w() / 4 - fl_width(closed_sym.c_str()) / 2;
				int Y = h() / 8 - _CH / 7;
				fl_draw(closed_sym.c_str(), X, Y);
			}
			if (_closed == BY_PLAYER)
			{
				int X = w() / 4 - fl_width(closed_sym.c_str()) / 2;
				int Y = h() - h() / 16;
				fl_draw(closed_sym.c_str(), X, Y);
			}
		}
	}

	void draw_20_40_suites()
	{
#if 0
		// TESTONLY
		_player.s20_40.clear();
		_player.s20_40.push_front(CardSuite(HEART));
		_player.s20_40.push_front(CardSuite(SPADE));
		_player.s20_40.push_front(CardSuite(CLUB));
		_player.s20_40.push_front(CardSuite(DIAMOND));
		_ai.s20_40.clear();
		_ai.s20_40.push_front(CardSuite(HEART));
		_ai.s20_40.push_front(CardSuite(SPADE));
		_ai.s20_40.push_front(CardSuite(CLUB));
		_ai.s20_40.push_front(CardSuite(DIAMOND));
		_player.score = 56;
		_ai.score = 56;
#endif

		for (size_t i = 0; i < _player.s20_40.size(); i++)
		{
			draw_suite_symbol(_player.s20_40[i], w() - w() / 30 * (i + 1), h() - 6);
		}
		for (size_t i = 0; i < _ai.s20_40.size(); i++)
		{
			draw_suite_symbol(_ai.s20_40[i], w() - w() / 30 * (i + 1), _CH / 6);
		}
	}

	void draw_cards()
	{
		_back.image()->scale(_CW, _CH, 0, 1);
		for (size_t i = 0; i < _ai.cards.size(); i++)
		{
			int X = ((i + 1) * w()) / 20 + w() / 2 - w() / 24;
			int Y = h()/40;
			_back.skewed_image()->draw(X, Y);
		}
		for (size_t i = 0; i < _player.cards.size(); i++)
		{
			Card &c = _player.cards[i];
			Fl_RGB_Image *image = c.image();
			int X = ((i + 1) * w()) / 20 + w() / 2 - w() / 24;
			int Y = h() - _CH - h() / 40;
			image->scale(_CW, _CH, 0, 1);
			if (_player.last_drawn.face() != NO_FACE &&
			    _player.last_drawn.name() == c.name())
			{
				Fl_Image *temp = image->copy();
				temp->color_average(FL_YELLOW, 0.9);
				temp->draw(X, Y);
				delete temp;
			}
			else
			{
				image->draw(X, Y);
			}
			int D = _CH / 20;
			c.rect(Rect(X, Y + D, i == _player.cards.size() - 1 ? image->w() : w() / 20, _CH - 2 * D));
		}
	}

	void do_animate(int src_X_, int src_Y_, int dest_X_, int dest_Y_)
	{
		int dx = dest_X_ - src_X_;
		int dy = dest_Y_ - src_Y_;

		static const int STEPS = 5;
		for (int i = 0; i < STEPS; i++)
		{
			int X = src_X_ + (floor)(((double)dx / STEPS) * i);
			int Y = src_Y_ + (floor)(((double)dy / STEPS) * i);
			_animate_xy = std::make_pair(X, Y);
			wait(1./50);
			redraw();
		}
		_animate = nullptr;
	}

	void animate_ai_move()
	{
		int src_X = w() / 20 + w() / 2 - w() / 24 + _CW / 2;
		int src_Y = h()/40 + _CH / 4;

		int dest_X = w() - w() / 3 + _CW / 2;
		int dest_Y = h() / 5 + _CH / 2;

		_animate = &Deck::draw_animated_move;

		do_animate(src_X, src_Y, dest_X, dest_Y);
	}

	void animate_trick()
	{
		int src_X = (_move == AI ? w() - w() / 3 : w() - w() / 2) + _CW / 2;
		int src_Y = (_move == AI ? h() / 5 : h() / 4 - h() / 40) + _CH / 2;

		int dest_X = w() - _CW / 2 - 2;
		int dest_Y = (_move == AI ? h() / 10 - w() / 800 : h() - _CH - h() / 10) + _CH / 2;

		_animate = &Deck::draw_animated_trick;

		do_animate(src_X, src_Y, dest_X, dest_Y);
	}

	void draw_pack()
	{
		// _cards.back() is the trump card
		_back.image()->scale(_CW, _CH, 0, 1);
		_shadow.image()->scale(_CW, _CH, 0, 1);

		if (_cards.size() && _cards.size() != 20)
		{
			_cards.back().image()->scale(_CW, _CH, 0, 1);
			int X = w() / 3 - _CW + _CW/4;
			int Y = (h() - _CW) / 2;
			if (_closed == NOT)
			{
				_cards.back().quer_image()->draw(X, Y);
				_cards.back().rect(Rect(X, Y, _cards.back().image()->h(), _cards.back().image()->w()));
			}

			// deck position
			X = w() / 3 - _CW / 4 - _CW;
			Y = (h() - _CH) / 2;
			if (_cards.size())
			{
				for (size_t i = 0; i < _cards.size() - 1; i++)
				{
					// NOTE: 200 is a 'realistic' value for card the 'height'
					//       of the full card pack with 20 cards.
					//       But the pack with 10 cards after dealing just look
					//       better, when single cards are visible.
					//       So a compromise...
					double h = (double)_CW / (_cards.size() == 20 ? 200 : 100);
					if (h > 1.) h = (int)h;
					double x = (double)X - i * h +.5;
					double y = (double)Y - i * h + .5;
					_back.image()->draw(floor(x), floor(y));
				}
				if (_closed == NOT)
				{
					_cards.front().rect(Rect(X, Y, _cards.back().image()->w(), _cards.back().image()->h()));
				}
			}
		}
		else
		{
			// draw an outline of pack
			int X = w() / 3 - _CW - _CW / 4;
			int Y = (h() - _CH) / 2;
			_outline.image()->scale(_CW, _CH, 0, 1);
				_outline.image()->draw(X, Y);
		}

		if (_closed != NOT && _cards.size())
		{
			int X = w() / 3 - _CW + _CW / 4;
			int Y = (h() - _CW) / 2;
			_back.quer_image()->draw(X - w() / 16, Y);
		}
	}

	void draw_decks()
	{
		// show played pack
		int X = w() - _CW - 2;
		int Y = h() - _CH - h() / 10;
		for (size_t i = 0; i < _player.deck.size(); i++)
		{
			_back.image()->draw(X - i * w() / 800, Y - i * w() / 800);
		}
		for (size_t i = 0; i < _ai.deck.size(); i++)
		{
			_back.image()->draw(X - i * w() / 800, h() / 10 - (i + 1) * w() /800);
		}
		if (_player.deck.size())
		{
			// click region for deck display ("tooltip")
			_player.deck.front().rect(Rect(X, Y, _CW, _CH));
		}
		if (_ai.deck.size())
		{
			// click region for deck display ("tooltip")
			_ai.deck.front().rect(Rect(X, h() / 10, _CW, _CH));
		}
	}

	void draw_move()
	{
		// cards moving or on table
		if (_ai.move_state == ON_TABLE)
		{
			Fl_RGB_Image *image = _ai.card.image();
			image->scale(_CW, _CH, 0, 1);
			int X =  w() - w() / 3;
			int Y =  h() / 5;
			image->draw(X, Y);
			_ai.card.rect(Rect(X, Y, image->w(), image->h()));
		}
		if (_player.move_state != NONE)
		{
			Fl_RGB_Image *image = _player.card.image();
			image->scale(_CW, _CH, 0, 1);
			int X = _player.move_state == MOVING ? Fl::event_x() - image->w() / 2 : w() - w() / 2;
			int Y = _player.move_state == MOVING ? Fl::event_y() - image->h() / 2 : h() / 4 - h() / 40;
			if (_player.move_state == MOVING)
				_shadow.image()->draw(X + image->w() / 12, Y + image->w() / 12);
			image->draw(X, Y);
			_player.card.rect(Rect(X, Y, image->w(), image->h()));
		}
		// marriage declarations
		if (_marriage == MARRIAGE_20) draw_blob("20", FL_GREEN, Fl::event_x(), Fl::event_y());
		if (_marriage == MARRIAGE_40) draw_blob("40", FL_RED, Fl::event_x(), Fl::event_y());
	}

	void draw_scores()
	{
		if (_player.score)
		{
			fl_font(FL_HELVETICA, h() / 25);
			fl_color(FL_BLUE);
			char buf[20];
			snprintf(buf, sizeof(buf), "%d", _player.score);
			fl_draw(buf, w() - fl_width(buf), h() - fl_descent());
		}
		if (_ai.score && (_ai.display_score | ::debug))
		{
			fl_font(FL_HELVETICA, h() / 25);
			fl_color(FL_BLUE);
			char buf[20];
			snprintf(buf, sizeof(buf), "%d", _ai.score);
			fl_draw(buf, w() - fl_width(buf), fl_height() - fl_descent());
		}
	}

	void draw_version()
	{
		fl_font(FL_HELVETICA, _CH / 20);
		fl_color(FL_YELLOW);
		char buf[30];
		snprintf(buf, sizeof(buf), " v%s", VERSION);
		fl_draw(buf, 0, fl_height() - fl_descent());
	}

	void draw_grayout()
	{
		if (Fl::first_window() != this || _grayout)
		{
			// use shadow image to "gray out" deck
			_shadow.image()->scale(_CW, _CH, 0, 1);
			for (int x = 0; x < w(); x += _CW / 2)
				for (int y = 0; y < h(); y += _CH / 2)
					_shadow.image()->draw(x, y, _CW / 2, _CH / 2, _CW / 4, _CH / 4);
		}
	}

	void draw_animated_trick()
	{
		_back.image()->scale(_CW, _CH, 0, 1);
		_back.image()->draw(_animate_xy.first - _CW / 2, _animate_xy.second - _CH / 2);
	}

	void draw_animated_move()
	{
		_ai.card.image()->scale(_CW, _CH, 0, 1);
		_shadow.image()->scale(_CW, _CH, 0, 1);

		int X = _animate_xy.first - _CW / 2;
		int Y = _animate_xy.second - _CH / 2;
		_shadow.image()->draw(X + _CW / 12, Y + _CH / 12);
		_ai.card.image()->draw(X, Y);
	}

	void draw()
	{
		// measure a "standard card"
//		_card_template.image()->scale(w() / 8, w() / 2, 1, 1);
		int W = w() / 8;
		int H = 1.5 * W;
		_card_template.image()->scale(W, H, 0, 1);
		_CW = _card_template.image()->w();
		_CH = _card_template.image()->h();
		draw_table();
		draw_gamebook(w() / 40 + _CW / 2, h() / 2);
		draw_suite_symbol(_trump, w() / 3 - _CW / 4, h() - h() / 2 + _CH / 2 + _CH / 5);
		draw_messages();
		draw_20_40_suites();
		draw_cards();
		draw_pack();
		draw_decks();
		draw_move();
		draw_scores();
		if (_animate)
			std::invoke(_animate, this);
		if (_player.deck_info)
			draw_player_deck_info(Fl::event_x(), Fl::event_y());
		if (_ai.deck_info)
			draw_ai_deck_info(Fl::event_x(), Fl::event_y());
		if (_cmd_input)
			_cmd_input->draw();
		if (!_disabled && _redeal_button && _redeal_button->visible())
			_redeal_button->draw();
		draw_version();
		draw_grayout();
	}

	void collect()
	{
		LOG("collect\n");
		for (auto &c : _player.cards)
			_cards.push_front(c);
		for (auto &c : _player.deck)
			_cards.push_front(c);
		for (auto &c :_ai.cards)
			_cards.push_front(c);
		for (auto &c :_ai.deck)
			_cards.push_front(c);
		_player.cards.clear();
		_ai.cards.clear();
		_player.deck.clear();
		_ai.deck.clear();
		if (_player.move_state != NONE)
			_cards.push_front(_player.card);
		if (_ai.move_state != NONE)
			_cards.push_front(_ai.card);
		if (_cards.size() != 20)
			DBG("#cards: " << _cards.size())
		assert(_cards.size() == 20);
		_cards.check();
	}

	void onCmd()
	{
		DBG("Your command: '" << _cmd << "'\n")
		if (_cmd.find("debug") == 0)
			debug();
		else if (_cmd.find("error=") == 0)
			error_message((Message)atoi(_cmd.substr(6).c_str()));
		else if (_cmd.find("message=") == 0)
		{
			fl_message_font_ = FL_COURIER;
			fl_message_size_ = h() / 40;
			Message m = (Message)atoi(_cmd.substr(8).c_str());
			if (m == YOU_WIN)
				show_win_msg();
			else
				fl_alert("%s", message(m).c_str());
		}
		else if (_cmd.find("gb=") == 0)
		{
			std::string args = _cmd.substr(3);
			if (args.empty())
				_gamebook.clear();
			else if(args.size() >= 3)
			{
				auto first = atoi(args.c_str());
				auto second = atoi(&args[2]);
				_gamebook.push_back(std::make_pair(first, second));
			}
		}
		else if (_cmd.find("cip") == 0)
		{
			OUT(Card::suite_symbol(HEART) << ": " << cards_in_play(HEART) << " (" << max_cards_player(HEART) << ")\n");
			OUT(Card::suite_symbol(SPADE) << ": " << cards_in_play(SPADE) << " (" << max_cards_player(SPADE) << ")\n");
			OUT(Card::suite_symbol(DIAMOND) << ": " << cards_in_play(DIAMOND) << " (" << max_cards_player(DIAMOND) << ")\n");
			OUT(Card::suite_symbol(CLUB) << ": " << cards_in_play(CLUB) << " (" << max_cards_player(CLUB) << ")\n");
		}
		else if (_cmd == "help")
		{
			OUT("debug/error/gb/cip\n");
		}
		else
		{
			bell();
		}
		redraw();
	}

	void toggle_fullscreen()
	{
		if (fullscreen_active())
		{
			fullscreen_off();
		}
		else
		{
			this->fullscreen();
		}
		config["fullscreen"] = fullscreen_active() ? "1" : "0";
	}

	void toggle_cmd_input()
	{
		if (_cmd_input)
		{
			delete _cmd_input;
			_cmd_input = nullptr;
			redraw();
			return;
		}
		begin();
		_cmd_input = new Cmd_Input(_CW / 20, h() - _CW / 5 - _CW / 20, _CW, _CW / 5);
		end();
		_cmd_input->take_focus();
		redraw();
		_cmd_input->when(FL_WHEN_ENTER_KEY|FL_WHEN_NOT_CHANGED);
		_cmd_input->callback([](Fl_Widget *w_, void *)
		{
			std::string c((static_cast<Cmd_Input *>(w_))->value());
			Deck *deck = static_cast<Deck *>((static_cast<Cmd_Input *>(w_))->window());
			deck->_cmd = c; // store cmd for onCmd()
			Fl::add_timeout(0.0, [](void *d_)
			{
				Deck *deck = static_cast<Deck *>(d_);
				deck->onCmd();
			}, deck);
		});
	}

	void dump_cards(const Cards &cards_, const std::string &title_) const
	{
		LOG(title_ << " (" << cards_.size() << " cards):" << "\n");
		LOG(cards_);
		LOG("\n");
	}

	void debug() const
	{
		if (_cards.size() == 20 || _cards.size() == 10)
			dump_cards(_cards, "Deck");
		LOG("AI cards: " << _ai.cards);
		LOG("\tscore: " << _ai.score << " pending: " << _ai.pending);
		LOG("\t20/40: ");
		for ([[maybe_unused]]auto s : _ai.s20_40)
			LOG(Card::suite_symbol(s));
		LOG("\n");

		LOG("PL cards: " << _player.cards);
		LOG("\tscore: " << _player.score << " pending: " << _player.pending);
		LOG("\t20/40: ");
		for ([[maybe_unused]]auto s : _player.s20_40)
			LOG(Card::suite_symbol(s));
		LOG("\n");
	}

	void init()
	{
		collect();
		player_message(NO_MESSAGE);
		ai_message(NO_MESSAGE);
		error_message(NO_MESSAGE);
		_closed = NOT;
		_marriage = NO_MARRIAGE;
		_player.s20_40.clear();
		_ai.s20_40.clear();
		_move = PLAYER;
		_player.score = 0;
		_player.pending = 0;
		_ai.score = 0;
		_ai.pending = 0;
		_ai.display_score = false;
		_player.message = NO_MESSAGE;
		_ai.message = NO_MESSAGE;
		_error_message = NO_MESSAGE;
		_player.move_state = NONE;
		_ai.move_state = NONE;
		_disabled = false;
		_player.deck_info = false;
		_ai.deck_info = false;
		_cards.shuffle();
		assert(_cards.size() == 20);
		bell(SHUFFLE);
		debug();
		deal();
		assert(_player.cards.size() == 5);
		assert(_ai.cards.size() == 5);
		_player.cards.sort();
		_ai.cards.sort();
		assert(_player.cards.size() == 5);
		assert(_ai.cards.size() == 5);
		redraw();
		debug();
	}

	void show_win_msg()
	{
		std::string m(message(YOU_WIN));
		Fl::add_timeout(0.0, [](void *d_)
		{
			std::string *m = (std::string *)d_;
			std::string s;
			const char *p = m->c_str();
			while (*p)
			{
				int len = fl_utf8len1(*p);
				std::string c = m->substr(p - m->c_str(), len);
				p += len;
				if (c == "♥" || c == "♦" || c == "\n")
					s.append(c);
				else
					s.push_back(' ');
			}

			Fl_Window *win = Fl::first_window();
			if (win)
			{
				Fl_Box *b = (Fl_Box *)win->child(0);
				if (b->label() == nullptr || std::string(b->label()) != *m) return;
				Fl_Box *box = new Fl_Box(b->x(), b->y(), b->w(), b->h());
				box->color(b->color());
				box->labelcolor(FL_RED);
				box->labelfont(b->labelfont());
				box->labelsize(b->labelsize());
				box->align(b->align());
				box->copy_label(s.c_str());
				win->insert(*box, 99);
				win->redraw();
			}
		}, &m);
		fl_alert("%s", m.c_str());
	}

	bool check_end_match()
	{
		int pscore = 0;
		int ascore = 0;
		for (auto s : _gamebook)
		{
			pscore += s.first;
			ascore += s.second;
		}
		LOG("match score PL:AI: " << pscore << ":" << ascore << "\n");
		fl_message_font_ = FL_COURIER;
		fl_message_size_ = h() / 40;
		if (pscore >= MATCH_SCORE)
		{
			LOG("You win match " << pscore << ":" << ascore << "\n");
			_player.matches_won++;
			stats["player_matches_won"] = std::to_string(_player.matches_won);
			bell(YOU_WIN);
			show_win_msg();
			_gamebook.clear();
			redraw();
			return true;
		}
		else if (ascore >= MATCH_SCORE)
		{
			LOG("AI wins match " << ascore << ":" << pscore << "\n");
			_ai.matches_won++;
			stats["ai_matches_won"] = std::to_string(_ai.matches_won);
			std::string m(message(YOU_LOST));
			bell(YOU_LOST);
			fl_alert("%s", m.c_str());
			_gamebook.clear();
			redraw();
			return true;
		}
		return false;
	}

	void update_gamebook()
	{
		if (_closed != NOT && _closed != AUTO)
		{
			// game was closed, now the closer must have enough points
			if (_closed == BY_PLAYER)
			{
				if (_player.score >= 66)
				{
					_gamebook.push_back(std::make_pair(_ai.score < 33 ? _ai.score == 0 ? 3 : 2 : 1, 0));
				}
				else
				{
					// TODO: officially the points are counted at the moment of closing
					_gamebook.push_back(std::make_pair(0, _player.score < 33 ? _player.score == 0 ? 3 : 2 : 2));
				}
			}
			else
			{
				// closed by AI
				if (_ai.score >= 66)
				{
					_gamebook.push_back(std::make_pair(0, _player.score < 33 ? _player.score == 0 ? 3 : 2 : 1));
				}
				else
				{
					// TODO: officially the points are counted at the moment of closing
					_gamebook.push_back(std::make_pair(_ai.score < 33 ? _ai.score == 0 ? 3 : 2 : 2, 0));
				}
			}
		}
		else
		{
			// normal game (not closed)
			if (_move == PLAYER)
			{
				_gamebook.push_back(std::make_pair(_ai.score < 33 ? _ai.score == 0 ? 3 : 2 : 1, 0));
			}
			else
			{
				_gamebook.push_back(std::make_pair(0, _player.score < 33 ? _player.score == 0 ? 3 : 2 : 1));
			}
		}
	}

	int run()
	{
		Player playout(PLAYER);
		while (Fl::first_window())
		{
			game(playout);
			cursor(FL_CURSOR_DEFAULT);
			playout = playout == PLAYER ? AI : PLAYER;
			update_gamebook();

			if (!Fl::first_window()) break;

			auto s = _gamebook.back();
			if (s.first)
				LOG("PL scores " << s.first << "\n");
			if (s.second)
				LOG("AI scores " << s.second << "\n");
			redraw();
			check_end_match();
		}
		save_config();
		save_stats();
		return 0;
	}

	void save_values_to_file(std::ofstream &of_, const string_map &values_, const std::string &id_) const
	{
		for (auto c : values_)
		{
			std::string name = c.first;
			std::string value = c.second;
			DBG("[save " << id_ << "] " << name << " = " << value << "\n");
			of_ << name << "=" << value << "\n";
		}
	}

	void save_config() const
	{
		config.erase("cards"); // don't save cards string!
		std::ofstream cfg(homeDir() + APPLICATION + ".cfg");
		config["width"] = std::to_string(w());
		config["height"] = std::to_string(h());
		save_values_to_file(cfg, config, "cfg");
	}

	void save_stats() const
	{
		std::ofstream stat(homeDir() + APPLICATION + ".sta");
		save_values_to_file(stat, stats, "stat");
	}

	bool ai_wins(const std::string &log_, Message player_message_ = NO_MESSAGE)
	{
		LOG(log_);
		ai_message(AI_GAME, true);
		player_message(player_message_);
		_ai.games_won++;
		stats["ai_games_won"] = std::to_string(_ai.games_won);
		_ai.display_score = true;
		wait(2.0);
		return true;
	}

	bool player_wins(const std::string &log_, Message ai_message_ = NO_MESSAGE)
	{
		LOG(log_);
		player_message(YOUR_GAME, true);
		ai_message(ai_message_);
		_player.games_won++;
		stats["player_games_won"] = std::to_string(_player.games_won);
		_ai.display_score = true;
		wait(2.0);
		return true;
	}

	void deal()
	{
		// 3 cards to player
		for (int i = 0; i < 3; i++)
		{
			Card c = _cards.front();
			_move == PLAYER ? _player.cards.push_front(c) : _ai.cards.push_front(c);
			_cards.pop_front();
		}
		// 3 cards to ai
		for (int i = 0; i < 3; i++)
		{
			Card c = _cards.front();
			_move == PLAYER ? _ai.cards.push_front(c) : _player.cards.push_front(c);
			_cards.pop_front();
		}
		// trump card
		Card trump = _cards.front();
		_cards.pop_front();
		_cards.push_back(trump); // will be the last card (_cards.back())
		_trump = trump.suite();
		LOG("trump: " << suite_symbols[_trump] << "\n");

		// 2 cards to player
		for (int i = 0; i < 2; i++)
		{
			Card c = _cards.front();
			_move == PLAYER ? _player.cards.push_front(c) : _ai.cards.push_front(c);
			_cards.pop_front();
		}
		// 2 cards to ai
		for (int i = 0; i < 2; i++)
		{
			Card c = _cards.front();
			_move == PLAYER ? _ai.cards.push_front(c) : _player.cards.push_front(c);
			_cards.pop_front();
		}

		// TEST TEST
		Suites res;
		res = have_40(_player.cards);
		if (res.size())
			DBG("player cards contain 40!\n")
		res = have_20(_player.cards);
		if (res.size())
			DBG("player cards contain " << res.size() << "x20!\n")
		res = have_40(_ai.cards);
		if (res.size())
			DBG("AI cards contain 40!\n")
		res = have_20(_ai.cards);
		if (res.size())
			DBG("AI cards contain " << res.size() << "x20!\n")
		size_t i = find(Card(JACK, _cards.back().suite()), _player.cards);
		if (i != NO_MOVE)
			DBG("player cards can change Jack!\n")
		i = find(Card(JACK, _cards.back().suite()), _ai.cards);
		if (i != NO_MOVE)
			DBG("AI cards can change Jack!\n")
	}

	void bell([[maybe_unused]]Message m_ = NO_MESSAGE)
	{
#ifdef USE_MINIAUDIO
		std::string snd = sound[m_];
		if (snd.size())
		{
			snd = homeDir() + "/rsc/" + snd + ".mp3";
			DBG("play '" << snd << "'\n");
			if (std::filesystem::exists(snd))
				_audio.play(snd);
			else
				snd.erase();
		}
		if (snd.empty())
		{
			fl_beep();
		}
#else
		// NOTE: under Wayland fl_beep() outputs a \007 character to stderr.
		//       This does not work for applications run from gnome dock,
		//       most likely because stderr/stdout are redirected or disabled.
		fl_beep();
#endif
		flicker();
	}

	bool check_end()
	{
		debug();
		auto no_cards_in_play = [&]() -> bool
		{
			return _player.cards.empty() && _ai.cards.empty() && _player.move_state == NONE && _ai.move_state == NONE;
		};

		if (_closed == NOT || _closed == AUTO)
		{
			if (_move == PLAYER && _player.score >= 66)
			{
				return player_wins("Player wins!\n");
			}
			else if (_move == AI && _ai.score >= 66)
			{
				return ai_wins("AI wins!\n");
			}
			else if (no_cards_in_play())
			{
				if (_move == AI)
				{
					return ai_wins("AI wins by last trick!\n");
				}
				else
				{
					// _move = PLAYER
					return player_wins("Player wins by last trick!\n");
				}
			}
		}
		else
		{
			// closed
			if (_closed == BY_PLAYER && _move == PLAYER && _player.score >= 66)
			{
				return player_wins("Player wins closed game!\n");
			}
			else if (_closed == BY_AI && _move == AI && _ai.score >= 66)
			{
				return ai_wins("AI wins closed game!\n");
			}
			else if (no_cards_in_play())
			{
				// closed and last trick done
				if (_closed == BY_PLAYER)
				{
					return ai_wins("AI wins because player closed and has not enough!\n", YOU_NOT_ENOUGH);
				}
				else
				{
					// _closed = BY_AI
					return player_wins("Player wins because AI closed and has not enough!\n", AI_NOT_ENOUGH);
				}
			}
		}
		return false;
	}

	void check_trick(Player move_)
	{
		_marriage = NO_MARRIAGE;
		if (move_ == PLAYER)
		{
			_move = card_tricks(_ai.card, _player.card) ? AI : PLAYER;
			if (_move == AI) LOG(_ai.card << " tricks " << _player.card << "\n")
			else LOG("Player card " << _player.card << " tricks AI card " << _ai.card << "\n")
		}
		else
		{
			_move = card_tricks(_player.card, _ai.card) ? PLAYER : AI;
			if (_move == PLAYER) LOG(_player.card << " tricks " << _ai.card << "\n")
			else LOG("AI card " << _ai.card << " tricks player card " << _player.card << "\n")
		}
		LOG("next move: " << (_move == PLAYER ? "PLAYER" : "AI") << "\n")

		_player.move_state = NONE;
		_ai.move_state = NONE;
		animate_trick();

		if (_move == PLAYER) // player won trick
		{
			_player.deck.push_front(_ai.card);
			_player.deck.push_front(_player.card);
			_player.score += _player.card.value() + _ai.card.value() + _player.pending;
			_player.pending = 0;
			player_message(YOUR_TRICK);
			ai_message(NO_MESSAGE);
			redraw();
		}
		else
		{
			_ai.deck.push_back(_player.card);
			_ai.deck.push_back(_ai.card);
			_ai.score += _ai.card.value() + _player.card.value() + _ai.pending;
			_ai.pending = 0;
			ai_message(AI_TRICK);
			player_message(NO_MESSAGE);
			redraw();
		}

	}

	void fillup_cards()
	{
		if (_closed == NOT)
		{
			// give cards from pack
			if (_cards.size())
			{
				Card c = _cards.front();
				_cards.pop_front();
				_move == AI ? _ai.last_drawn = c : _player.last_drawn = c;
				if (_move == AI)
					_ai.cards.push_front(c);
				else
					_player.cards.push_front(c);
			}

			if (_cards.size())
			{
				Card c = _cards.front();
				_move == PLAYER ? _ai.last_drawn = c : _player.last_drawn = c;
				_cards.pop_front();
				if (_move == AI)
					_player.cards.push_front(c);
				else
					_ai.cards.push_front(c);
			}
			assert(_player.cards.size() == _ai.cards.size());
			_player.cards.sort();
			_ai.cards.sort();
			debug();

			if (_cards.empty())
			{
				_closed = AUTO; // same rules as closing now
				LOG("*** pack cleared - end game ***\n");
			}
		}
		redraw();
	}

	void game(Player playout_)
	{
		init();
		wait(1.0);
		_move = playout_;
		_player.move_state = NONE;
		_ai.move_state = NONE;
		cursor(FL_CURSOR_DEFAULT);
		_redeal_button->show();
		redraw();
		while (Fl::first_window() && (_player.cards.size() || _ai.cards.size()))
		{
			if (_move == PLAYER)
			{
				cursor(FL_CURSOR_DEFAULT);
				ai_message(NO_MESSAGE);
				redraw();
				_player.move_state = NONE;
				if (check_end()) break;
				player_message(_ai.move_state == NONE ? YOU_LEAD : YOUR_TURN);
				while (Fl::first_window() && _player.move_state != ON_TABLE)
				{
					Fl::wait();
				}
				_redeal_button->hide();
				if (check_end()) break; // if enough from 20/40!!

				if (!Fl::first_window()) break;
				if (_ai.move_state == ON_TABLE)
				{
					wait(1.5);
					check_trick(AI);

					if (check_end()) break;

					fillup_cards();
					wait(1.5);
				}
				else
				{
					if (check_end()) break;
					_move = AI;
				}
			}

			if (_move == AI)
			{
				player_message(NO_MESSAGE);
				if (check_end()) break;
				_ai.move_state = MOVING;
				ai_message(_player.move_state == NONE ? AI_LEADS : AI_TURN);
				cursor(FL_CURSOR_WAIT);
				wait(2.0);
				if (!Fl::first_window()) break;
				ai_move();

				if (check_end()) break; // if enough from 20/40!!

				if (_player.move_state == ON_TABLE)
				{
					wait(1.5);
					check_trick(PLAYER);

					if (check_end()) break;

					fillup_cards();
					wait(1.5);
				}
				else
				{
					if (check_end()) break;
					_move = PLAYER;
				}
			}
		}
		if (!Fl::first_window()) return;

		_marriage = NO_MARRIAGE;
		wait(2.0);
	}

	void wait(double s_)
	{
		DBG("wait(" << s_ << ")\n");
		_disabled = true;
		double min_wait = s_ > 1./50 ? 1./50 : s_;
		std::chrono::time_point<std::chrono::system_clock> start =
			std::chrono::system_clock::now();
		while (Fl::first_window() && _disabled)
		{
			Fl::wait(min_wait);
//			redraw(); // TODO/FIXME: do redraws in game loop instead
			std::chrono::time_point<std::chrono::system_clock> end =
				std::chrono::system_clock::now();
			std::chrono::duration<double> diff = end - start;
			if (diff.count() >= s_) break;
		}
		_disabled = false;
	}

	std::string make_stats() const
	{
		std::ostringstream os;
		os << message(GAMES_WON) << _player.games_won << " / " << _ai.games_won;
		os << "\t" << message(MATCHES_WON) <<  _player.matches_won << " / " << _ai.matches_won;
		return os.str();
	}

	void unit_tests()
	{
		_trump = SPADE;
		Cards temp;
		temp.push_front(Card(QUEEN, CLUB));
		temp.push_front(Card(KING, CLUB));
		temp.push_front(Card(TEN, CLUB));
		temp.push_front(Card(ACE, SPADE));
		Card c(ACE, CLUB);
		assert(can_trick_with_suite(c, temp) == false);
		assert(can_trick(c, temp) == true);
		Cards res(_cards);
		res += _cards;
		assert(res.size() == 40);
		res -= _cards;
		assert(res.size() == 20);
		res = _cards + _cards;
		assert(res.size() == 40);
		Cards c2;
		c2.push_back(Card(QUEEN, SPADE));
		c2.push_back(Card(KING, HEART));
		res = _cards - c2;
		assert(res.size() == 18);
	}

	void create_welcome()
	{
		_welcome = new Welcome(*this);
		_welcome->show();
		_welcome->wait_for_expose();
		redraw();
		bell(WELCOME);
	}

	void welcome()
	{
		Fl::add_timeout(0.0, [](void *d_)
		{
			(static_cast<Deck *>(d_))->create_welcome();
		}, this);
	}

	void flicker()
	{
		for (int i = 0; i < 2; i++)
		{
			_grayout = !_grayout;
			redraw();
			wait(0.1);
		}
		_grayout = false;
		redraw();
	}

private:
	GameData _player;
	GameData _ai;
	CardImage _back;
	CardImage _shadow;
	CardImage _outline;
	Cards _cards;			// remaining cards
	CardSuite _trump;		// trump suite
	Marriage _marriage;
	Closed _closed;
	Player _move;
	Message _error_message;
	bool _disabled;
	Card _card_template;
	int _CW;					// card pixel width (used as "unit" for UI)
	int _CH;					// card pixel height
	std::vector<std::pair<int, int>> _gamebook;
	Cmd_Input *_cmd_input;
	Button *_redeal_button;
	Welcome *_welcome;
	bool _grayout;
#ifdef USE_MINIAUDIO
	Audio _audio;
#endif
	std::string _cmd;
	std::pair<int, int> _animate_xy;
	DeckMemberFn _animate;
};

void list_decks(std::ostringstream &os_)
{
	std::string svg_cards(homeDir() + cardDir);
	os_ << "\navailaible cardsets:\n";
	for (auto const &dir_entry : std::filesystem::directory_iterator(svg_cards))
	{
		std::filesystem::path card(dir_entry.path());
		card /= Card(QUEEN, HEART).filename();
		if (dir_entry.is_directory() && std::filesystem::exists(card))
			os_ << "\t" << dir_entry.path().filename() << "\n";
	}
	std::filesystem::path back(homeDir() + cardDir + "/back");
	os_ << "\navailaible card backs:\n";
	for (auto const &dir_entry : std::filesystem::directory_iterator(back))
	{
		if (dir_entry.is_regular_file())
			os_ << "\t" << dir_entry.path().filename() << "\n";
	}
}

std::string make_help(const string_map &la_, const string_map &sa_)
{
	std::ostringstream os;
	os << APPLICATION << " " << VERSION << "\n\n";
	os << "Usage:\n";
	for (auto a : la_)
	{
		os << "--" << a.first << "\t" << a.second << "\n";
	}
	os << "\n";
	for (auto a : sa_)
	{
		os << "-" << a.first << "\t" << a.second << "\n";
	}
	list_decks(os);
	return os.str();
}

bool process_arg(const std::string &arg_, const std::string &value_)
{
	static const string_map long_args =
	{
		{ "lang", "\t{id}\t\tset language [de,en]" },
		{ "loglevel", "{level}\t\tset loglevel [0-2]" },
		{ "background", "{name/number}\tset background image or color [imagepath/[0-255]]" },
		{ "cardset", "{directory}\tuse cardset [name]" },
		{ "cardback", "{file}\t\tuse cardback image [svg]" },
		{ "cards", "\t{cards-string}\tuse this cards to play (for debugging only)" }
	};
	static const string_map short_args =
	{
		{ "C", "default config values" },
		{ "d", "enable debug" },
		{ "f", "run fullscreen" },
		{ "w", "show welcome screen" },
		{ "h", "this help" }
	};

	if (value_.size() && long_args.find(arg_.substr(2)) != long_args.end())
	{
		config[arg_.substr(2)] = value_;
	}
	else if (arg_.size() == 2 && arg_[0] == '-' && short_args.find(arg_.substr(1)) != short_args.end())
	{
		switch (arg_[1])
		{
			case 'd':
				debug = true;
				break;
			case 'f':
				config["fullscreen"] = "1";
			break;
			case 'w':
				config["welcome"] = "1";
				break;
			case 'h':
				OUT(make_help(long_args, short_args));
				return false;
			case 'C':
				config.clear();
		}
	}
	else
	{
		fl_message_font_ = FL_COURIER;
		fl_alert("Invalid argument '%s'!\n\n%s", arg_.c_str(), make_help(long_args, short_args).c_str());
		return false;
	}
	return true;
}

void parse_arg(int argc_, char *argv_[])
{
	for (int i = 1; i < argc_; i++)
	{
		std::string arg = argv_[i];
		std::string value;
		if (arg.find("--") == 0)
		{
			if (i + 1 < argc_)
			{
				i++;
				value = argv_[i];
			}
		}
		if (process_arg(arg, value) == false)
		{
			exit(0);
		}
	}
}

Welcome::Welcome(const Deck &deck_) : Fl_Double_Window(deck_.w() / 2, deck_.h() / 4 * 3),
	_deck(deck_)
{
	clear_border();
	set_modal();
	box(FL_UP_BOX);
	color(FL_WHITE);
	redraw_timer(this);
}

Welcome::~Welcome()
{
	Fl::remove_timeout(redraw_timer, this);
}

/*static*/
void Welcome::redraw_timer(void *d_)
{
	(static_cast<Welcome *>(d_))->redraw();
	Fl::add_timeout(0.2, redraw_timer, d_);
}

int Welcome::handle(int e_)
{
	if (e_ == FL_NO_EVENT) return 1;
	if (e_ == FL_PUSH || e_ == FL_KEYDOWN)
	{
		Fl::delete_widget(this);
		Fl::first_window()->redraw();
	}
	return Fl_Double_Window::handle(e_);
}

void Welcome::draw_stats()
{
	fl_font(FL_COURIER_BOLD, h() / 42);
	fl_draw_box(FL_FLAT_BOX, 0, h() - h() / 38 - fl_descent(), w(), h() / 38 + fl_descent(), fl_lighter(fl_lighter(FL_YELLOW)));
	fl_color(FL_BLACK);
	std::string stat(_deck.make_stats());
	fl_draw(stat.c_str(), (w() - fl_width(stat.c_str())) / 2, h() - h() / 38 + fl_height() - fl_descent() - 2);
}

void Welcome::draw()
{
	fl_draw_box(box(), 0, 0, w(), h(), color());
	Rect r(*this, box());
	fl_push_clip(r.x - x(), r.y - y(), r.w, r.h);
	fl_font(FL_HELVETICA_BOLD, h() / 7);
	for (int i = 0; i < 30; i++)
	{
		static const std::vector<CardSuite> suites = { HEART, SPADE, DIAMOND, CLUB };
		int x = random() % w();
		int y = random() % h();
		auto s = random() % suites.size();
		if (suites[s] == HEART || suites[s] == DIAMOND)
			fl_color(fl_lighter(fl_lighter(FL_RED)));
		else
			fl_color(fl_lighter(fl_lighter(FL_BLACK)));
		fl_draw(suite_symbols[suites[s]].c_str(), x, y + fl_height());
	}
	Card c(QUEEN, HEART);
	int W = w() / 2 - w() / 10;
	int H = 1.5 * W;
//	int H = (double)c.image()->h() / c.image()->w() * W;
	c.image()->scale(W, H, 0, 1);
	c.image()->draw(w() / 40, h() / 4);
	fl_font(FL_HELVETICA_BOLD, w() / 10);
	fl_color(FL_BLACK);
	static constexpr char title[] = "^rF^BL^rT^BK^r S^BC^rH^BN^rA^BP^rS^BE^rN^B";
	draw_color_text(title, (w() - fl_width("FLTK SCHNAPSEN")) / 2, h() / 7, text_colors);
	fl_color(FL_BLUE);
	fl_font(FL_HELVETICA_BOLD, w() / 26);
	static constexpr char cr[] = "(c) 2025 Christian Grabner <wcout@gmx.net>";
	fl_draw(cr, (w() - fl_width(cr)) / 2, h() / 7 + h() / 14);
	fl_color(FL_BLACK);
	fl_font(FL_HELVETICA_BOLD, h() / 16);
	fl_draw(message(WELCOME).c_str(), w() / 2 + w() / 60, h() / 2);
	draw_stats();
	fl_pop_clip();
}

int main(int argc_, char *argv_[])
{
	Fl::keyboard_screen_scaling(0); // disable keyboard scaling - we do that ourselves
	fl_message_title_default(message(TITLE).c_str());
	Fl::get_system_colors();
	Fl::background(240, 240,240); // brighter color for message background
	fl_register_images();
	load_config();
	load_stats();
	LOG("homeDir: " << homeDir() << "\n");
	parse_arg(argc_, argv_);
	fl_message_title_default(message(TITLE).c_str()); // redo ... maybe language changed
	srand(time(nullptr));
	Deck deck;
	deck.show();
	deck.wait_for_expose();
	if (atoi(config["welcome"].c_str()))
	{
		deck.welcome();
	}
	return deck.run();
}
