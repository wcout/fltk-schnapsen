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

#include "system.h"

constexpr char APPLICATION[] = "fltk-schnapsen";
constexpr char VERSION[] = "1.0.8";

#include "Deck.h"
namespace Schnapsen
{
	int debug = 0;
	Fl_Font CustomFont = FL_HELVETICA;
	Player first_to_move = Player::PLAYER;
	std::string game_to_load;
};
using namespace Schnapsen;

#include "debug.h"
#include "Util.cxx"
#include "CardImage.cxx"
#include "Card.cxx"
#include "Cards.cxx"
#include "GameBook.cxx"
#include "Unittest.cxx"
#include "Engine.cxx"
#include "Welcome.cxx"
#include "Selector.cxx"
#include "Alert.cxx"
#include "Deck.cxx"
#include "Deck_Cmd.cxx"
#include "Args.cxx"
#include "FontLoader.cxx"
#include "AnimText.cxx"

int main(int argc_, char *argv_[])
{
	Fl::keyboard_screen_scaling(0); // disable keyboard scaling - we do that ourselves
	fl_message_title_default(Util::message(TITLE).c_str());
	fl_message_hotspot(0);
	Fl::get_system_colors();
	Fl::background(240, 240, 240); // brighter color for message background
	srand(time(nullptr));
	Util::load_config();
	Util::load_stats();
	if (Args::parse(argc_, argv_) == false)
	{
		exit(EXIT_FAILURE);
	}
	LOG(Args::arg0 << " " << VERSION << " [" << Util::home_dir() << "]\n");
	fl_message_title_default(Util::message(TITLE).c_str()); // redo ... maybe language changed
	try
	{
		Deck deck;
		deck.xclass(APPLICATION); // otherwise Wayland/Gnome displays an 'FLTK' icon in sidebar
		deck.show();
		deck.wait_for_expose();
		if (Fl::get_key(FL_Control_L)) // reset config file by pressing 'Ctrl' key at startup
		{
			Util::config().clear();
			WNG("configuration reset\n");
		}
		if (Fl::screen_scale(deck.screen_num()) != 1)
			Fl::screen_scale(deck.screen_num(), 1);
		std::optional welcome = Util::config_value("welcome");
		if ((!welcome || atoi(welcome.value().c_str())) && game_to_load.empty())
		{
			deck.welcome();
		}
		return deck.run();
	}
	catch (const std::runtime_error &e_)
	{
		fl_alert("%s", e_.what());
	}
	return EXIT_FAILURE;
}
