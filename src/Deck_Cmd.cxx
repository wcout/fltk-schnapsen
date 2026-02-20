//
// This file is part of Deck.cxx, and is only there
// to aid debugging during development without
// cluttering Deck.cxx.
//
#include "debug.h"
#include "Util.h"
#include "messages.h"
#include "GameBook.h"
#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <string>
#include <cstdlib>

void Deck::onCmd(const std::string &cmd_)
{
	DBG("Your command: '" << cmd_ << "'\n")
	if (cmd_.find("animate=") == 0)
	{
		int value = atoi(cmd_.substr(8).c_str());
		if (value >= 0 && value <= 2)
			_animation_level = value;
		OUT("animate: " << _animation_level << "\n");
	}
	else if (cmd_.find("debug=") == 0)
	{
		int value = atoi(cmd_.substr(6).c_str());
		if (value >= 1 && value <= 4) // don't allow 0 here
			::debug = value;
		OUT("debug: " << ::debug << "\n");
	}
	else if (cmd_.find("loglevel=") == 0)
	{
		int value = atoi(cmd_.substr(9).c_str());
		if (value >= 0 && value <= 2)
			Util::config("loglevel", std::to_string(value));
		OUT("loglevel: " << atoi(Util::config("loglevel").c_str()) << "\n");
	}
	else if (cmd_.find("error=") == 0)
	{
		error_message((Message)atoi(cmd_.substr(6).c_str()));
	}
	else if (cmd_.find("player_message=") == 0)
	{
		Message m = (Message)atoi(cmd_.substr(15).c_str());
		player_message(m);
	}
	else if (cmd_.find("ai_message=") == 0)
	{
		Message m = (Message)atoi(cmd_.substr(11).c_str());
		ai_message(m);
	}
	else if (cmd_.find("message=") == 0)
	{
		fl_message_font_ = FL_COURIER;
		fl_message_size_ = h() / 40;
		Message m = (Message)atoi(cmd_.substr(8).c_str());
		Fl::add_timeout(0., [](void *d_) { (static_cast<Deck *>(d_))->redraw();	}, this);
		if (m == YOU_WIN)
		{
			show_win_msg();
		}
		else if (m == YOU_LOST)
		{
			show_lost_msg();
		}
		else
		{
			DBG("fl_alert(" << Util::message(m) << ")\n");
			fl_message_position(x() + w() / 2, y() + h() / 2, 1);
			fl_alert("%s", Util::message(m).c_str());
		}
	}
	else if (cmd_.find("gb=") == 0)
	{
		std::string args = cmd_.substr(3);
		if (args.empty())
			_game.book.clear();
		else if(args.size() >= 3)
		{
			auto first = atoi(args.c_str());
			auto second = atoi(&args[2]);
			_game.book.push_back(std::make_pair(first, second));
		}
	}
	else if (cmd_.find("cip") == 0)
	{
		OUT(suite_symbol(HEART) << ": " << _engine.cards_in_play(HEART) << " (" << _engine.max_cards_player(HEART) << ")\n");
		OUT(suite_symbol(SPADE) << ": " << _engine.cards_in_play(SPADE) << " (" << _engine.max_cards_player(SPADE) << ")\n");
		OUT(suite_symbol(DIAMOND) << ": " << _engine.cards_in_play(DIAMOND) << " (" << _engine.max_cards_player(DIAMOND) << ")\n");
		OUT(suite_symbol(CLUB) << ": " << _engine.cards_in_play(CLUB) << " (" << _engine.max_cards_player(CLUB) << ")\n");
		OUT("max_trumps_player: " << _engine.max_trumps_player() << "\n");
		OUT("PL-deck: " << _player.deck << "\n");
		OUT("AI-deck: " << _ai.deck << "\n");
		OUT("Cards  : " << _game.cards << "\n");
	}
	else if (cmd_ == "help")
	{
		OUT("animate|back|debug|error|loglevel|message|ai_message|player_message|gb|cip|quit\n");
	}
	else if (cmd_ == "back")
	{
		WNG("history size: " << _history.size());
		if (back_history()) bell();
		return;
	}
	else if (cmd_ == "quit")
	{
		toggle_cmd_input();
	}
	else
	{
		bell();
	}
	redraw();
}
