//
// Part of "Schnapsen for 2" card game.
//
// (c) 2026 Christian Grabner
//
// Manage the FLTK UI and the game.
//

#include "Deck.h"
#include "Engine.h"

#include "Util.h"
#include "Alert.h"
#include "FontLoader.h"
#include "AnimText.h"

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_SVG_Image.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_Tiled_Image.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/fl_draw.H>

#include <string>
#include <vector>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <functional>
#include <format>
#include <cmath>

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

class Deck;
typedef void (Deck::*DeckMemberFn)();

using enum Player;
using enum CardState;
using enum Message;
using enum Closed;
using enum Marriage;
using enum Result;

//
// UI dependent (drawing/event handling) stuff
//

class Cmd_Input : public Fl_Input
{
public:
	Cmd_Input(int x_, int y_, int w_, int h_) : Fl_Input(x_, y_, w_, h_)
	{
		box(FL_BORDER_BOX);
		textfont(FL_HELVETICA);
		textsize(h() / 3 * 2);
	}
	void draw()
	{
		damage(FL_DAMAGE_ALL);
		Fl_Input::draw();
	}
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
		box(FL_FLAT_BOX);
		down_box(FL_BORDER_BOX);
		labelfont(FL_HELVETICA_BOLD);
		labelsize(h() / 2);
		visible_focus(0);
	}
	int handle(int e_) override
	{
		if (e_ == FL_ENTER) { value(1); redraw(); }
		if (e_ == FL_LEAVE) { value(0); redraw(); }
		return Fl_Button::handle(e_);
	}
	void resize(int x_, int y_, int w_, int h_)
	{
		Fl_Button::resize(x_, y_, w_, h_);
		labelsize(h() / 2);
	}
};

#ifdef USE_MINIAUDIO
class Audio
{
public:
	Audio() : _volume(100)
	{
		// Initialize the engine
		if (ma_engine_init(NULL, &_engine) != MA_SUCCESS)
		{
			WNG("Failed to initialize audio engine.");
		}
	}
	void play(const std::string &filename_)
	{
		// Play the sound asynchronously.
		// This function returns immediately.
		ma_engine_play_sound(&_engine, filename_.c_str(), NULL);
	}
	void set_volume(int volume_)
	{
		// [0, 100] 0=mute, 100=max
		_volume = volume_;
		ma_engine_set_volume(&_engine, (double)volume_ / 100);
	}
	int volume() const { return _volume; }
	~Audio()
	{
		ma_engine_stop(&_engine);
	}
private:
	ma_engine _engine;
	int _volume;
};
#endif


class Deck : public Fl_Double_Window, public UI
{
private:
	struct CardAnimParams
	{
		CardAnimParams(DeckMemberFn func_ = nullptr) :
			func(func_), cards(1), steps(5), closing(0) {}
		DeckMemberFn func;
		size_t cards;
		int steps;
		int closing;
		int src_X;
		int src_Y;
		int dest_X;
		int dest_Y;
		int X;
		int Y;
	};
	struct GameState
	{
		GameState(PlayerData &player_, PlayerData &ai_, GameData &game_) :
			player(player_), ai(ai_), game(game_) {}
		PlayerData player;
		PlayerData ai;
		GameData   game;
	};
public:
	Deck() : Fl_Double_Window(800, 600),
		_engine(_game, _player, _ai, *this),
		_error_message(NO_MESSAGE),
		_disabled(false),
		_redeal(false),
		_CW(w() / 8),
		_CH(1.5 * w()),
		_cmd_input(nullptr),
		_redeal_button(nullptr),
		_winning_button(nullptr),
		_winning_claim(false),
		_welcome(nullptr),
		_selector(nullptr),
		_grayout(false),
		_strictness(Util::config_as_int("strict")),
		_animation_level(Util::config("animate").empty() ? 2 : Util::config_as_int("animate")),
		_show_ai_cards(false),
		_restart(false),
		_card_scale(1.0),
		_player_anim_text(nullptr),
		_ai_anim_text(nullptr)
	{
		// NOTE: FLTK (1.4) currently does not allow to update the internal font list after
		// initial the Fl::set_fonts(). Therefore all maybe used fonts must be loaded at once.
		FontLoader::load_dir(Util::rsc_dir().c_str());
		load_font();
		_game.trump_sort = Util::config_as_int("trump-sort");
		_player.games_won = Util::stats_as_int("player_games_won");
		_ai.games_won = Util::stats_as_int("ai_games_won");
		_player.matches_won = Util::stats_as_int("player_matches_won");
		_ai.matches_won = Util::stats_as_int("ai_matches_won");
		copy_label(Util::message(TITLE).c_str());
		fl_register_images();
		_shadow.image("card_shadow", Card::shadow_svg(), true);
		_outline.image("card_outline", Card::outline_svg(), true);
		_empty.image("card_empty", Card::empty_svg(), true);
		_game.cards = Cards::fullcards();
		assert(_game.cards.check());
		_card_template = _game.cards[0];
		_engine.unit_tests();
		default_cursor(FL_CURSOR_HAND);
		Fl_RGB_Image *icon = Card(QUEEN, HEART).image();
		icon->normalize();
		default_icon(icon);
		_redeal_button = new Button(w() - 100, h() - 40, 100, 40, Util::message(REDEAL).c_str());
		_redeal_button->color(FL_YELLOW);
		_redeal_button->selection_color(fl_darker(FL_YELLOW));
		_redeal_button->hide();
		_winning_button = new Button(w() / 2 - 100, h() - 16, 200, 16, Util::message(CLAIM).c_str());
		_winning_button->box(FL_RFLAT_BOX);
		_winning_button->down_box(FL_ROUNDED_BOX);
		_winning_button->color(0xffe4e100);
		_winning_button->selection_color(0xbc8f8f00);
		_winning_button->hide();
		resizable(this);
		size_range(400, 300/*, 0, 0, 0, 0, 1*/);
		int width = Util::config_as_int("width");
		int height = Util::config_as_int("height");
		if (width > 400 && height > 300)
			size(width, height);
		int xpos = Util::config_as_int("xpos");
		int ypos = Util::config_as_int("ypos");
		position(xpos, ypos);
#ifdef USE_MINIAUDIO
		std::string volume = Util::config("volume");
		if (volume.size())
			_audio.set_volume(std::atoi(volume.c_str()));
#endif
		std::string s = Util::config("card_scale");
		if (s.size())
		{
			double card_scale = atof(s.c_str());
			if (card_scale >= 1 && card_scale <= 1.5)
				_card_scale = card_scale;
		}
		_redeal_button->callback([](Fl_Widget *wgt_, void *)
		{
			static_cast<Deck *>(wgt_->window())->redeal();
		});
		_winning_button->callback([](Fl_Widget *wgt_, void *)
		{
			static_cast<Deck *>(wgt_->window())->winning_claim();
		});
		if (Util::config("fullscreen") == "1")
		{
			toggle_fullscreen();
		}
		LOG("strictness: " << _strictness << ", animation_level: " << _animation_level << "\n");
		_game.book.history(Util::stats("gamebook"));
		apply_selections();
	}

	void update() override
	{
		redraw();
	}

	bool test_change()
	{
		if (_game.cards.size() && _game.cards.back().face() != JACK &&
		    _player.card.face() == JACK && _player.card.suite() == _game.cards.back().suite())
		{
			if (_game.cards.size() < 4)
			{
				error_message(NO_CHANGE, true);
				return false;
			}
			// make change
			Card c = _game.cards.back();
			_player.move_state = NONE;
			_player.cards.push_back(_player.card);
			_engine.sort_cards(_player.cards);
			_engine.test_change(_player, true);
			_player.last_drawn = c;
			return true;
		}
		bell(NO_CHANGE);
		return false;
	}

	bool test_close(int x_, int y_)
	{
		// test if player wants close and is allowed to
		if (_game.closed == NOT && idle() &&
		    _game.cards.front().rect().includes(x_, y_))
		{
		   if (_game.cards.size() < 4)
			{
				error_message(NO_CLOSE, true);
			}
			else
			{
				// do close
				_engine.do_close(_player);
				return true;
			}
		}
		return false;
	}

	bool test_20_40(int x_, int y_)
	{
		_game.marriage = NO_MARRIAGE;
		if (_player.card.face() == QUEEN || _player.card.face() == KING)
		{
			for (auto &c : _player.cards)
			{
				if (c.suite() == _player.card.suite() &&
				   (c.face() == QUEEN || c.face() == KING) &&
					(c.includes(x_, y_)))
				{
					if (c.suite() == _game.trump)
					{
						_game.marriage = MARRIAGE_40;
					}
					else
					{
						_game.marriage = MARRIAGE_20;
					}
					return true;
				}
			}
		}
		return false;
	}

	bool can_trick(const Card &c_, const Cards &cards_) const
	{
		for (auto &c : cards_)
		{
			if (_engine.card_tricks(c, c_)) return true;
		}
		return false;
	}

