//
//	Schnapsen for 2 card game for FLTK.
//
// Copyright 2025-2026 Christian Grabner.
//
#include <filesystem>
#include <string>
#include <sstream>
#include <cstdlib>

#include <FL/Fl.H>
#include <FL/fl_ask.H>

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

constexpr char APPLICATION[] = "fltk-schnapsen";
constexpr char VERSION[] = "1.0";

// only for testing
int debug = 0;

#include "Deck.h"
Player first_to_move = Player::PLAYER;

#include "debug.h"
#include "Util.cxx"
#include "CardImage.cxx"
#include "Card.cxx"
#include "Cards.cxx"
#include "GameBook.cxx"
#include "Engine.cxx"
#include "Welcome.cxx"
#include "Deck.cxx"

void list_decks(std::ostringstream &os_)
{
	std::string svg_cards(Util::homeDir() + cardDir);
	os_ << "\navailaible cardsets:\n";
	for (auto const &dir_entry : std::filesystem::directory_iterator(svg_cards))
	{
		std::filesystem::path card(dir_entry.path());
		card /= Card(QUEEN, HEART).filename();
		std::filesystem::path card_png(dir_entry.path());
		card_png /= Card(QUEEN, HEART).filename(".png");
		if (dir_entry.is_directory() &&
			(std::filesystem::exists(card) || std::filesystem::exists(card_png)))
		{
			os_ << "\t" << dir_entry.path().filename() << "\n";
		}
	}
	std::filesystem::path back(Util::homeDir() + cardDir + "/back");
	os_ << "\navailaible card backs:\n";
	for (auto const &dir_entry : std::filesystem::directory_iterator(back))
	{
		if (dir_entry.is_regular_file())
		{
			os_ << "\t" << dir_entry.path().filename() << "\n";
		}
	}
}

std::string make_help(const string_map &la_, const string_map &sa_)
{
	std::ostringstream os;
	os << APPLICATION << " " << VERSION << "\n\n";
	os << "Usage:\n";
	for (const auto &[option, description] : la_)
	{
		os << "--" << option << "\t" << description << "\n";
	}
	os << "\n";
	for (const auto &[option, description] : sa_)
	{
		os << "-" << option << "\t" << description << "\n";
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
		{ "cards", "\t{cards-string}\tuse this cards to play (for debugging only)" },
		{ "animate", "{level}\t\tanimate card moves [0-2] 0=off, 2=all" },
		{ "strict", "{strictness}\tconfirm strict(er) to the rules [0-2] 0=off, 2=all" }
	};
	static const string_map short_args =
	{
		{ "C", "default config values" },
		{ "d", "enable debug" },
		{ "f", "run fullscreen" },
		{ "w", "show welcome screen" },
		{ "r", "start with AI move" },
		{ "s", "faster response" },
		{ "h", "this help" }
	};

	if (value_.size() && long_args.find(arg_.substr(2)) != long_args.end())
	{
		Util::config(arg_.substr(2), value_);
	}
	else if (arg_.size() == 2 && arg_[0] == '-' && short_args.find(arg_.substr(1)) != short_args.end())
	{
		switch (arg_[1])
		{
			case 'd':
				debug++;
				break;
			case 'f':
				Util::config("fullscreen", "1");
			break;
			case 'r':
				first_to_move = AI;
				break;
			case 's':
				Util::config("fast", "1");
				break;
			case 'w':
				Util::config("welcome", "1");
				break;
			case 'h':
				OUT(make_help(long_args, short_args));
				return false;
			case 'C':
				Util::config().clear();
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

int main(int argc_, char *argv_[])
{
	Fl::keyboard_screen_scaling(0); // disable keyboard scaling - we do that ourselves
	fl_message_title_default(Util::message(TITLE).c_str());
	fl_message_hotspot(0);
	Fl::get_system_colors();
	Fl::background(240, 240,240); // brighter color for message background
	Util::load_config();
	Util::load_stats();
	LOG("homeDir: " << Util::homeDir() << "\n");
	parse_arg(argc_, argv_);
	fl_message_title_default(Util::message(TITLE).c_str()); // redo ... maybe language changed
	srand(time(nullptr));
	Deck deck;
	deck.show();
	deck.wait_for_expose();
	if (atoi(Util::config("welcome").c_str()))
	{
		deck.welcome();
	}
	return deck.run();
}
