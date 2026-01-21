#include "Deck.h"
#include "Engine.h"

#include "Util.h"

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

#include <string>
#include <vector>
#include <chrono>
#include <filesystem>
#include <functional>
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

class Deck : public Fl_Double_Window, public UI
{
public:
	Deck() : Fl_Double_Window(800, 600),
		_engine(_game, _player, _ai, *this),
		_error_message(NO_MESSAGE),
		_disabled(false),
		_card_template(QUEEN, SPADE),
		_CW(w() / 8),
		_CH(1.5 * w()),
		_cmd_input(nullptr),
		_redeal_button(nullptr),
		_welcome(nullptr),
		_grayout(false),
		_animate_xy(std::make_pair(-1, -1)),
		_animate(nullptr)
	{
		_player.games_won = atoi(Util::stats("player_games_won").c_str());
		_ai.games_won = atoi(Util::stats("ai_games_won").c_str());
		_player.matches_won = atoi(Util::stats("player_matches_won").c_str());
		_ai.matches_won = atoi(Util::stats("ai_matches_won").c_str());
		copy_label(Util::message(TITLE).c_str());
		fl_register_images();
		std::string root = Util::homeDir() + cardDir;
		std::string cardback = Util::config("cardback");
		if (cardback.empty())
			cardback = "Card_back_red.svg";
		_back.image("card_back", root + "/back/" + cardback);
		_shadow.image("card_shadow", Util::homeDir() + cardDir + "/Card_shadow.svg");
		_outline.image("card_outline", Util::homeDir() + cardDir + "/Card_outline.svg");
		_game.cards = Cards::fullcards();
		_game.cards.check();
		_card_template = _game.cards[0];
		_engine.unit_tests();
		default_cursor(FL_CURSOR_HAND);
		Fl_RGB_Image *icon = Card(QUEEN, HEART).image();
		icon->normalize();
		default_icon(icon);
		_redeal_button = new Button(w() - 100, h() - 40, 100, 40, Util::message(REDEAL).c_str());
		_redeal_button->color(FL_YELLOW);
		_redeal_button->selection_color(fl_darker(FL_YELLOW));
		_redeal_button->visible_focus(0);
		_redeal_button->hide();
		resizable(this);
		size_range(400, 300, 0, 0, 0, 0, 1);
		int width = atoi(Util::config("width").c_str());
		int height = atoi(Util::config("height").c_str());
		if (width > 400 && height > 300)
			size(width, height);
		int xpos = atoi(Util::config("xpos").c_str());
		int ypos = atoi(Util::config("ypos").c_str());
		position(xpos, ypos);
		end();
		_redeal_button->callback([](Fl_Widget *wgt_, void *)
		{
			static_cast<Deck *>(wgt_->window())->init();
		});
		if (Util::config("fullscreen") == "1")
		{
			toggle_fullscreen();
		}
	}

	~Deck()
	{
		delete _redeal_button;
		delete _cmd_input;
	}

	void update() override
	{
		redraw();
	}

	void message(Message m_, bool bell_ = false) override
	{
		if (m_ == CLOSED)  m_ = _game.move == AI ? AI_CLOSED : YOU_CLOSED;
		// NOTE: conflict with Fl_Widget::CHANGED!
		if (m_ == Message::CHANGED) m_ = _game.move == AI ? AI_CHANGED : YOU_CHANGED;
		_game.move == AI ? ai_message(m_, bell_) : player_message(m_, bell_);
	}

	void ai_message(Message m_, bool bell_ = false)
	{
		if (bell_) bell(m_);
		_ai.message = m_;
		std::string m(Util::message(m_));
		DBG("ai_message(" << m << ")\n")
		redraw();
	}

	void player_message(Message m_, bool bell_ = false)
	{
		if (bell_) bell(m_);
		_player.message = m_;
		std::string m(Util::message(m_));
		DBG("player_message(" << m << ")\n")
		redraw();
	}