	bool valid_move(const Card &card_)
	{
		// check closed game and AI has card on table
		if (_game.closed != NOT)
		{
			if (_ai.move_state == ON_TABLE)
			{
				// construct player cards from cards+played card
				Cards temp = _player.cards;
				temp.push_back(card_);
				if (_engine.has_suite(temp, _ai.card.suite()))
				{
					// if player cards has suite it must use
					if (card_.suite() != _ai.card.suite())
					{
						error_message(INVALID_SUITE, true);
						return false;
					}
					// if player can trick with suite, he must
					if (_engine.can_trick_with_suite(_ai.card, temp) && !_engine.card_tricks(card_, _ai.card))
					{
						error_message(MUST_TRICK_WITH_SUITE, true);
						return false;
					}
					// otherwise a lower card of suite is accepted
					return true;
				}
				// if player can trick (with trump now), he must
				if (can_trick(_ai.card, temp) && !_engine.card_tricks(card_, _ai.card))
				{
					error_message(MUST_TRICK_WITH_TRUMP, true);
					return false;
				}
			}
		}
		// player can't trick and has no suite color
		return true;
	}

	void handle_move()
	{
		bool player_deck_info = _player.deck_info;
		bool ai_deck_info = _ai.deck_info;
		if (_player.move_state != NONE && _ai.move_state == NONE)
		{
			if (_game.move == PLAYER)
			{
				test_20_40(Fl::event_x(), Fl::event_y());
			}
		}
		else if (_player.move_state == NONE)
		{
			_player.deck_info = _player.deck.size() &&
			                    _player.deck.front().rect().includes(Fl::event_x(), Fl::event_y());
			_ai.deck_info = _ai.deck.size() &&
			                _ai.deck.front().rect().includes(Fl::event_x(), Fl::event_y());
		}
		if (_game.marriage != NO_MARRIAGE || _player.move_state == MOVING ||
		    _player.deck_info || _ai.deck_info ||
		    (_player.deck_info != player_deck_info) ||
		    (_ai.deck_info != ai_deck_info))
		{
			redraw();
		}
	}

	void change_card_scale(bool up_)
	{
		if (up_)
		{
			_card_scale += 0.05;
			if (_card_scale > 1.5)
			{
				_card_scale = 1.5;
				bell();
			}
		}
		else
		{
			_card_scale -= 0.05;
			if (_card_scale < 1)
			{
				_card_scale = 1;
				bell();
			}
		}
		redraw();
	}

	bool handle_key()
	{
		if (Fl::event_key('q') && !_disabled && ::debug) // just for testing -> redeal
		{
			redeal();
		}
		else if (Fl::event_key('d')) // just for testing -> debug
		{
			debug(true);
		}
		else if (Fl::event_key('a'))
		{
			_animation_level++;
			_animation_level %= 3;
			error_message(ANIMATION, true);
		}
#ifdef USE_MINIAUDIO
		else if (Fl::event_key('v'))
		{
			int volume = _audio.volume();
			if (Fl::event_ctrl())
				volume -= 10;
			else
				volume += 10;
			if (volume > 100)
				volume = 100;
			if (volume < 0)
				volume = 0;
			_audio.set_volume(volume);
			error_message(VOLUME);
			bell(NO_MESSAGE, false);
		}
#endif
		else if (Fl::event_key('t'))
		{
			_game.trump_sort = !_game.trump_sort;
			DBG("handle_key 't': trump_sort is now " << _game.trump_sort << "\n");
			error_message(TRUMP_SORT, true);
			_engine.sort_cards(_player.cards);
			_engine.sort_cards(_ai.cards);
		}
		else if (Fl::event_key('+') || Fl::event_dy() > 10)
		{
			change_card_scale(true);
		}
		else if (Fl::event_key('-') || Fl::event_dy() < -10)
		{
			change_card_scale(false);
		}
		else if (Fl::event_key('0'))
		{
			_card_scale = 1;
			redraw();
		}
		else if (Fl::event_key(FL_F + 1) && idle())
		{
			delayed_call(&Deck::welcome);
		}
		else if (Fl::event_key(FL_F + 8) && idle())
		{
			delayed_call(&Deck::selector);
		}
		else if (Fl::event_key(FL_F + 12) && ::debug) // just for testing -> cmd
		{
			toggle_cmd_input();
		}
		else if (Fl::event_key(FL_F + 10)) // toggle fullscreen
		{
			toggle_fullscreen();
		}
		else
		{
			return false;
		}
		return true;
	}

	void select_background()
	{
		if (Fl::event_alt())
		{
			// clear background
			Util::config("background", "NONE");
		}
		else
		{
			// select background image
			_grayout = true;
			const char *bg_tile = Util::config("background").c_str();
			bg_tile = fl_file_chooser(Util::message(DECK_BG).c_str(), "*.{png,gif,jpg,svg}",
			                          (std::string(bg_tile) != "NONE" ? bg_tile : Util::rsc_dir().c_str()));
			_grayout = false;
			if (bg_tile)
			{
				Util::config("background", bg_tile);
			}
		}
		redraw();
	}

	void load_font()
	{
		std::string custom_font = Util::config("font");
		if (custom_font.size())
		{
			static std::string fontName;
			fontName = FontLoader::convertToFontName(Util::filename(custom_font));
			std::string dir = Util::dirname(custom_font);
			std::string font_path = dir.size() ? custom_font : Util::rsc_dir() + custom_font;
			CustomFont = FontLoader::load(font_path.c_str(), fontName.c_str());
			DBG("CustomFont: #" << CustomFont << "\n");
			redraw();
		}
	}

	void select_font()
	{
		if (Fl::event_alt())
		{
			// clear font
			Util::config("font", "");
			CustomFont = FL_HELVETICA;
			redraw();
		}
		else
		{
			// select font file
			_grayout = true;
			const char *font = Util::config("font").c_str();
			std::string dir = Util::dirname(font);
			std::string font_path = dir.size() ? font : Util::rsc_dir() + font;
			font = fl_file_chooser(Util::message(FONT_SEL).c_str(), "*.{ttf}",
			                       font_path.c_str());
			_grayout = false;
			if (font)
			{
				Util::config("font", font);
				load_font();
			}
		}
		redraw();
	}

	void handle_click(int x_, int y_)
	{
		if (_disabled)
		{
			// terminate wait()
			_disabled = false;
			return;
		}

		_game.marriage = NO_MARRIAGE;
		if (gamebook_rect().includes(x_, y_) && idle())
		{
			_game.book.next_current();
			return;
		}
		if (cards_rect(AI).includes(x_, y_))
		{
			_show_ai_cards = !_show_ai_cards;
			return;
		}
		if (test_close(x_, y_) == true)
		{
			return;
		}
		_game.book.reset_current();
		if (_player.move_state != NONE)
		{
			if (_player.move_state == MOVING &&
				(Fl::event_button() > 1 || !valid_move(_player.card)))
			{
					// withdraw move, put card back to hand
				LOG("withdraw or invalid " << _player.card << "\n");
				_player.cards.push_back(_player.card);
				_player.move_state = NONE;
				_engine.sort_cards(_player.cards);
				return;
			}
			if (_ai.move_state == NONE)
			{
				test_20_40(x_, y_);
			}

			if (_game.closed == NOT && _game.cards.size() && _ai.move_state == NONE &&
			    _game.cards.back().rect().includes(x_, y_))
			{
				test_change();
				return;
			}
			// beware of unwanted (unsucessfull) click on change
			if (x_ < w() / 2)
				return;

			// make accepted move
			if (_player.move_state == MOVING)
			{
				if (_game.marriage == MARRIAGE_20)
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
				if (_game.marriage == MARRIAGE_40)
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
				bell(PLACE_CARD, false);
				_player.move_state = ON_TABLE; // _player.card is on table
				return;
			}
		}
		assert(_player.cards.size());
		for (size_t i = 0; i < _player.cards.size(); i++)
		{
			const Card &c = _player.cards[i];
			if (c.rect().includes(x_, y_))
			{
				_player.card = _player.cards[i];
				_player.cards.erase(_player.cards.begin() + i);
				_player.move_state = MOVING;
				LOG("PL move: " << _player.card << "\n");
				return;
			}
		}
		if (idle()/* && Fl::event_button() == FL_RIGHT_MOUSE*/)
		{
			if (cards_area_rect().includes(x_, y_))
			{
				select_background();
			}
			if (message_rect(PLAYER).includes(x_, y_))
			{
				select_font();
			}
		}
	}

