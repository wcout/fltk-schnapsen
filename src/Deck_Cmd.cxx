//
// This file is part of Deck.cxx, and is only there
// to aid debugging during development without
// cluttering Deck.cxx.
//
#include "debug.h"
#include "Util.h"
#include "Alert.h"
#include "messages.h"
#include "GameBook.h"
#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <string>
#include <cstdlib>

// helper
static Suites suites_from_string(std::string s_)
{
	size_t pos;
	Suites suites;
	if (s_.size() && s_[0] == '|')
		s_.erase(0, 1);
	while ((pos = s_.find('|')) != std::string::npos)
	{
		std::string suite_sym = s_.substr(0, pos);
		s_.erase(0, pos + 1);
		for (auto &[suite, sym] : ::suite_symbols)
		{
			if (sym == suite_sym)
			{
				suites.push_back(suite);
				break;
			}
		}
	}
	return suites;
}

bool Deck::load_game(const std::string &name_)
{
	std::string name(name_);
	if (name.empty() || std::filesystem::exists(name) == false)
	{
		WNG("Not existing game file '" << name << "'!");
		bell();
		return false;
	}
	std::ifstream ifs(name.c_str(), std::ios::binary);
	if (!ifs.is_open() || ifs.bad())
	{
		WNG("Bad game file '" << name << "'!");
		bell();
		return false;
	}
	std::string line;
	_player.move_state = NONE;
	_ai.move_state = NONE;
	_player.s20_40.clear();
	_ai.s20_40.clear();
	bool bad_file(false);
	while (getline(ifs, line))
	{
    	line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());
    	if (line.empty()) continue; // all whitespace
		else if (line.size() && (line[0] == '#' || line[0] == '/')) continue; // comment
		else if (line.find("player_cards:") == 0) _player.cards = line.substr(13);
		else if (line.find("player_20_40:") == 0) _player.s20_40 = suites_from_string(line.substr(13));
		else if (line.find("player_card:") == 0)
		{
			_player.card = Cards("|" + line.substr(12) + "|")[0];
			_player.move_state = ON_TABLE;
		}
		else if (line.find("player_deck:") == 0 ) _player.deck = line.substr(12);
		else if (line.find("player_score:") == 0 ) _player.score = atoi(line.substr(13).c_str());
		else if (line.find("player_pending:") == 0) _player.pending = atoi(line.substr(15).c_str());
		else if (line.find("ai_cards:") == 0) _ai.cards = line.substr(9);
		else if (line.find("ai_20_40:") == 0) _ai.s20_40 = suites_from_string(line.substr(9));
		else if (line.find("ai_card:") == 0)
		{
			_ai.card = Cards("|" + line.substr(12) + "|")[0];
			_ai.move_state = ON_TABLE;
		}
		else if (line.find("ai_deck:") == 0) _ai.deck = line.substr(8);
		else if (line.find("ai_score:") == 0) _ai.score = atoi(line.substr(9).c_str());
		else if (line.find("ai_pending:") == 0) _ai.pending = atoi(line.substr(11).c_str());
		else if (line.find("cards:") == 0) _game.cards = line.substr(6);
		else if (line.find("closed:") == 0) _game.closed = static_cast<Closed>(atoi(line.substr(7).c_str()));
		else if (line.find("ai_score_closed:") == 0) _ai.score_closed = atoi(line.substr(16).c_str());
		else if (line.find("player_score_closed:") == 0) _player.score_closed = atoi(line.substr(20).c_str());
		else if (line.find("trump:") == 0) _game.trump = Cards("|A" + line.substr(6) + "|")[0].suite();
		else if (line.find("move:") == 0) _game.move = static_cast<Player>(atoi(line.substr(5).c_str()));
		else { bad_file = true; break; }
	}
	// Check cards for completeness
	Cards c = _player.cards + _player.deck + _ai.cards + _ai.deck + _game.cards;
	if (_player.move_state == ON_TABLE)
		c += _player.card;
	if (_ai.move_state == ON_TABLE)
		c += _ai.card;
	if (bad_file ||
	    c.size() != 20 || c.check() == false ||
	    (_player.move_state == ON_TABLE && _game.move == PLAYER) ||
	    (_ai.move_state == ON_TABLE && _game.move == AI))
	{
		WNG("Corrupt game file '" << name << "'!");
		bell();
		_game.cards = Cards::fullcards();
		_player.cards.clear();
		_player.deck.clear();
		_ai.cards.clear();
		_ai.deck.clear();
		init();
		return false;
	}
	else
	{
		LOG("Loaded game file '" << name << "' - next to move: " << (_game.move == PLAYER ? "Player": "AI") << "\n");
		_redeal = false;
		prepare_game();
	}
	return true;
}

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
		OUT("loglevel: " << Util::config_as_int("loglevel") << "\n");
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
		fl_message_font_ = FL_COURIER_BOLD;
		fl_message_size_ = h() / 40;
		Message m = (Message)atoi(cmd_.substr(8).c_str());
		Fl::add_timeout(0., [](void *d_) { (static_cast<Deck *>(d_))->redraw();	}, this);
		if (m == YOU_WIN)
		{
			LOG("command win_msg\n");
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
		OUT(Card::suite_symbol(HEART) << ": " << _engine.cards_in_play(HEART) << " (" << _engine.max_cards_player(HEART) << ")\n");
		OUT(Card::suite_symbol(SPADE) << ": " << _engine.cards_in_play(SPADE) << " (" << _engine.max_cards_player(SPADE) << ")\n");
		OUT(Card::suite_symbol(DIAMOND) << ": " << _engine.cards_in_play(DIAMOND) << " (" << _engine.max_cards_player(DIAMOND) << ")\n");
		OUT(Card::suite_symbol(CLUB) << ": " << _engine.cards_in_play(CLUB) << " (" << _engine.max_cards_player(CLUB) << ")\n");
		OUT("max_trumps_player: " << _engine.max_trumps_player() << "\n");
		OUT("PL-deck: " << _player.deck << "\n");
		OUT("AI-deck: " << _ai.deck << "\n");
		OUT("Cards  : " << _game.cards << "\n");
	}
	else if (cmd_ == "help")
	{
		OUT("animate|back|debug|error|load|save|loglevel|message|ai_message|player_message|gb|cip|quit\n");
	}
	else if (cmd_ == "back")
	{
		WNG("history size: " << _history.size());
		if (back_history()) bell();
		return;
	}
	else if (cmd_.find("save") == 0)
	{
		std::string arg = cmd_.substr(4);
		if (arg.size() && (arg[0] == ' ' || arg[0] == '='))
			arg.erase(0, 1);
		std::string name(arg.size() ? arg.c_str() : "game.scg");
		std::ofstream ofs(name.c_str(), std::ios::binary);
		ofs << "player_cards:" << _player.cards << "\n";
		if (_player.move_state == ON_TABLE)
			ofs << "player_card:" << _player.card << "\n";
		if (_player.s20_40.size())
		{
			ofs << "player_20_40:|";
			for (auto &s : _player.s20_40)
				ofs << Card::suite_symbol(s) << "|";
			ofs << "\n";
		}
		if (_ai.move_state == ON_TABLE)
			ofs << "ai_card:" << _ai.card << "\n";
		ofs << "player_deck:" << _player.deck << "\n";
		ofs << "player_score:" << _player.score << "\n";
		ofs << "player_pending:" << _player.pending << "\n";
		ofs << "ai_cards:" << _ai.cards << "\n";
		if (_ai.s20_40.size())
		{
			ofs << "ai_20_40:|";
			for (auto &s : _ai.s20_40)
				ofs << Card::suite_symbol(s) << "|";
			ofs << "\n";
		}
		if (_ai.move_state == ON_TABLE)
			ofs << "ai_card:" << _ai.card << "\n";
		if (_ai.move_state == ON_TABLE)
			ofs << "ai_card:" << _ai.card << "\n";
		ofs << "ai_deck:" << _ai.deck << "\n";
		ofs << "ai_score:" << _ai.score << "\n";
		ofs << "ai_pending:" << _ai.pending << "\n";
		ofs << "cards:" << _game.cards << "\n";
		ofs << "closed:" << (int)_game.closed << "\n";
		ofs << "ai_score_closed:" << _ai.score_closed << "\n";
		ofs << "player_score_closed:" << _player.score_closed << "\n";
		ofs << "trump:" << Card::suite_symbol(_game.trump) << "\n";
		LOG("Saved to game file: '" << name << "\n");
	}
	else if (cmd_.find("load") == 0)
	{
		std::string arg = cmd_.substr(4);
		if (arg.size() && (arg[0] == ' ' || arg[0] == '='))
			arg.erase(0, 1);
		if (load_game(arg))
		{
			if (_game.move == AI)
				_restart = true;
		}
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