	void error_message(Message m_, bool bell_ = false)
	{
		if (bell_) bell(m_);
		_error_message = m_;
		std::string m(Util::message(m_));
		DBG("error_message(" << m << ")\n")
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
			_player.move_state = NONE;
			_player.cards.push_back(_player.card);
			_player.cards.sort();
			_engine.test_change(_player, true);
			return true;
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
		if (_game.closed == NOT && idle() &&
		    _game.cards.front().rect().includes(x_, y_))
		{
		   if (_game.cards.size() < 4)
			{
				error_message(NO_CLOSE, true);
			}
			else
			{
				LOG("closed by player!\n");
				_game.closed = BY_PLAYER;
				player_message(YOU_CLOSED, true);
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

	bool can_trick(const Card &c_, const Cards &cards_) const
	{
		for (auto &c : cards_)
		{
			if (_engine.card_tricks(c, c_)) return true;
		}
		return false;
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
		else if (Fl::event_key(' ') && ::debug > 1)
		{
			static bool paused = false;
			paused = !paused;
			if (paused)
			{
				Fl::add_timeout(0.0, [](void *d_)
				{
					Deck *deck = static_cast<Deck *>(d_);
					deck->_grayout = true;
					while (paused)	{ deck->wait(0.1); deck->redraw(); }
					deck->_grayout = false;
				}, this);
			}
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
		_game.marriage = NO_MARRIAGE;
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

			if (_game.closed == NOT && _game.cards.size() && _ai.move_state == NONE &&
		    _game.cards.back().rect().includes(Fl::event_x(), Fl::event_y()))
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
			Fl::remove_timeout(cb_sleep, this);
			Fl::add_timeout(20., cb_sleep, this);
			ai_message(NO_MESSAGE);
			_game.move == AI ? _ai.last_drawn = Card() : _player.last_drawn = Card();
			handle_click();
			return 1;
		}
		return ret;
	}

	Rect gamebook_rect() const
	{
		return Rect(w()/40, h() / 2 - _CH / 2, _CW, _CH);
	}

	Rect cards_rect(Player player_) const
	{
		return Rect(w() / 20 + w() / 2 - w() / 24,
			(player_ == AI ? h() / 40 : h() - _CH - h() / 40),
			4 * w() / 20 + _CW,
			(player_ == AI ? _CH / 3 : _CH));
	}

	Rect change_rect() const
	{
		return Rect(w() / 3 - _CW + _CW / 4,
			(h() - _CW) / 2,
			_CH,
			_CW
			);
	};

	Rect deck_rect(Player player_) const
	{
		return Rect(w() - _CW - 2, (player_ == AI ?
			h() / 10 - w() / 800 : h() - _CH - h() / 10) , _CW, _CH);
	}

	Rect move_rect(Player player_) const
	{
		int ma = h() / 40 + _CH / 3 + h() / 40 + _CH / 2;
		int mp = h() - h() / 40 - _CH - h() / 40 - _CH / 2;
		int m = (ma + mp) / 2;
		return Rect(
			(player_ == AI ? w() - w() / 3 : _player.move_state == MOVING ? Fl::event_x() - _CW / 2 : w() - w() / 2),
			(player_ == AI ? m - _CH / 2 - _CH / 8: _player.move_state == MOVING ? Fl::event_y() - _CH / 2 : m - _CH / 2 + _CH / 8),
			_CW,
			_CH
			);
	}

	Rect pack_rect() const
	{
		return Rect(w() / 3 - _CW / 4 - _CW, (h() - _CH) / 2, _CW, _CH);
	}

	void draw_gamebook()
	{
		fl_color(fl_lighter(fl_lighter(FL_YELLOW)));
		int X = gamebook_rect().x;
		int Y = gamebook_rect().y;
		int W = gamebook_rect().w;
		int H = gamebook_rect().h;
		fl_rectf(X, Y, W, H);
		fl_color(GRAY);
		fl_rect(X, Y, W, H);
		fl_color(FL_BLACK);
		fl_font(FL_COURIER, _CH / 14);
		X += _CW / 20;
		Y += fl_descent() + fl_height();
		Util::draw_color_text(Util::message(GAMEBOOK), X, Y, text_colors);
		fl_line_style(FL_SOLID, 2);
		fl_line(X, Y + fl_descent(), X + _CW - _CW / 10, Y + fl_descent());
		Y += _CH / 10;
		Util::draw_color_text(Util::message(GB_HEADLINE), X, Y);
		H = _CH - _CH / 5;
		fl_line_style(FL_SOLID, 1);
		W = _CW - _CW / 10;
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
			Util::draw_color_text(buf, X, Y);
		};

		// limit display to last 8 scores
		size_t first = _game.book.size() > 8 ? _game.book.size() - 8 : 0;
		if (first == 0)
			draw_score(std::make_pair(0, 0));
		for (size_t i = 0; i < _game.book.size(); i++)
		{
			auto s = _game.book[i];
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
			Util::draw_color_text(buf, X, Y);
		}
	}

	void draw_deck_info(int x_, int y_, const Cards &deck_, int max_tricks_ = 8)
	{
		fl_color(fl_lighter(fl_lighter(FL_YELLOW)));
		fl_rectf(x_, y_, w() / 10, w() / 7);
		fl_color(GRAY);
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
			Util::draw_color_text(s, x_ + w() / 80, y_ + w() / 50 + i * w() / 100);
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
		Util::draw_string(text_, x_ - fl_width(text_) / 2, y_ + fl_height() / 2 - fl_descent());
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

		std::string symbol_image = Util::cardset_dir() + suite_symbol_image(c.suite());
		if (std::filesystem::exists(symbol_image + ".svg"))
		{
			os << "^|" << symbol_image << "|";
		}
		else
		{
			os << c.suite_symbol();
		}
		Util::draw_string(os.str(), x_ - Util::string_size(os.str()) / 2, y_);
	}

	std::string background_image()
	{
		std::string def_image(Util::homeDir() + "rsc/deck.gif");
		std::string image = Util::config("background");
		if (image == "NONE") return "";
		if (image == "") return def_image;
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
			std::string player_message = Util::message(_player.message);
			fl_font(FL_HELVETICA, w() / (player_message.back() == '!' ? 24 : 34));
			fl_color(FL_RED);
			Util::draw_string(player_message, w() / 4 - Util::string_size(player_message) / 2, h() - h() / 8);
		}
		if (_ai.message != NO_MESSAGE)
		{
			std::string ai_message = Util::message(_ai.message);
			fl_font(FL_HELVETICA, w() / (ai_message.back() == '!' ? 24 : 34));
			size_t pos = ai_message.find("!!");
			if (pos != std::string::npos)
				ai_message.erase(pos, 2);
			fl_color(FL_RED);
			Util::draw_string(ai_message, w() / 4 - Util::string_size(ai_message) / 2, h() / 8);
		}
		if (_error_message != NO_MESSAGE)
		{
			std::string error_message = Util::message(_error_message);
			fl_color(FL_RED);
			fl_rectf(0, h() - h() / 40, w(), h() / 40);
			fl_font(FL_HELVETICA|FL_BOLD, h() / 50);
			fl_color(FL_WHITE);
			Util::draw_string(error_message, w() / 2 - fl_width(error_message.c_str()) / 2, h() - fl_descent());
		}
		if (_game.closed != NOT && _ai.display_score == false)
		{
			fl_font(FL_HELVETICA, _CH / 7);
			fl_color(GRAY);
			static const std::string closed_sym =
#ifndef WIN32
				"⛔";
#else
				"^|26d4|";
#endif
			if (_game.closed == BY_AI)
			{
				int X = w() / 4 - fl_width(closed_sym.c_str()) / 2;
				int Y = h() / 8 - _CH / 7;
				Util::draw_string(closed_sym, X, Y);
			}
			if (_game.closed == BY_PLAYER)
			{
				int X = w() / 4 - Util::string_size(closed_sym) / 2;
				int Y = h() - h() / 16;
				Util::draw_string(closed_sym, X, Y);
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
			if (::debug > 1)
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

	void do_animate(int src_X_, int src_Y_, int dest_X_, int dest_Y_, int steps_ = 5)
	{
		int dx = dest_X_ - src_X_;
		int dy = dest_Y_ - src_Y_;

		for (int i = 0; i < steps_; i++)
		{
			int X = src_X_ + (floor)(((double)dx / steps_) * i);
			int Y = src_Y_ + (floor)(((double)dy / steps_) * i);
			_animate_xy = std::make_pair(X, Y);
			wait(1./50);
			redraw();
		}
		_animate = nullptr;
	}

	void animate_move() override
	{
//		int src_X = cards_rect(_game.move).center().first;
		int src_X = cards_rect(_game.move).x + _CW / 2;
		int src_Y = cards_rect(_game.move).center().second;

		int dest_X = move_rect(_game.move).center().first;
		int dest_Y = move_rect(_game.move).center().second;

		_animate = &Deck::draw_animated_move;

		do_animate(src_X, src_Y, dest_X, dest_Y);
	}

	void animate_trick()
	{
		int src_X = move_rect(_game.move).center().first;
		int src_Y = move_rect(_game.move).center().second;

		int dest_X = deck_rect(_game.move).center().first;
		int dest_Y = deck_rect(_game.move).center().second;

		_animate = &Deck::draw_animated_trick;

		do_animate(src_X, src_Y, dest_X, dest_Y);
	}

	void animate_change(bool from_hand_ = false) override
	{
		int src_X = change_rect().center().first;
		int src_Y = change_rect().center().second;

		int dest_X = cards_rect(_game.move).center().first;
		int dest_Y = cards_rect(_game.move).center().second;

		_animate = &Deck::draw_animated_change;
		if (from_hand_)
			do_animate(dest_X, dest_Y, src_X, src_Y, 10);
		else
			do_animate(src_X, src_Y, dest_X, dest_Y, 10);
	}

	void draw_pack()
	{
		// _game.cards.back() is the trump card
		if (_game.cards.size())
		{
			int X = w() / 3 - _CW + _CW/4;
			int Y = (h() - _CW) / 2;
			if (_game.closed == NOT && _game.cards.size() != 20)
			{
				_game.cards.back().quer_image()->draw(X, Y);
				_game.cards.back().rect(Rect(X, Y, _game.cards.back().image()->h(), _game.cards.back().image()->w()));
			}

			// pack position
			X = pack_rect().x;
			Y = pack_rect().y;
			if (_game.cards.size())
			{
				for (size_t i = 0; i < _game.cards.size() - 1; i++)
				{
					// NOTE: 200 is a 'realistic' value for card the 'height'
					//       of the full card pack with 20 cards.
					//       But the pack with 10 cards after dealing just look
					//       better, when single cards are visible.
					//       So a compromise...
					double h = (double)_CW / (_game.cards.size() == 20 ? 200 : 100);
					if (h > 1.) h = (int)h;
					double x = (double)X - i * h +.5;
					double y = (double)Y - i * h + .5;
					_back.image()->draw(floor(x), floor(y));
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
			int X = w() / 3 - _CW - _CW / 4;
			int Y = (h() - _CH) / 2;
			_outline.image()->draw(X, Y);
		}

		if (_game.closed != NOT && _game.cards.size())
		{
			int X = w() / 3 - _CW + _CW / 4;
			int Y = (h() - _CW) / 2;
			_back.quer_image()->draw(X - w() / 16, Y);
		}
	}

	void draw_decks()
	{
		// show played pack
		int X = deck_rect(PLAYER).x;
		int Y = deck_rect(PLAYER).y;
		for (size_t i = 0; i < _player.deck.size(); i++)
		{
			_back.image()->draw(X - i * w() / 800, Y - i * w() / 800);
		}
		X = deck_rect(AI).x;
		Y = deck_rect(AI).y;
		for (size_t i = 0; i < _ai.deck.size(); i++)
		{
			_back.image()->draw(X - i * w() / 800, Y - i * w() / 800);
		}
		if (_player.deck.size())
		{
			// click region for deck display ("tooltip")
			_player.deck.front().rect(deck_rect(PLAYER));;
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
			int X =  move_rect(AI).x;
			int Y =  move_rect(AI).y;
			_ai.card.image()->draw(X, Y);
			_ai.card.rect(move_rect(AI));
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
			fl_font(FL_HELVETICA, w() / 42);
			fl_color(FL_BLUE);
			char buf[20];
			snprintf(buf, sizeof(buf), "%d", _player.score);
			Util::draw_string(buf, w() - fl_width(buf), h() - fl_descent());
		}
		if (_ai.score && (_ai.display_score | ::debug))
		{
			fl_font(FL_HELVETICA, w() / 42);
			fl_color(FL_BLUE);
			char buf[20];
			snprintf(buf, sizeof(buf), "%d", _ai.score);
			Util::draw_string(buf, w() - fl_width(buf), fl_height() - fl_descent());
		}
	}

	void draw_version()
	{
		fl_font(FL_HELVETICA, _CH / 20);
		fl_color(FL_YELLOW);
		char buf[30];
		snprintf(buf, sizeof(buf), " v%s", VERSION);
		Util::draw_string(buf, 0, fl_height() - fl_descent());
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

	void draw_animated_trick()
	{
		_back.image()->draw(_animate_xy.first - _CW / 2, _animate_xy.second - _CH / 2);
	}

	void draw_animated_move()
	{
		int X = _animate_xy.first - _CW / 2;
		int Y = _animate_xy.second - _CH / 2;
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
		fl_color(FL_GREEN);
		int X, Y, W, H;
		cards_rect(PLAYER).get(X, Y, W, H);
		fl_rect(X, Y, W, H);
		cards_rect(AI).get(X, Y, W, H);
		fl_rect(X, Y, W, H);

		move_rect(PLAYER).get(X, Y, W, H);
		fl_rect(X, Y, W, H);
		move_rect(AI).get(X, Y, W, H);
		fl_rect(X, Y, W, H);

		deck_rect(PLAYER).get(X, Y, W, H);
		fl_rect(X, Y, W, H);
		deck_rect(AI).get(X, Y, W, H);
		fl_rect(X, Y, W, H);

		pack_rect().get(X, Y, W, H);
		fl_rect(X, Y, W, H);

		change_rect().get(X, Y, W, H);
		fl_rect(X, Y, W, H);

		gamebook_rect().get(X, Y, W, H);
		fl_rect(X, Y, W, H);
	}

	void draw()
	{
		// measure a "standard card"
		int W = w() / 8;
		int H = 1.5 * W;
		if (_CW != W || _CH != H)
		{
			DBG("new card size: " << W << "x" << H << "\n");
		}
		_CW = W;
		_CH = H;
		_card_template.set_pixel_size(_CW, _CH);
		if (_ai.message != AI_SLEEP)
		{
			Fl::remove_timeout(cb_sleep, this);
			Fl::add_timeout(20.0, cb_sleep, this);
		}
		draw_table();
		draw_gamebook();
		if (_game.trump != NO_SUITE)
			draw_suite_symbol(_game.trump, w() / 3 - _CW / 4, h() - h() / 2 + _CH / 2 + _CH / 5);
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
		if (!_disabled && _redeal_button && _redeal_button->visible() && !::debug)
			_redeal_button->draw();
		draw_version();
		if (::debug >= 3)
			draw_debug_rects();

		draw_grayout();
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
		_game.cards.check();
	}

	void onCmd()
	{
		DBG("Your command: '" << _cmd << "'\n")
		if (_cmd.find("debug") == 0)
			debug();
		else if (_cmd.find("error=") == 0)
			error_message((Message)atoi(_cmd.substr(6).c_str()));
		else if (_cmd.find("player_message=") == 0)
		{
			Message m = (Message)atoi(_cmd.substr(15).c_str());
			player_message(m);
		}
		else if (_cmd.find("ai_message=") == 0)
		{
			Message m = (Message)atoi(_cmd.substr(11).c_str());
			ai_message(m);
		}
		else if (_cmd.find("message=") == 0)
		{
			fl_message_font_ = FL_COURIER;
			fl_message_size_ = h() / 40;
			Message m = (Message)atoi(_cmd.substr(8).c_str());
			Fl::add_timeout(0., [](void *d_) { (static_cast<Deck *>(d_))->redraw();	}, this);
			if (m == YOU_WIN)
			{
				show_win_msg();
			}
			else
			{
				DBG("fl_alert(" << Util::message(m) << ")\n");
				fl_message_position(x() + w() / 2, y() + h() / 2, 1);
				fl_alert("%s", Util::message(m).c_str());
			}
		}
		else if (_cmd.find("gb=") == 0)
		{
			std::string args = _cmd.substr(3);
			if (args.empty())
				_game.book.clear();
			else if(args.size() >= 3)
			{
				auto first = atoi(args.c_str());
				auto second = atoi(&args[2]);
				_game.book.push_back(std::make_pair(first, second));
			}
		}
		else if (_cmd.find("cip") == 0)
		{
			OUT(suite_symbol(HEART) << ": " << _engine.cards_in_play(HEART) << " (" << _engine.max_cards_player(HEART) << ")\n");
			OUT(suite_symbol(SPADE) << ": " << _engine.cards_in_play(SPADE) << " (" << _engine.max_cards_player(SPADE) << ")\n");
			OUT(suite_symbol(DIAMOND) << ": " << _engine.cards_in_play(DIAMOND) << " (" << _engine.max_cards_player(DIAMOND) << ")\n");
			OUT(suite_symbol(CLUB) << ": " << _engine.cards_in_play(CLUB) << " (" << _engine.max_cards_player(CLUB) << ")\n");
			OUT("max_trumps_player: " << _engine.max_trumps_player() << "\n");
		}
		else if (_cmd == "help")
		{
			OUT("debug/error/message/gb/cip\n");
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
		if (_game.cards.size() == 20 || _game.cards.size() == 10)
			dump_cards(_game.cards, "Deck");
		LOG("AI cards: " << _ai.cards);
		LOG("\tscore: " << _ai.score << " pending: " << _ai.pending);
		LOG("\t20/40: ");
		for ([[maybe_unused]]auto s : _ai.s20_40)
			LOG(suite_symbol(s));
		LOG("\n");

		LOG("PL cards: " << _player.cards);
		LOG("\tscore: " << _player.score << " pending: " << _player.pending);
		LOG("\t20/40: ");
		for ([[maybe_unused]]auto s : _player.s20_40)
			LOG(suite_symbol(s));
		LOG("\n");
	}

	void init()
	{
		collect();
		player_message(NO_MESSAGE);
		ai_message(NO_MESSAGE);
		error_message(NO_MESSAGE);
		_game.closed = NOT;
		_game.marriage = NO_MARRIAGE;
		_game.trump = NO_SUITE;
		_player.s20_40.clear();
		_ai.s20_40.clear();
		_game.move = PLAYER;
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
		_game.cards.shuffle();
		assert(_game.cards.size() == 20);
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
		std::string m(Util::message(YOU_WIN));
		fl_message_icon()->box(FL_NO_BOX);
#ifndef WIN32
		fl_message_icon_label("🏆");
#else
		static Fl_SVG_Image icon((Util::homeDir() + "rsc/" + "1f3c6.svg").c_str());
		if (icon.w() > 0 && icon.h() > 0)
		{
			icon.normalize();
			icon.scale(fl_message_icon()->w(), fl_message_icon()->h(), 1, 1);
			fl_message_icon()->image(&icon);
			fl_message_icon()->align(FL_ALIGN_IMAGE_BACKDROP);
			fl_message_icon_label("");
		}
#endif
		Fl::add_timeout(0.0, [](void *d_)
		{
			Fl_Window *win = Fl::first_window();
			if (win == nullptr) return;
			auto &m = *(static_cast<const std::string *>(d_));
			Fl_Box *b = static_cast<Fl_Box *>(win->child(0));
			if (b->label() == nullptr || std::string(b->label()) != m) return;
			std::string s;
			const char *p = m.c_str();
			while (*p)
			{
				int len = fl_utf8len1(*p);
				std::string c = m.substr(p - m.c_str(), len);
				p += len;
				if (c == "♥" || c == "♦" || c == "\n")
					s.append(c);
				else
					s.push_back(' ');
			}
			Fl_Box *box = new Fl_Box(b->x(), b->y(), b->w(), b->h());
			box->color(b->color());
			box->labelcolor(FL_RED);
			box->labelfont(b->labelfont());
			box->labelsize(b->labelsize());
			box->align(b->align());
			box->copy_label(s.c_str());
			win->insert(*box, 99);
			win->redraw();
		}, &m);
		fl_message_position(x() + w() / 2, y() + h() / 2, 1);
		fl_alert("%s", m.c_str());
		fl_message_icon()->image(nullptr);
	}

	bool check_end_match()
	{
		int pscore = 0;
		int ascore = 0;
		for (auto s : _game.book)
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
			Util::stats("player_matches_won", std::to_string(_player.matches_won));
			bell(YOU_WIN);
			show_win_msg();
			_game.book.clear();
			redraw();
			return true;
		}
		else if (ascore >= MATCH_SCORE)
		{
			LOG("AI wins match " << ascore << ":" << pscore << "\n");
			_ai.matches_won++;
			Util::stats("ai_matches_won", std::to_string(_ai.matches_won));
			std::string m(Util::message(YOU_LOST));
			bell(YOU_LOST);
			fl_message_icon()->box(FL_NO_BOX);
#ifndef WIN32
			fl_message_icon_label("⚫");
#else
			static Fl_SVG_Image icon((Util::homeDir() + "rsc/" + "26ab.svg").c_str());
			if (icon.w() > 0 && icon.h() > 0)
			{
				icon.normalize();
				icon.scale(fl_message_icon()->w(), fl_message_icon()->h(), 1, 1);
				fl_message_icon()->image(&icon);
				fl_message_icon()->align(FL_ALIGN_IMAGE_BACKDROP);
				fl_message_icon_label("");
			}
#endif
			fl_message_position(x() + w() / 2, y() + h() / 2, 1);
			fl_alert("%s", m.c_str());
			fl_message_icon()->image(nullptr);
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
					_game.book.push_back(std::make_pair(_ai.score < 33 ? _ai.score == 0 ? 3 : 2 : 1, 0));
				}
				else
				{
					// TODO: officially the points are counted at the moment of closing
					_game.book.push_back(std::make_pair(0, _player.score < 33 ? _player.score == 0 ? 3 : 2 : 2));
				}
			}
			else
			{
				// closed by AI
				if (_ai.score >= 66)
				{
					_game.book.push_back(std::make_pair(0, _player.score < 33 ? _player.score == 0 ? 3 : 2 : 1));
				}
				else
				{
					// TODO: officially the points are counted at the moment of closing
					_game.book.push_back(std::make_pair(_ai.score < 33 ? _ai.score == 0 ? 3 : 2 : 2, 0));
				}
			}
		}
		else
		{
			// normal game (not closed)
			if (_game.move == PLAYER)
			{
				_game.book.push_back(std::make_pair(_ai.score < 33 ? _ai.score == 0 ? 3 : 2 : 1, 0));
			}
			else
			{
				_game.book.push_back(std::make_pair(0, _player.score < 33 ? _player.score == 0 ? 3 : 2 : 1));
			}
		}
	}

	int run()
	{
		Player playout(::first_to_move);
		while (Fl::first_window())
		{
			game(playout);
			cursor(FL_CURSOR_DEFAULT);
			playout = playout == PLAYER ? AI : PLAYER;
			update_gamebook();

			if (!Fl::first_window()) break;

			auto &[pscore, ascore] = _game.book.back();
			if (pscore)
				LOG("PL scores " << pscore << "\n");
			if (ascore)
				LOG("AI scores " << ascore << "\n");
			redraw();
			check_end_match();
		}
		save_config();
		save_stats();
		return 0;
	}

	void save_config() const
	{
		Util::config("cards", std::string()); // don't save cards string!
		Util::config("width", std::to_string(w()));
		Util::config("height", std::to_string(h()));
		Util::config("xpos", std::to_string(x()));
		Util::config("ypos", std::to_string(y()));
		Util::save_config();
	}

	void save_stats() const
	{
		Util::save_stats();
	}

	bool ai_wins(const std::string &log_, Message player_message_ = NO_MESSAGE)
	{
		LOG(log_);
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
		LOG(log_);
		player_message(YOUR_GAME, true);
		ai_message(ai_message_);
		_player.games_won++;
		Util::stats("player_games_won", std::to_string(_player.games_won));
		_ai.display_score = true;
		wait(2.0);
		return true;
	}

	void deal()
	{
		// 3 cards to player
		for (int i = 0; i < 3; i++)
		{
			Card c = _game.cards.front();
			_game.move == PLAYER ? _player.cards.push_front(c) : _ai.cards.push_front(c);
			_game.cards.pop_front();
		}
		// 3 cards to ai
		for (int i = 0; i < 3; i++)
		{
			Card c = _game.cards.front();
			_game.move == PLAYER ? _ai.cards.push_front(c) : _player.cards.push_front(c);
			_game.cards.pop_front();
		}
		// trump card
		Card trump = _game.cards.front();
		_game.cards.pop_front();
		_game.cards.push_back(trump); // will be the last card (_game.cards.back())
		_game.trump = trump.suite();
		LOG("trump: " << suite_symbol(_game.trump) << "\n");

		// 2 cards to player
		for (int i = 0; i < 2; i++)
		{
			Card c = _game.cards.front();
			_game.move == PLAYER ? _player.cards.push_front(c) : _ai.cards.push_front(c);
			_game.cards.pop_front();
		}
		// 2 cards to ai
		for (int i = 0; i < 2; i++)
		{
			Card c = _game.cards.front();
			_game.move == PLAYER ? _ai.cards.push_front(c) : _player.cards.push_front(c);
			_game.cards.pop_front();
		}

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
		size_t i = _engine.find(Card(JACK, _game.cards.back().suite()), _player.cards);
		if (i != NO_MOVE)
			DBG("player cards can change Jack!\n")
		i = _engine.find(Card(JACK, _game.cards.back().suite()), _ai.cards);
		if (i != NO_MOVE)
			DBG("AI cards can change Jack!\n")
	}

	void bell([[maybe_unused]]Message m_ = NO_MESSAGE) override
	{
#ifdef USE_MINIAUDIO
		std::string snd = sound[m_];
		if (snd.size())
		{
			snd = Util::homeDir() + "rsc/" + snd + ".mp3";
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

		if (_game.closed == NOT || _game.closed == AUTO)
		{
			if (_game.move == PLAYER && _player.score >= 66)
			{
				return player_wins("Player wins!\n");
			}
			else if (_game.move == AI && _ai.score >= 66)
			{
				return ai_wins("AI wins!\n");
			}
			else if (no_cards_in_play())
			{
				if (_game.move == AI)
				{
					return ai_wins("AI wins by last trick!\n");
				}
				else
				{
					// _game.move = PLAYER
					return player_wins("Player wins by last trick!\n");
				}
			}
		}
		else
		{
			// closed
			if (_game.closed == BY_PLAYER && _game.move == PLAYER && _player.score >= 66)
			{
				return player_wins("Player wins closed game!\n");
			}
			else if (_game.closed == BY_AI && _game.move == AI && _ai.score >= 66)
			{
				return ai_wins("AI wins closed game!\n");
			}
			else if (no_cards_in_play())
			{
				// closed and last trick done
				if (_game.closed == BY_PLAYER)
				{
					return ai_wins("AI wins because player closed and has not enough!\n", YOU_NOT_ENOUGH);
				}
				else
				{
					// _game.closed = BY_AI
					return player_wins("Player wins because AI closed and has not enough!\n", AI_NOT_ENOUGH);
				}
			}
		}
		return false;
	}

	void check_trick(Player move_)
	{
		_game.marriage = NO_MARRIAGE;
		if (move_ == PLAYER)
		{
			_game.move = _engine.card_tricks(_ai.card, _player.card) ? AI : PLAYER;
			if (_game.move == AI) LOG(_ai.card << " tricks " << _player.card << "\n")
			else LOG("Player card " << _player.card << " tricks AI card " << _ai.card << "\n")
		}
		else
		{
			_game.move = _engine.card_tricks(_player.card, _ai.card) ? PLAYER : AI;
			if (_game.move == PLAYER) LOG(_player.card << " tricks " << _ai.card << "\n")
			else LOG("AI card " << _ai.card << " tricks player card " << _player.card << "\n")
		}
		LOG("next move: " << (_game.move == PLAYER ? "PLAYER" : "AI") << "\n")

		_player.move_state = NONE;
		_ai.move_state = NONE;
		animate_trick();

		if (_game.move == PLAYER) // player won trick
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
		if (_game.closed == NOT)
		{
			// give cards from pack
			if (_game.cards.size())
			{
				Card c = _game.cards.front();
				_game.cards.pop_front();
				_game.move == AI ? _ai.last_drawn = c : _player.last_drawn = c;
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
				if (_game.move == AI)
					_player.cards.push_front(c);
				else
					_ai.cards.push_front(c);
			}
			assert(_player.cards.size() == _ai.cards.size());
			_player.cards.sort();
			_ai.cards.sort();
			debug();

			if (_game.cards.empty())
			{
				_game.closed = AUTO; // same rules as closing now
				LOG("*** pack cleared - end game ***\n");
			}
		}
		redraw();
	}

	static void cb_sleep(void *d_)
	{
		if (Fl::first_window() == static_cast<Deck *>(d_))
			(static_cast<Deck *>(d_))->ai_message(AI_SLEEP);
	}

	void game(Player playout_)
	{
		init();
		wait(1.0);
		_game.move = playout_;
		_player.move_state = NONE;
		_ai.move_state = NONE;
		cursor(FL_CURSOR_DEFAULT);
		_redeal_button->show();
		redraw();
		while (Fl::first_window() && (_player.cards.size() || _ai.cards.size()))
		{
			if (_game.move == PLAYER)
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
				Fl::remove_timeout(cb_sleep, this);
				ai_message(NO_MESSAGE);
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
					_game.move = AI;
				}
			}

			if (_game.move == AI)
			{
				player_message(NO_MESSAGE);
				if (check_end()) break;
				_ai.move_state = MOVING;
				ai_message(_player.move_state == NONE ? AI_LEADS : AI_TURN);
				cursor(FL_CURSOR_WAIT);
				wait(2.0);
				if (!Fl::first_window()) break;
				_engine.ai_move();

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
		if (!Fl::first_window()) return;

		Fl::remove_timeout(cb_sleep, this);
		_game.marriage = NO_MARRIAGE;
		wait(2.0);
	}

	void wait(double s_) override
	{
		if (Util::config("fast") == "1" && s_ >= 1.0)
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
		while (Fl::first_window() && _disabled)
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
		os << "    " << Util::message(MATCHES_WON) <<  _player.matches_won << " / " << _ai.matches_won;
		return os.str();
	}

	void welcome()
	{
		_welcome = new Welcome(w() / 2, h() / 4 * 3);
		_welcome->position(x() + (w() - _welcome->w()) / 2, y() + (h() - _welcome->h()) / 2);
		_welcome->stats(make_stats());
		bell(WELCOME);
		_welcome->show();
		_welcome->wait_for_expose();
		_welcome->run();
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

private:
	// Engine
	GameState _player;
	GameState _ai;
	GameData _game;
	Engine _engine;

	// UI
	Message _error_message;
	bool _disabled;
	Card _card_template;
	CardImage _back;
	CardImage _shadow;
	CardImage _outline;
	int _CW;					// card pixel width (used as "unit" for UI)
	int _CH;					// card pixel height
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