	int handle(int e_) override
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
			if (handle_key())
			{
				check_sleep(true);
				return 1; // important to stop ESC from Welcome to also close main window
			}
		}
		else if (e_ == FL_PUSH)
		{
			error_message(NO_MESSAGE);
			Fl::remove_timeout(cb_sleep, this);
			Fl::add_timeout(20., cb_sleep, this);
			ai_message(NO_MESSAGE);
			_game.move == AI ? _ai.last_drawn = Card() : _player.last_drawn = Card();
			handle_click(Fl::event_x(), Fl::event_y());
			return 1;
		}
		else if (e_ == FL_MOUSEWHEEL)
		{
			change_card_scale(Fl::event_dy() > 0);
		}
		return ret;
	}

	Rect gamebook_rect() const
	{
		return Rect(w() / 40, h() / 2 - _CH / 2, _CW, _CH);
	}

	Rect cards_rect(Player player_) const
	{
		return Rect(w() / 20 + w() / 2 - w() / 24,
			(player_ == AI ? h() / 40 : h() - _CH - h() / 40),
			4 * w() / 20 + _CW,
			(player_ == AI ? _CH / 3 : _CH));
	}

	Rect cards_area_rect() const
	{
		Rect p{ on_table_rect(PLAYER) };
		Rect a{ on_table_rect(AI) };
		return Rect(p.x, a.y, a.x + a.w - p.x, p.y + p.h - a.y);
	}

	Rect change_rect() const
	{
		return Rect(w() / 3 - _CW + _CW / 4,
			(h() - _CW) / 2,
			_CH,
			_CW
			);
	};

	Rect closed_rect(Player player_)
	{
		Fl_Font f = fl_font();
		Fl_Fontsize sz = fl_size();
		fl_font(CustomFont, _CH / 7);
		int W = fl_height();
		int X = pack_rect().center().x - W / 2;
		int Y = (player_ == AI ? h() / 16 : h() - h() / 16) - W + fl_descent();
		Rect r = Rect(X, Y, W, W);
		fl_font(f, sz);
		return r;
	}

	Rect deck_rect(Player player_) const
	{
		return Rect(w() - _CW - 2, (player_ == AI ?
			h() / 10 - w() / 800 : h() - _CH - h() / 10) , _CW, _CH);
	}

	Rect message_rect(Player player_) const
	{
		Fl_Font f = fl_font();
		Fl_Fontsize sz = fl_size();
		fl_font(CustomFont, w() / 24);
		Rect r = Rect(0, (player_ == AI ? h() / 8 - fl_height() + fl_descent() : h() - h() / 8 - fl_height() + fl_descent()), w() / 2, fl_height());
		fl_font(f, sz);
		return r;
	}

	Rect move_rect(Player player_) const
	{
		int ma = h() / 40 + _CH / 3 + h() / 40 + _CH / 2;
		int mp = h() - h() / 40 - _CH - h() / 40 - _CH / 2;
		int m = (ma + mp) / 2;
		int mx = cards_rect(player_).center().x;
		int dx = _CW / 8;
		return Rect(
			(player_ == AI ? mx + dx : _player.move_state == MOVING ? Fl::event_x() - _CW / 2 : mx - dx - _CW),
			(player_ == AI ? m - _CH / 2 - _CH / 8 : _player.move_state == MOVING ? Fl::event_y() - _CH / 2 : m - _CH / 2 + _CH / 8),
			_CW,
			_CH
			);
	}

	Rect on_table_rect(Player player_) const
	{
		int ma = h() / 40 + _CH / 3 + h() / 40 + _CH / 2;
		int mp = h() - h() / 40 - _CH - h() / 40 - _CH / 2;
		int m = (ma + mp) / 2;
		int mx = cards_rect(player_).center().x;
		int dx = _CW / 8;
		return Rect(
			(player_ == AI ? mx + dx : mx - dx - _CW),
			(player_ == AI ? m - _CH / 2 - _CH / 8 : m - _CH / 2 + _CH / 8),
			_CW,
			_CH
			);
	}

	Rect pack_rect() const
	{
		return Rect(w() / 4 - _CW / 2, (h() - _CH) / 2, _CW, _CH);
	}

	void draw_gamebook()
	{
		_game.book.draw(gamebook_rect());
	}

	void draw_deck_info(int x_, int y_, const Cards &deck_, int max_tricks_ = 8)
	{
		auto put_card = [&](const Card &c_, std::ostringstream &os_)
		{
			if (c_.is_red_suite())
				os_ << "^r";
			else
				os_ << "^B";
			os_ << face_abbrs[c_.face()];
			std::string symbol_image = Util::cardset_dir() + Card::suite_symbol_image(c_.suite());
			if (std::filesystem::exists(symbol_image + ".svg"))
			{
				os_ << "^|" << symbol_image << "|";
			}
			else
			{
				os_ << c_.suite_symbol();
			}
		};

		fl_color(fl_lighter(fl_lighter(FL_YELLOW)));
		Rect r(Rect(_CW, _CH).inset(_CW / 10));
		fl_rectf(x_, y_, r.w, r.h);
		fl_color(GRAY);
		fl_rect(x_, y_, r.w, r.h);
		fl_color(FL_BLACK);
		fl_font(FL_COURIER, _CW / 7);

		for (size_t i = 0; i < deck_.size(); i += 2)
		{
			max_tricks_--;
			if (max_tricks_ < 0) break;
			std::ostringstream os;
			os << " ";
			put_card(deck_[i], os);
			os << "^G|";
			put_card(deck_[i + 1], os);
			std::string s = os.str();
			Util::draw_string(s, x_ + _CW / 10, y_ + _CW / 5 + (i / 2) * (fl_height() - fl_descent()));
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

		Rect r(x_, y_, 1, 1);
		if (_player.move_state == ON_TABLE)
		{
			r = _player.card.rect().center();
		}
		else if (_ai.move_state == ON_TABLE)
		{
			r = _ai.card.rect().center();
		}
		int D = h() / 10;
		fl_color(c_);
		fl_pie(r.x - D / 2, r.y - D / 2, D, D, 0., 360.);
		fl_color(FL_WHITE);
		fl_font(FL_HELVETICA_BOLD, D / 2);
		Util::draw_string(text_, r.x - Util::string_width(text_) / 2, r.y + fl_height() / 2 - fl_descent(), true);
	}

	void draw_suite_symbol(CardSuite suite_, int x_, int y_)
	{
		fl_font(FL_COURIER, _CH / 6);
		fl_color(FL_BLACK);
		Card c(ACE, suite_);
		std::ostringstream os;
		if (c.is_red_suite())
			os << "^r";
		else
			os << "^B";

		std::string symbol_image = Util::cardset_dir() + Card::suite_symbol_image(c.suite());
		if (std::filesystem::exists(symbol_image + ".svg"))
		{
			os << "^|" << symbol_image << "|";
		}
		else
		{
			os << c.suite_symbol();
		}
		Util::draw_string(os.str(), x_ - Util::string_width(os.str()) / 2, y_);
	}

	std::string background_image()
	{
		std::string def_image(Util::rsc_dir() + "deck.gif");
		std::string image = Util::config("background");
		if (image == "NONE") return "";
		if (image == "" || image == "default") return def_image;
		return image;
	}

	Fl_Color background_color()
	{
		Fl_Color def_color(FL_CYAN);
		std::string color = Util::config("background");
		if (color.empty() || !isdigit(color[0]))
			return def_color;
		// return color at index in FLTK palette
		return Fl_Color(atoi(color.c_str()));
	}

	void draw_table()
	{
		fl_rectf(0, 0, w(), h(), background_color());
		Fl_Image *bg = Util::get_shared_image(background_image());
		if (bg && bg->w() && bg->h())
		{
			Fl_Tiled_Image tbg(bg, w(), h());
			tbg.draw(0, 0, w(), h());
		}
	}

	std::string format_message(Message msg_)
	{
		std::string m = Util::message(msg_);
		if (m.find("{") != std::string::npos)
		{
			switch (msg_)
			{
				case ANIMATION:
				{
					std::string a("^g" + std::to_string(_animation_level));
					m = std::vformat(m, std::make_format_args(a));
					break;
				}
				case TRUMP_SORT:
				{
					static const std::string ON("^|2705|");
					static const std::string OFF("^|274c|");
					m = std::vformat(m, std::make_format_args((_game.trump_sort ? ON : OFF)));
					break;
				}
#ifdef USE_MINIAUDIO
				case VOLUME:
				{
					int v = _audio.volume();
					m = std::vformat(m, std::make_format_args(v));
					break;
				}
#endif
				default:
					break;
			}
		}
		return m;
	}

	static void cb_clear_error(void *d_)
	{
		Deck *deck = static_cast<Deck *>(d_);
		deck->error_message(NO_MESSAGE);
		deck->redraw();
	}

	void draw_messages()
	{
		if (_player.message != NO_MESSAGE)
		{
			std::string player_message = Util::message(_player.message);
			fl_font(CustomFont, w() / (player_message.back() == '!' ? 24 : 34));
			fl_color(FL_RED);
			if (_player_anim_text)
				player_message =_player_anim_text->text();
			Util::draw_string(player_message, message_rect(PLAYER).center().x - Util::string_width(player_message) / 2, message_rect(PLAYER).baseline(), true);
		}
		if (_ai.message != NO_MESSAGE)
		{
			std::string ai_message = Util::message(_ai.message);
			fl_font(CustomFont, w() / (ai_message.back() == '!' ? 24 : 34));
			size_t pos = ai_message.find("!!");
			if (pos != std::string::npos)
				ai_message.erase(pos, 2);
			fl_color(FL_RED);
			if (_ai_anim_text)
				ai_message = _ai_anim_text->text();
			Util::draw_string(ai_message, message_rect(AI).center().x - Util::string_width(ai_message) / 2, message_rect(AI).baseline(), true);
		}
		if (_error_message != NO_MESSAGE)
		{
			std::string error_message = format_message(_error_message);
			fl_color(FL_RED);
			fl_rectf(0, h() - h() / 40, w(), h() / 40);
			fl_font(FL_HELVETICA_BOLD, h() / 50);
			fl_color(FL_WHITE);
			Util::draw_string(error_message, w() / 2 - Util::string_width(error_message) / 2, h() - fl_descent());
			if (!Fl::has_timeout(cb_clear_error, this))
				Fl::add_timeout(3.0, cb_clear_error, this);
		}
		else Fl::remove_timeout(cb_clear_error, this);
		if ((_game.closed == BY_PLAYER || _game.closed == BY_AI) && _ai.display_score == false)
		{
			fl_font(CustomFont, _CH / 7);
			fl_color(GRAY);
			std::string closed_sym = Util::message(CLOSED_MARKER);
			int X = closed_rect(_game.closed == BY_PLAYER ? PLAYER : AI).center().x;
			int Y = closed_rect(_game.closed == BY_PLAYER ? PLAYER : AI).baseline();
			Util::draw_string(closed_sym, X - Util::string_width(closed_sym) / 2 , Y);
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

		fl_font(FL_HELVETICA, _CH / 7);
		for (size_t i = 0; i < _player.s20_40.size(); i++)
		{
			draw_suite_symbol(_player.s20_40[i], w() - w() / 30 * (4 - i) - fl_descent(), h() - fl_descent());
		}
		for (size_t i = 0; i < _ai.s20_40.size(); i++)
		{
			draw_suite_symbol(_ai.s20_40[i], w() - w() / 30 * (4 - i) - fl_descent(), fl_height() - fl_descent());
		}
	}

	void draw_cards()
	{
		for (size_t i = 0; i < _ai.cards.size(); i++)
		{
			int X = cards_rect(AI).x + i * w() / 20;
			int Y = cards_rect(AI).y;
			if ((::debug > 1 && _show_ai_cards) || (::debug == 0 && _ai.display_score))
			{
				_ai.cards[i].skewed_image()->draw(X, Y);
			}
			else
			{
				_back.skewed_image()->draw(X, Y);
			}
		}
		for (size_t i = 0; i < _player.cards.size(); i++)
		{
			Card &c = _player.cards[i];
			Fl_RGB_Image *image = c.image();
			int X = cards_rect(PLAYER).x + i * w() / 20;
			int Y = cards_rect(PLAYER).y;
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

	void delayed_call(DeckMemberFn func_)
	{
		static DeckMemberFn func;
		func = func_;
		Fl::add_timeout(0.0, [](void *d_)
		{
			Deck *deck = static_cast<Deck *>(d_);
			std::invoke(func, deck);
		}, this);
	}

	void do_animate(bool delete_ = true)
	{
		assert(_anim_params.func != nullptr);
		int dx = _anim_params.dest_X - _anim_params.src_X;
		int dy = _anim_params.dest_Y - _anim_params.src_Y;

		for (int i = 0; i < _anim_params.steps; i++)
		{
			_anim_params.X = _anim_params.src_X + (floor)(((double)dx / _anim_params.steps) * i);
			_anim_params.Y = _anim_params.src_Y + (floor)(((double)dy / _anim_params.steps) * i);
			wait(1./50);
			redraw();
		}
		if (delete_)
			_anim_params = {};
	}

	void animate_move() override
	{
		if (_animation_level == 0) return;

		_anim_params = { &Deck::draw_animated_move };
		_anim_params.src_X = cards_rect(_game.move).x + _CW / 2;
		_anim_params.src_Y = cards_rect(_game.move).center().y;

		_anim_params.dest_X = move_rect(_game.move).center().x;
		_anim_params.dest_Y = move_rect(_game.move).center().y;

		do_animate();
	}

	void animate_deal(Player player_, size_t cards_ = 1) override
	{
		if (_animation_level < 2) return;

		_anim_params = { &Deck::draw_animated_trick };
		_anim_params.src_X = pack_rect().center().x;
		_anim_params.src_Y = pack_rect().center().y;

		_anim_params.dest_X = cards_rect(player_).center().x;
		_anim_params.dest_Y = cards_rect(player_).center().y;

		_anim_params.cards = cards_;
		if (cards_ > 1)
			_anim_params.steps = 10;

		do_animate();
	}

	void animate_fillup(Player player_)
	{
		animate_deal(player_);
	}

	void animate_shuffle() override
	{
		if (_animation_level < 2) return;

		Cards save = _game.cards;
		_game.cards.clear();

		_anim_params = { &Deck::draw_animated_trick };

		_anim_params.src_X = pack_rect().x;
		_anim_params.src_Y = pack_rect().y;

		_anim_params.dest_X = pack_rect().center().x;
		_anim_params.dest_Y = pack_rect().center().y;

		static constexpr int step = 3;

		for (size_t i = 0; i < save.size(); i += step)
		{
			for (int c = 0; c < step; c++)
			{
				if (i + c < save.size())
					_game.cards.push_back(save[i + c]);
			}
			do_animate(false);
		}
		assert(_game.cards == save);
		_game.cards = save;
		_anim_params = {};
	}

	void animate_trick() override
	{
		if (_animation_level == 0) return;

		_anim_params = { &Deck::draw_animated_trick };
		_anim_params.src_X = move_rect(_game.move).center().x;
		_anim_params.src_Y = move_rect(_game.move).center().y;

		_anim_params.dest_X = deck_rect(_game.move).center().x;
		_anim_params.dest_Y = deck_rect(_game.move).center().y;

//		_anim_params.cards = 2; // would be correct, but not really visible

		do_animate();
	}

	void animate_change(bool from_hand_ = false) override
	{
		if (_animation_level == 0) return;

		_anim_params = { &Deck::draw_animated_change };
		_anim_params.src_X = change_rect().center().x;
		_anim_params.src_Y = change_rect().center().y;

		_anim_params.dest_X = cards_rect(_game.move).center().x;
		_anim_params.dest_Y = cards_rect(_game.move).center().y;

		if (from_hand_)
		{
			std::swap(_anim_params.src_X, _anim_params.dest_X);
			std::swap(_anim_params.src_Y, _anim_params.dest_Y);
		}
		_anim_params.steps = 10;
		do_animate();
	}

	void animate_close() override
	{
		if (_animation_level == 0) return;

		_anim_params = { &Deck::draw_closing };

		for (_anim_params.closing = 1; _anim_params.closing <= 4; _anim_params.closing++)
		{
			redraw();
			wait(1./30);
		}
		_anim_params = {};
	}

	void draw_pack()
	{
		auto card_stack_pos = [&](size_t i) -> double
		{
			// Calculate the space between cards on pack for card `i`.
			//
			// NOTE: Card width / 200 is a 'realistic' value for the
			//       height of the full card pack with 20 cards.
			//       But the pack with 10 cards after dealing just looks
			//       better, when single cards are visible.
			//       So making a compromise...
			double h = (double)_CW /
				((_game.cards.size() == 20 ||
			    (_player.cards.empty() && _game.closed == NOT)) ? 200 : 100);
			if (h > 1.) h = (int)h;
			return (double)i * h + .5;
		};

		// _game.cards.back() is the trump card
		if (_game.cards.size())
		{
			int X = change_rect().x;
			int Y = change_rect().y;
			if (_game.closed == NOT && _game.cards.size() != 20 && _player.cards.size() > 3 && !_anim_params.closing)
			{
				_game.cards.back().rot90_image()->draw(X, Y);
				_game.cards.back().rect(Rect(X, Y, _game.cards.back().image()->h(), _game.cards.back().image()->w()));
			}

			// pack position
			X = pack_rect().x;
			Y = pack_rect().y;
			if (_game.cards.size())
			{
				for (size_t i = 0; i < _game.cards.size() - 1; i++)
				{
					double d = card_stack_pos(i);
					double x = (double)X - d;
					double y = (double)Y - d;
					bool speedup = !_back.image()->as_svg_image();
					// Speedup shuffle/deal animation under Cairo:
					// Cairo cached images (patterns) are slow, when the image is very detailed,
					// as is the case with some card backs. So draw only the top card with the
					// card back, the lower cards with a much faster "empty" image.
					if (i + 1 == _game.cards.size() - 1 || speedup == false/* || _animate_func == nullptr*/)
						_back.image()->draw(floor(x), floor(y));
					else
						_empty.image()->draw(floor(x), floor(y));
				}
				if (_game.closed == NOT)
				{
					_game.cards.front().rect(Rect(X, Y, _game.cards.back().image()->w(), _game.cards.back().image()->h()));
				}
			}
		}
		else
		{
			// draw an outline of pack
			int X = pack_rect().x;
			int Y = pack_rect().y;
			_outline.image()->draw(X, Y);
		}

		if (_game.closed != NOT && _game.cards.size() && !_anim_params.closing)
		{
			int SW = card_stack_pos(_game.cards.size() - 1); // TEST: shadow size depends on stack size
			int X = pack_rect().x;
			int Y = pack_rect().center().y - _CW / 2 - SW / 2;

			// Use clipping for the card shadow to go over side of pack
			Rect r(pack_rect());
			fl_push_clip(r.x + r.w - SW, r.y, r.w * 2, r.h);
			_shadow.rot90_image()->draw(X + SW, Y + SW);
			fl_pop_clip();

			_back.rot90_image()->draw(X, Y);
		}
	}

	void draw_decks()
	{
		// show played pack
		auto draw_deck_cards = [&](size_t size_, int x_, int y_)
		{
			bool speedup = !_back.image()->as_svg_image();
			for (size_t i = 0; i < size_; i++)
			{
				if (i + 1 == size_ || speedup == false/* || _animate_func == nullptr*/)
					_back.image()->draw(x_ - i * w() / 800, y_ - i * w() / 800);
				else
					_empty.image()->draw(x_ - i * w() / 800, y_ - i * w() / 800);
			}
		};

		int X = deck_rect(PLAYER).x;
		int Y = deck_rect(PLAYER).y;
		draw_deck_cards(_player.deck.size(), X, Y);

		X = deck_rect(AI).x;
		Y = deck_rect(AI).y;
		draw_deck_cards(_ai.deck.size(), X, Y);

		if (_player.deck.size())
		{
			// click region for deck display ("tooltip")
			_player.deck.front().rect(deck_rect(PLAYER));
		}
		if (_ai.deck.size())
		{
			// click region for deck display ("tooltip")
			_ai.deck.front().rect(deck_rect(AI));
		}
	}

	void draw_move()
	{
		// cards moving or on table
		if (_ai.move_state == ON_TABLE)
		{
			int X =  on_table_rect(AI).x;
			int Y =  on_table_rect(AI).y;
			_ai.card.image()->draw(X, Y);
			_ai.card.rect(on_table_rect(AI));
		}
		if (_player.move_state != NONE)
		{
			int X = move_rect(PLAYER).x;
			int Y = move_rect(PLAYER).y;
			if (_player.move_state == MOVING)
				_shadow.image()->draw(X + _CW / 12, Y + _CW / 12);
			_player.card.image()->draw(X, Y);
			_player.card.rect(move_rect(PLAYER));
		}
		// marriage declarations
		if (_game.marriage == MARRIAGE_20) draw_blob("20", FL_GREEN, Fl::event_x(), Fl::event_y());
		if (_game.marriage == MARRIAGE_40) draw_blob("40", FL_RED, Fl::event_x(), Fl::event_y());
	}

	void draw_scores()
	{
		if (_player.score)
		{
			bool show_closed_score = _game.closed == BY_AI && _strictness > 0;
			fl_font(CustomFont, w() / 42);
			fl_color(show_closed_score ? FL_RED : FL_BLUE);
			std::ostringstream os;
			os << (show_closed_score ? _player.score_closed : _player.score);
			Util::draw_string(os.str(), w() - Util::string_width(os.str()), h() - fl_descent());
		}
		if (_ai.score && (_ai.display_score | ::debug))
		{
			bool show_closed_score = _game.closed == BY_PLAYER && _strictness > 0;
			fl_font(CustomFont, w() / 42);
			fl_color(show_closed_score ? FL_RED : FL_BLUE);
			std::ostringstream os;
			os << (show_closed_score ? _ai.score_closed : _ai.score);
			Util::draw_string(os.str(), w() - Util::string_width(os.str()), fl_height() - fl_descent());
		}
	}

	void draw_version()
	{
		fl_font(FL_HELVETICA, _CH / 20);
		fl_color(FL_YELLOW);
		std::ostringstream os;
		os << "v" << VERSION;
		Util::draw_string(os.str(), 2, fl_height() - fl_descent(), true);
	}

	void draw_grayout()
	{
		if (Fl::first_window() != this || _grayout)
		{
			// use shadow image to "gray out" deck
			for (int x = 0; x < w(); x += _CW / 2)
				for (int y = 0; y < h(); y += _CH / 2)
					_shadow.image()->draw(x, y, _CW / 2, _CH / 2, _CW / 4, _CH / 4);
		}
	}

	void draw_closing()
	{
		int X = pack_rect().x + pack_rect().w;
		int Y = change_rect().center().y;
		int closing = _anim_params.closing;
		if (closing >= 1 && closing <= 4)
		{
			Fl_RGB_Image *image = closing <= 2 ? _game.cards.back().rot90_image() : _back.rot90_image();
			int W = (closing == 1 || closing == 4) ? (image->w() / 3) * 2 : image->w() / 2;
			Fl_Image *temp = image->copy(W, image->h());
			// NOTE: shadow should be from rotated image, but it is not noticable..
			Fl_Image *stemp = _shadow.image()->copy(W, image->h());
			X -= temp->w() / 2;
			Y -= temp->h() / 2;
			stemp->draw(X + _CW / 12, Y + _CW / 12);
			temp->draw(X, Y);
			delete stemp;
			delete temp;
		}
	}

	void draw_animated_trick()
	{
		int X = _anim_params.X - _CW / 2;
		int Y = _anim_params.Y - _CH / 2;
		for (size_t i = 0; i < _anim_params.cards; i++)
		{
			_shadow.image()->draw(X + _CW / 12 + i * (_CW / 4), Y + _CW / 12);
			_back.image()->draw(X + i * (_CW / 4), Y);
		}
	}

	void draw_animated_move()
	{
		int X = _anim_params.X - _CW / 2;
		int Y = _anim_params.Y - _CH / 2;
		_shadow.image()->draw(X + _CW / 12, Y + _CW / 12);
		_game.move == AI ? _ai.card.image()->draw(X, Y) : _player.card.image()->draw(X, Y);
	}

	void draw_animated_change()
	{
		// same as:
		draw_animated_move();
	}

	void draw_debug_rects()
	{
		auto draw_rect = [&](Rect r, Fl_Color c_ = FL_GREEN) -> void
		{
			fl_color(c_);
			fl_rect(r.x, r.y, r.w, r.h);
			fl_color(GRAY);
			fl_line_style(FL_DASH);
			fl_xyline(r.x + 1, r.center().y, r.x + r.w - 1);
			fl_yxline(r.center().x, r.y + 1, r.y + r.h - 1);
			fl_line_style(0);
		};

		draw_rect(cards_rect(PLAYER));
		draw_rect(cards_rect(AI));

		draw_rect(move_rect(PLAYER));
		draw_rect(move_rect(AI));

		draw_rect(deck_rect(PLAYER));
		draw_rect(deck_rect(AI));

		draw_rect(pack_rect());

		draw_rect(change_rect());

		draw_rect(gamebook_rect());

		draw_rect(message_rect(PLAYER));
		draw_rect(message_rect(AI));

		draw_rect(closed_rect(PLAYER));
		draw_rect(closed_rect(AI));

		draw_rect(on_table_rect(PLAYER));
		draw_rect(cards_area_rect(), FL_YELLOW);
	}

	bool check_sleep(bool cancel_)
	{
		if (_ai.message != AI_SLEEP)
		{
			Fl::remove_timeout(cb_sleep, this);
			Fl::add_timeout(20.0, cb_sleep, this);
		}
		else
		{
			if (cancel_)
			{
				ai_message(NO_MESSAGE);
				return false;
			}
			// Gimmick: draw an animated sleepy face if player takes too long
			// (This overrides the AI_SLEEP message display)
			static Fl_Anim_GIF_Image sleepyFace((Util::rsc_dir() + "1f634.gif").c_str());
			sleepyFace.scale(_CW / 2, _CW / 2, 1, 1);
			int X = pack_rect().center().x - sleepyFace.w() / 2;
			int Y = pack_rect().y - _CW;
			fl_push_clip(X, Y, sleepyFace.w(), sleepyFace.h());
			draw_table();
			sleepyFace.draw(X, Y, sleepyFace.w(), sleepyFace.h());
			fl_pop_clip();
			Fl::add_timeout(1./10, [](void *d_)
			{
				Deck *deck = static_cast<Deck *>(d_);
				deck->redraw();
			}, this);
			return true;
		}
		return false;
	}

	void draw() override
	{
		// measure a "standard card"
		double ratio = (double)w() / h();
		int W = (w() / 8 + (ratio >= 800. / 600 ? h() / 5 : h() / 10)) / 2;
		W *= _card_scale;
		int H = 1.5 * W;
		bool scale_change = (_CW != W || _CH != H);
		if (scale_change)
		{
			DBG("new card size: " << W << "x" << H << "\n");
		}
		_CW = W;
		_CH = H;
		_card_template.set_pixel_size(_CW, _CH);
		Rect r(cards_rect(PLAYER));
		W = w() / 20 * (_player.cards.size() - 1) + _CW;
		_winning_button->resize(r.x, r.y + r.h, W, h() - (r.y + r.h));

		if (check_sleep(scale_change)) return;

		draw_table();
		draw_gamebook();
		if (_game.trump != NO_SUITE)
			draw_suite_symbol(_game.trump, pack_rect().x + pack_rect().w, pack_rect().y + pack_rect().h + _CH / 5);
		draw_messages();
		draw_20_40_suites();
		draw_cards();
		draw_pack();
		draw_decks();
		draw_move();
		draw_scores();
		if (_anim_params.func)
			std::invoke(_anim_params.func, this);
		if (_player.deck_info)
			draw_player_deck_info(Fl::event_x(), Fl::event_y());
		if (_ai.deck_info)
			draw_ai_deck_info(Fl::event_x(), Fl::event_y());
		draw_version();
		if (::debug >= 3)
			draw_debug_rects();
		if (!_disabled)
			draw_children();
		draw_grayout();
	}

	bool load_game(const std::string &name_);
	void onCmd(const std::string &cmd_);

	void onCmd()
	{
		onCmd(_cmd);
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
		Util::config("fullscreen", (fullscreen_active() ? "1" : "0"));
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
		_cmd_input = new Cmd_Input(_CW / 20, h() - _CW / 5 - _CW / 20, _CW * 2, _CW / 5);
		end();
		_cmd_input->take_focus();
		redraw();
		_cmd_input->when(FL_WHEN_ENTER_KEY|FL_WHEN_NOT_CHANGED);
		_cmd_input->callback([](Fl_Widget *w_, void *)
		{
			std::string c((static_cast<Cmd_Input *>(w_))->value());
			Deck *deck = static_cast<Deck *>((static_cast<Cmd_Input *>(w_))->window());
			deck->_cmd = c; // store cmd for onCmd()
			deck->delayed_call(&Deck::onCmd);
		});
	}

	void welcome()
	{
		_welcome = new Welcome(w() / 2, h() / 4 * 3, _game.cards.size() != 20);
		_welcome->position(x() + (w() - _welcome->w()) / 2, y() + (h() - _welcome->h()) / 2);
		_welcome->stats(make_stats());
		bell(WELCOME);
		Util::run(*_welcome);
		redraw();
	}

	void apply_selections()
	{
		std::string card_root = Util::home_dir() + Card::cardDir + "/";
		std::string cardback = Util::config("cardback");
		if (cardback.empty() || cardback == "default")
		{
			cardback = "Card_back_red.svg";
			Util::config("cardback", cardback);
		}
		_back.image("card_back", card_root + "back/" + cardback);
		for (auto &c : _game.cards) c.reload();
		for (auto &c : _player.cards) c.reload();
		for (auto &c : _ai.cards) c.reload();
		for (auto &c : _player.deck) c.reload();
		for (auto &c : _ai.deck) c.reload();
	}

	void selector()
	{
		std::string cardset = Util::config("cardset");
		std::string cardback = Util::config("cardback");
		_selector = new Selector(w() / 2, h() / 4 * 3);
		_selector->position(x() + (w() - _selector->w()) / 2, y() + (h() - _selector->h()) / 2);
		bell(ANIMATION);
		Util::run(*_selector);
		if (cardset != Util::config("cardset") || cardback != Util::config("cardback"))
		{
			apply_selections();
		}
		redraw();
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

	//
	// UI independent stuff (mostly)
	//

	void debug(bool unconditional_ = false) const
	{
		// log only when change of deck cards
		static Cards cards;
		if (unconditional_ == true ||
			cards != _game.cards)
		{
			cards = _game.cards;
			LOG("cards: " << cards << " (" << cards.size() << ")\n");
		}
		// log only when change of playing cards
		static Cards player_cards;
		static Cards ai_cards;
		static int player_score = 0;
		static int ai_score = 0;
		if (unconditional_ == false &&
			_ai.cards == ai_cards && _player.cards == player_cards &&
			_ai.score == ai_score && _player.score == player_score) return; // no change
		player_cards = _player.cards;
		ai_cards = _ai.cards;
		player_score = _player.score;
		ai_score = _ai.score;

		LOG("PL deck: " << _player.deck << " (" << _player.deck.size() << ")\n");
		LOG("AI deck: " << _ai.deck << " (" << _ai.deck.size() << ")\n");

		LOG("AI cards: " << _ai.cards << " (" << _ai.cards.size() << ")");
		if (_strictness >= 1 && _game.closed == BY_PLAYER)
		{
			LOG("\tscore closed: " << _ai.score_closed);
		}
		else
		{
			LOG("\tscore: " << _ai.score << " pending: " << _ai.pending);
		}
		LOG("\t20/40: ");
		for ([[maybe_unused]]auto s : _ai.s20_40)
			LOG(Card::suite_symbol(s));
		LOG("\n");

		LOG("PL cards: " << _player.cards << " (" << _player.cards.size() << ")");
		if (_strictness >= 1 && _game.closed == BY_AI)
		{
			LOG("\tscore closed: " << _player.score_closed);
		}
		else
		{
			LOG("\tscore: " << _player.score << " pending: " << _player.pending);
		}
		LOG("\t20/40: ");
		for ([[maybe_unused]]auto s : _player.s20_40)
			LOG(Card::suite_symbol(s));
		LOG("\n");
		Util::logstream().flush();
	}

	void init2()
	{
		player_message(NO_MESSAGE);
		ai_message(NO_MESSAGE);
		error_message(NO_MESSAGE);
		_game.closed = NOT;
		_game.marriage = NO_MARRIAGE;
		_game.trump = NO_SUITE;
		_player.s20_40.clear();
		_ai.s20_40.clear();
		_player.score = 0;
		_player.score_closed = 0;
		_player.pending = 0;
		_ai.score = 0;
		_ai.score_closed = 0;
		_ai.pending = 0;
		_ai.display_score = false;
		_player.message = NO_MESSAGE;
		_ai.message = NO_MESSAGE;
		_error_message = NO_MESSAGE;
		_player.move_state = NONE;
		_ai.move_state = NONE;
		_disabled = false;
		_redeal = false;
		_player.deck_info = false;
		_ai.deck_info = false;
	}

	void init()
	{
		collect();
		init2();
		_game.cards.shuffle();
		assert(_game.cards.size() == 20);
		bell(SHUFFLE);
		animate_shuffle();
		debug();
		deal();
		assert(_player.cards.size() == 5);
		assert(_ai.cards.size() == 5);
		_engine.sort_cards(_player.cards)
		       .sort_cards(_ai.cards);
		assert(_player.cards.size() == 5);
		assert(_ai.cards.size() == 5);
		redraw();
		debug();
	}

	void collect()
	{
		LOG("collect\n");
		for (auto &c : _player.cards)
			_game.cards.push_front(c);
		for (auto &c : _player.deck)
			_game.cards.push_front(c);
		for (auto &c :_ai.cards)
			_game.cards.push_front(c);
		for (auto &c :_ai.deck)
			_game.cards.push_front(c);
		_player.cards.clear();
		_ai.cards.clear();
		_player.deck.clear();
		_ai.deck.clear();
		if (_player.move_state != NONE)
			_game.cards.push_front(_player.card);
		if (_ai.move_state != NONE)
			_game.cards.push_front(_ai.card);
		if (_game.cards.size() != 20)
			DBG("#cards: " << _game.cards.size())
		assert(_game.cards.size() == 20);
		assert(_game.cards.check());
	}

	void deal()
	{
		LOG("dealer is " << (_game.move == PLAYER ? "AI" : "PLAYER") << "\n");
		// 3 cards to player
		animate_deal(_game.move == PLAYER ? PLAYER : AI, 3);
		for (size_t i = 0; i < 3; i++)
		{
			Card c = _game.cards.front();
			_game.cards.pop_front();
			_game.move == PLAYER ? _player.cards.push_front(c) : _ai.cards.push_front(c);
		}

		// 3 cards to ai
		animate_deal(_game.move == PLAYER ? AI : PLAYER, 3);
		for (size_t i = 0; i < 3; i++)
		{
			Card c = _game.cards.front();
			_game.cards.pop_front();
			_game.move == PLAYER ? _ai.cards.push_front(c) : _player.cards.push_front(c);
		}

		// trump card
		Card trump = _game.cards.front();
		_game.cards.pop_front();
		_game.cards.push_back(trump); // will be the last card (_game.cards.back())
		_game.trump = trump.suite();
		LOG("trump: " << Card::suite_symbol(_game.trump) << "\n");
		redraw();

		// 2 cards to player
		animate_deal(_game.move == PLAYER ? PLAYER : AI, 2);
		for (size_t i = 0; i < 2; i++)
		{
			Card c = _game.cards.front();
			_game.cards.pop_front();
			_game.move == PLAYER ? _player.cards.push_front(c) : _ai.cards.push_front(c);
		}

		// 2 cards to ai
		animate_deal(_game.move == PLAYER ? AI : PLAYER, 2);
		for (size_t i = 0; i < 2; i++)
		{
			Card c = _game.cards.front();
			_game.cards.pop_front();
			_game.move == PLAYER ? _ai.cards.push_front(c) : _player.cards.push_front(c);
		}

		if (::debug)
		{
			// TEST TEST
			Suites res;
			res = _engine.have_40(_player.cards);
			if (res.size())
				DBG("player cards contain 40!\n")
			res = _engine.have_20(_player.cards);
			if (res.size())
				DBG("player cards contain " << res.size() << "x20!\n")
			res = _engine.have_40(_ai.cards);
			if (res.size())
				DBG("AI cards contain 40!\n")
			res = _engine.have_20(_ai.cards);
			if (res.size())
				DBG("AI cards contain " << res.size() << "x20!\n")
			Move i = _engine.find(Card(JACK, _game.cards.back().suite()), _player.cards);
			if (i)
				DBG("player cards can change Jack!\n")
			i = _engine.find(Card(JACK, _game.cards.back().suite()), _ai.cards);
			if (i)
				DBG("AI cards can change Jack!\n")
		}
	}

	void fillup_cards()
	{
		if (_game.closed == NOT && _player.cards.size() < 5 && _ai.cards.size() < 5)
		{
			// give cards from pack
			if (_game.cards.size())
			{
				Card c = _game.cards.front();
				_game.cards.pop_front();
				_game.move == AI ? _ai.last_drawn = c : _player.last_drawn = c;

				animate_fillup(_game.move == AI ? AI : PLAYER);

				if (_game.move == AI)
					_ai.cards.push_front(c);
				else
					_player.cards.push_front(c);
			}

			if (_game.cards.size())
			{
				Card c = _game.cards.front();
				_game.move == PLAYER ? _ai.last_drawn = c : _player.last_drawn = c;
				_game.cards.pop_front();

				animate_fillup(_game.move == AI ? PLAYER : AI);

				if (_game.move == AI)
					_player.cards.push_front(c);
				else
					_ai.cards.push_front(c);
			}
			assert(_player.cards.size() == _ai.cards.size());
			_engine.sort_cards(_player.cards)
			       .sort_cards(_ai.cards);
			debug();

			if (_game.cards.empty())
			{
				_game.closed = AUTO; // same rules as closing now
				LOG("*** pack cleared - end game ***\n");
			}
		}
		redraw();
	}

	void show_win_msg() override
	{
		cursor(FL_CURSOR_DEFAULT);
		int ascore = _game.book.ai_score();
		std::string m(Util::message(ascore ? YOU_WIN : YOU_WINX));
		Alert &alert = *new Alert(m.c_str(), Util::message(TITLE).c_str());
		alert.set_bg_image((Util::rsc_dir() + (ascore ? "1f3c6.gif" : "1f4af.gif")).c_str())
		     .center_on(Rect(*this))
		     .run();
	}

	void show_lost_msg() override
	{
		cursor(FL_CURSOR_DEFAULT);
		int pscore = _game.book.player_score();
		std::string m(Util::message(pscore ? YOU_LOST : YOU_LOSTX));
		Alert &alert = *new Alert(m.c_str(), Util::message(TITLE).c_str());
		alert.set_bg_image((Util::rsc_dir() + (pscore ? "1f61e.gif" : "1fae3.gif")).c_str())
		     .center_on(Rect(*this))
		     .run();
	}

	void save_config() const
	{
		Util::config("cards", std::string()); // don't save cards string!
		Util::config("width", std::to_string(w()));
		Util::config("height", std::to_string(h()));
		Util::config("xpos", std::to_string(x()));
		Util::config("ypos", std::to_string(y()));
		Util::config("card_scale", std::to_string(_card_scale));
#ifdef USE_MINIAUDIO
		Util::config("volume", std::to_string(_audio.volume()));
#endif
		Util::save_config();
	}

	void save_stats() const
	{
		Util::save_stats();
	}

	void save_gamebook()
	{
		assert(_game.book.size());

		_game.book.reset_current();

		// append current gamebook to saved gamebooks
		std::string gb = Util::stats("gamebook");

		// count entries
		int entries = gb.empty() ? 0 : 1;
		size_t pos = (size_t)-1;
		while ((pos = gb.find(';', ++pos)) != std::string::npos) entries++;

		// limit to last 10 entries
		for (int i = 0; i < entries - 10; i++)
		{
			pos = gb.find(';');
			if (pos == std::string::npos) break;
			gb.erase(0, pos + 1);
		}
		// add current gamebook to back of entries
		std::string gamebook = gb + (gb.size() ? ";" : "") + _game.book.str();

		// write to file
		Util::stats("gamebook", gamebook);

		// update history display
		_game.book.history(gamebook);
	}

	bool check_end_match()
	{
		int pscore = _game.book.player_score();
		int ascore = _game.book.ai_score();
		LOG("match score PL:AI: " << pscore << ":" << ascore << "\n");
		fl_message_font_ = FL_COURIER_BOLD;
		fl_message_size_ = h() / 40;
		if (pscore >= MATCH_SCORE)
		{
			LOG("You win match " << pscore << ":" << ascore << "\n");
			_player.matches_won++;
			Util::stats("player_matches_won", std::to_string(_player.matches_won));
			bell(YOU_WIN);
			show_win_msg();
			save_gamebook();
			_game.book.clear();
			redraw();
			return true;
		}
		else if (ascore >= MATCH_SCORE)
		{
			LOG("AI wins match " << ascore << ":" << pscore << "\n");
			_ai.matches_won++;
			Util::stats("ai_matches_won", std::to_string(_ai.matches_won));
			bell(YOU_LOST);
			show_lost_msg();
			save_gamebook();
			_game.book.clear();
			redraw();
			return true;
		}
		return false;
	}

	void update_gamebook()
	{
		if (_game.closed != NOT && _game.closed != AUTO)
		{
			// game was closed, now the closer must have enough points
			if (_game.closed == BY_PLAYER)
			{
				if (_player.score >= 66)
				{
					int score = _strictness >= 1 ? _ai.score_closed : _ai.score;
					_game.book.emplace_back(score < 33 ? score == 0 ? 3 : 2 : 1, 0);
				}
				else
				{
					// TODO: officially the points are counted at the moment of closing
					_game.book.emplace_back(0, _player.score < 33 ? _player.score == 0 ? 3 : 2 : 2);
				}
			}
			else
			{
				// closed by AI
				if (_ai.score >= 66)
				{
					int score = _strictness >= 1 ? _player.score_closed : _player.score;
					_game.book.emplace_back(0, score < 33 ? score == 0 ? 3 : 2 : 1);
				}
				else
				{
					// TODO: officially the points are counted at the moment of closing
					_game.book.emplace_back(_ai.score < 33 ? _ai.score == 0 ? 3 : 2 : 2, 0);
				}
			}
		}
		else
		{
			// normal game (not closed)
			if (_game.move == PLAYER)
			{
				_game.book.emplace_back(_ai.score < 33 ? _ai.score == 0 ? 3 : 2 : 1, 0);
			}
			else
			{
				_game.book.emplace_back(0, _player.score < 33 ? _player.score == 0 ? 3 : 2 : 1);
			}
		}
	}

	virtual void prepare_game() override
	{
		_history.clear();
		_engine.init();
		cursor(FL_CURSOR_DEFAULT);
		_redeal ? _redeal_button->show() : _redeal_button->hide();
		_winning_button->hide();
		_winning_claim = false;
		update();
	}

	int run()
	{
		Player playout(::first_to_move);
		while (playing())
		{
			_redeal = true;
			_history.clear();
			prepare_game();
			game(playout);
			if (_redeal) continue;
			playout = playout == PLAYER ? AI : PLAYER;
			update_gamebook();

			if (!playing()) break;

			auto &[pscore, ascore] = _game.book.back();
			if (pscore)
				LOG("PL scores " << pscore << "\n");
			if (ascore)
				LOG("AI scores " << ascore << "\n");
			update();
			check_end_match();
		}
		save_config();
		save_stats();
		return 0;
	}

	void message(Message m_, bool bell_ = false) override
	{
		if (m_ == CLOSED) m_ = _game.move == AI ? AI_CLOSED : YOU_CLOSED;
		// NOTE: conflict with Fl_Widget::CHANGED!
		if (m_ == Message::CHANGED) m_ = _game.move == AI ? AI_CHANGED : YOU_CHANGED;
		_game.move == AI ? ai_message(m_, bell_) : player_message(m_, bell_);
		debug();
	}

	void ai_message(Message m_, bool bell_ = false)
	{
		if (bell_) bell(m_);
		_ai.message = m_;
		std::string m(Util::message(m_));
		DBG("ai_message(" << m << ")\n")
		delete _ai_anim_text;
		_ai_anim_text = nullptr;
		if (_animation_level > 1)
		{
			_ai_anim_text = new AnimText(m, *this);
		}
		update();
	}

	void player_message(Message m_, bool bell_ = false)
	{
		if (bell_) bell(m_);
		_player.message = m_;
		std::string m(Util::message(m_));
		DBG("player_message(" << m << ")\n")
		delete _player_anim_text;
		_player_anim_text = nullptr;
		if (_animation_level > 1)
		{
			_player_anim_text = new AnimText(m, *this);
		}
		update();
	}

	void error_message(Message m_, bool bell_ = false)
	{
		if (bell_) bell(m_, false);
		_error_message = m_;
		std::string m(Util::message(m_));
		DBG("error_message(" << m << ")\n")
		update();
	}

	bool ai_wins(const std::string &log_, Message player_message_ = NO_MESSAGE)
	{
		LOG(log_ + "\n");
		ai_message(AI_GAME, true);
		player_message(player_message_);
		_ai.games_won++;
		Util::stats("ai_games_won", std::to_string(_ai.games_won));
		_ai.display_score = true;
		wait(2.0);
		return true;
	}

	bool player_wins(const std::string &log_, Message ai_message_ = NO_MESSAGE)
	{
		LOG(log_ + "\n");
		player_message(YOUR_GAME, true);
		ai_message(ai_message_);
		_player.games_won++;
		Util::stats("player_games_won", std::to_string(_player.games_won));
		_ai.display_score = true;
		wait(2.0);
		return true;
	}

	void bell([[maybe_unused]]Message m_ = NO_MESSAGE, [[maybe_unused]]bool visual_ = true) override
	{
#ifdef USE_MINIAUDIO
		std::string snd = sound[m_];
		if (snd.size())
		{
			snd = Util::rsc_dir() + snd + ".mp3";
			DBG("play '" << snd << "'\n");
			if (std::filesystem::exists(snd))
				_audio.play(snd);
			else
				snd.erase();
		}
		if (snd.empty())
		{
			fl_beep();
			if (visual_)
				flicker();
		}
#else
		// NOTE: under Wayland fl_beep() outputs a \007 character to stderr.
		//       This does not work for applications run from gnome dock,
		//       most likely because stderr/stdout are redirected or disabled.
		fl_beep();
#endif
		if (visual_)
			flicker();
	}

	Result test_end()
	{
		auto no_cards_in_play = [&]() -> bool
		{
			return _player.cards.empty() && _ai.cards.empty() && _player.move_state == NONE && _ai.move_state == NONE;
		};

		if (_game.closed == NOT || _game.closed == AUTO)
		{
			if (_game.move == PLAYER && _player.score >= 66)
			{
				return PLAYER_WINS_BY_SCORE;
			}
			else if (_game.move == AI && _ai.score >= 66)
			{
				return AI_WINS_BY_SCORE;
			}
			else if (no_cards_in_play())
			{
				if (_game.move == AI)
				{
					return AI_WINS_BY_LAST_TRICK;
				}
				else
				{
					// _game.move = PLAYER
					return PLAYER_WINS_BY_LAST_TRICK;
				}
			}
		}
		else
		{
			// closed
			if (_game.closed == BY_PLAYER && _game.move == PLAYER && _player.score >= 66)
			{
				return PLAYER_WINS_CLOSED_GAME;
			}
			else if (_game.closed == BY_AI && _game.move == AI && _ai.score >= 66)
			{
				return AI_WINS_CLOSED_GAME;
			}
			else if (no_cards_in_play())
			{
				// closed and last trick done
				if (_game.closed == BY_PLAYER)
				{
					return AI_WINS_PLAYER_CLOSED_NOT_ENOUGH;
				}
				else
				{
					// _game.closed = BY_AI
					return PLAYER_WINS_AI_CLOSED_NOT_ENOUGH;
				}
			}
		}
		return NO_WIN;
	}

	bool check_end()
	{
		debug();
		Result res = test_end();
		switch (res)
		{
			case PLAYER_WINS_BY_SCORE:             return player_wins("Player wins!");
			case PLAYER_WINS_BY_LAST_TRICK:        return player_wins("Player wins by last trick!");
			case PLAYER_WINS_CLOSED_GAME:          return player_wins("Player wins closed game!");
			case PLAYER_WINS_AI_CLOSED_NOT_ENOUGH: return player_wins("Player wins because AI closed and has not enough!", AI_NOT_ENOUGH);

			case AI_WINS_BY_SCORE:                 return ai_wins("AI wins!");
			case AI_WINS_BY_LAST_TRICK:            return ai_wins("AI wins by last trick!");
			case AI_WINS_CLOSED_GAME:              return ai_wins("AI wins closed game!");
			case AI_WINS_PLAYER_CLOSED_NOT_ENOUGH: return ai_wins("AI wins because player closed and has not enough!", YOU_NOT_ENOUGH);
			default: break;
		}
		return false;
	}

	void check_trick(Player move_)
	{
		_game.marriage = NO_MARRIAGE;
		_game.move = _engine.check_trick(move_);

		if (_game.move == PLAYER) // player won trick
		{
			_player.deck.push_front(_ai.card);
			_player.deck.push_front(_player.card);
			_player.score += _player.card.value() + _ai.card.value() + _player.pending;
			_player.pending = 0;
			player_message(YOUR_TRICK);
			ai_message(NO_MESSAGE);
		}
		else
		{
			_ai.deck.push_back(_player.card);
			_ai.deck.push_back(_ai.card);
			_ai.score += _ai.card.value() + _player.card.value() + _ai.pending;
			_ai.pending = 0;
			ai_message(AI_TRICK);
			player_message(NO_MESSAGE);
		}
	}

	static void cb_sleep(void *d_)
	{
		if (Fl::first_window() == static_cast<Deck *>(d_))
			(static_cast<Deck *>(d_))->ai_message(AI_SLEEP);
	}

	void player_move() override
	{
		auto estimated_ai_cards_value = [&]() -> int
		{
			Cards a = _ai.cards + _game.cards; // all cards AI can have
			a.sort_by_value(false); // sort low->high
			int value = 0; // calc. value of lowest cards (only for the number of cards AI is holding)
			for (size_t i = 0; i < _ai.cards.size(); i++)
				value += a[i].value();
			return value;
		};

		cursor(FL_CURSOR_DEFAULT);
		update_history();
		//  Try to offer a 'claim remaining tricks' button
		if (_game.closed != NOT &&  _player.cards.size() >= 2 && _ai.move_state == NONE &&
		    _engine.highest_cards_in_hand(_player.cards).size() == _player.cards.size() &&
			 (int)_engine.highest_trumps_in_hand(_player.cards).size() >= _engine.max_trumps(AI) &&
			_player.cards.value() + estimated_ai_cards_value() + _player.pending + _player.score >= 66)
		{
			LOG("You have a winner hand!\n");
			_winning_button->show();
		}
		else
		{
			_winning_button->hide();
		}
		while (playing() && _player.move_state != ON_TABLE && _redeal == false && _winning_claim == false)
		{
			wait(0.);
		}
		Fl::remove_timeout(cb_sleep, this);
		_redeal_button->hide();
		_restart = false;
	}

	void redeal()
	{
		LOG("***redeal***\n");
		_redeal = true;
	}

	void winning_claim()
	{
		_player.score = 66;
		_winning_claim = true;
	}

	bool playing() override
	{
		return Fl::first_window() && _restart == false;
	}

	void ai_move() override
	{
		cursor(FL_CURSOR_WAIT);
		wait(2.0);
		if (!playing()) return;
		_engine.ai_move();
	}

	void game(Player playout_)
	{
		DBG(std::string(80, '~') << "\n");
		DBG("new game: " << (playout_ == PLAYER ? "PLAYER" : "AI") << " to lead\n");
		_game.move = playout_;
		init();
		if (game_to_load.size())
		{
			load_game(game_to_load);
			game_to_load.erase();
		}

		while (playing() && (_player.cards.size() || _ai.cards.size()))
		{
			if (_game.move == PLAYER)
			{
				ai_message(NO_MESSAGE);
				_player.move_state = NONE;
				if (check_end()) break;
				player_message(_ai.move_state == NONE ? YOU_LEAD : YOUR_TURN);
				player_move();
				ai_message(NO_MESSAGE);
				if (_redeal) break;

				if (check_end()) break; // if enough from 20/40!!

				if (!playing()) break;
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
					_game.move = AI;
				}
			}

			if (_game.move == AI)
			{
				player_message(NO_MESSAGE);
				if (check_end()) break;
				_ai.move_state = MOVING;
				ai_message(_player.move_state == NONE ? AI_LEADS : AI_TURN);
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
					_game.move = PLAYER;
				}
			}
		}
		if (!playing()) return;

		_game.marriage = NO_MARRIAGE;
		if (_redeal == true) return;

		wait(2.0);
	}

	bool idle() const
	{
		return _player.move_state == NONE && _ai.move_state == NONE &&	_anim_params.func == nullptr;
	}

	void wait(double s_) override
	{
		if (!s_)
		{
			Fl::wait();
			return;
		}
		std::string fast = Util::config("fast");
		if ((fast.empty() || fast == "1") && s_ >= 1.0)
		{
			s_ /= 2;
		}
		if (s_ > 0.1 || ::debug > 2)
		{
			DBG("wait(" << s_ << ")\n");
		}
		_disabled = true;
		double min_wait = s_ > 1./50 ? 1./50 : s_;
		std::chrono::time_point<std::chrono::system_clock> start =
			std::chrono::system_clock::now();
		while (playing() && _disabled)
		{
			Fl::wait(min_wait);
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
		os << Util::message(GAMES_WON) << _player.games_won << " / " << _ai.games_won;
		os << "    " << Util::message(MATCHES_WON) << _player.matches_won << " / " << _ai.matches_won;
		return os.str();
	}

	void update_history()
	{
		GameState h(_player, _ai, _game);
		_history.push_back(h);
	}

	bool back_history()
	{
		if (_history.empty()) return false;
		GameState h = _history.back();
		if (_history.size() != 1)
			_history.pop_back();
		_player = h.player;
		_ai = h.ai;
		_game = h.game;
		return true;
	}

private:
	// Engine
	PlayerData _player;
	PlayerData _ai;
	GameData _game;
	Engine _engine;

	// UI
	Message _error_message;
	bool _disabled;
	bool _redeal;
	Card _card_template;
	CardImage _back;
	CardImage _shadow;
	CardImage _outline;
	CardImage _empty;
	int _CW;					// card pixel width (used as "unit" for UI)
	int _CH;					// card pixel height
	Cmd_Input *_cmd_input;
	Button *_redeal_button;
	Button *_winning_button;
	bool _winning_claim;
	Welcome *_welcome;
	Selector *_selector;
	bool _grayout;
#ifdef USE_MINIAUDIO
	Audio _audio;
#endif
	std::string _cmd;
	CardAnimParams _anim_params;
	int _strictness;
	int _animation_level;
	bool _show_ai_cards;
	bool _restart;
	std::vector<GameState> _history;
	double _card_scale;
	AnimText *_player_anim_text;
	AnimText *_ai_anim_text;
};
