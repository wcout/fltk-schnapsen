//
// Part of "Schnapsen for 2" card game.
//
// (c) 2026 Christian Grabner
//
// Allow to select card style and background style.
//

#ifdef STANDALONE
constexpr char APPLICATION[] = "Selector";
#include <FL/Enumerations.H>
Fl_Font CustomFont = FL_HELVETICA;
#endif
#include "debug.h"
#include "Rect.h"
#include "Card.h"
#include "Selector.h"
#include "Util.h"
#include "CardImage.h"

#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include "Util.h"

using enum CardSuite;
using enum CardFace;

static const std::vector<CardSuite> suites = { HEART, SPADE, DIAMOND, CLUB };
static const std::vector<CardFace> faces = { KING, QUEEN, JACK, TEN, ACE };

Selector::Selector(int w_, int h_) : Fl_Double_Window(w_, h_),
	_card_index(0), _back_index(0), _cardsets(Card::cardsets()), _cardbacks(Card::cardbacks()),
	_face(0), _suite(0)
{
	clear_border();
	set_modal();
	box(FL_UP_BOX);
	color(FL_WHITE);
	DBG(_cardsets.size() << " cardsets\n");
	DBG(_cardbacks.size() << " cardbacks\n");
	// try to start with current style
	for (size_t i = 0; i < _cardsets.size(); i++)
	{
		if (_cardsets[i] == Util::config("cardset"))
		{
			_card_index = i;
			break;
		}
	}
	for (size_t i = 0; i < _cardbacks.size(); i++)
	{
		if (_cardbacks[i] == Util::config("cardback"))
		{
			_back_index = i;
			break;
		}
	}
}

int Selector::handle(int e_)
{
	if (e_ == FL_NO_EVENT) return 1;
	if (e_ == FL_PUSH)
	{
		if (_card_rect.includes(Fl::event_x(), Fl::event_y()))
		{
			_card_index++;
			if (_card_index >= _cardsets.size())
				_card_index = 0;
			redraw();
		}
		else if (_back_rect.includes(Fl::event_x(), Fl::event_y()))
		{
			_back_index++;
			if (_back_index >= _cardbacks.size())
				_back_index = 0;
			redraw();
		}
		return 1;
	}
	if (e_ == FL_KEYDOWN)
	{
		int key = Fl::event_key();

		// Switch card style with left/right
		if (key == FL_Right && _card_index + 1 != _cardsets.size())
		{
			_card_index++;
			redraw();
		}
		if (key == FL_Left && _card_index > 0)
		{
			_card_index--;
			redraw();
		}
		// Switch back style with up/down
		if (key == FL_Down && _back_index + 1 != _cardbacks.size())
		{
			_back_index++;
			redraw();
		}
		if (key == FL_Up && _back_index > 0)
		{
			_back_index--;
			redraw();
		}
		// Switch suite/face with page up/page down
		if (key == FL_Page_Up)
		{
			_suite++;
			if (_suite >= (int)suites.size())
				_suite = 0;
			redraw();
		}
		if (key == FL_Page_Down)
		{
			_face++;
			if (_face >= (int)faces.size())
				_face = 0;
			redraw();
		}
		if (key == FL_Enter || key == ' ')
		{
			DBG("store selection: " << _cardsets[_card_index] << " / " << _cardbacks[_back_index] << "\n");
			Util::config("cardback", _cardbacks[_back_index]);
			Util::config("cardset", _cardsets[_card_index]);
		}
		else if (key != FL_Escape)
		{
			return 1;
		}
		hide();
		return 1;
	}
	return Fl_Double_Window::handle(e_);
}

void Selector::draw()
{
	fl_draw_box(box(), 0, 0, w(), h(), color());
	Rect r(*this, box());
	fl_push_clip(r.x - x(), r.y - y(), r.w, r.h);

	fl_font(CustomFont != FL_HELVETICA ? CustomFont : FL_HELVETICA_BOLD, w() / 16);
	fl_color(FL_BLACK);
	std::string title = Util::message(SELECTION);
	Util::draw_string(title, (w() - Util::string_width(title)) / 2, h() / 7, true);

	fl_font(FL_COURIER, w() / 40);
	int dh = fl_height() / 2 + fl_height(); // space for bottom line
	Card c(faces[_face], suites[_suite]);
	int W = w() / 2 - w() / 10;
	int H = 1.5 * W;
	int Y = h() / 5;
	if (Y + H > h() - dh - 4)
	{
		H = h() - Y - dh - 4;
	}
	_card.image("card", Util::cardset_dir(_cardsets[_card_index]) + c.filename());
	int X = w() / 40;
	_card.image("card", W, H)->draw(X, Y);
	_card_rect = Rect(X, Y, W, H);

	_back.image("back", Util::home_dir() + Card::cardDir + "/back/" + _cardbacks[_back_index]);
	X = w() - W - w() / 40;
	_back.image("back", W, H)->draw(X, Y);
	_back_rect = Rect(X, Y, W, H);

	std::string accept = Util::message(ACCEPT);
	Util::draw_string(accept, (w() - Util::string_width(accept)) / 2, h() - 4 - fl_height() / 2);
	fl_pop_clip();
}

#ifdef STANDALONE
#undef STANDALONE
#include "CardImage.cxx"
#include "Card.cxx"
#include "Util.cxx"
int main()
{
	Selector *s = new Selector(800, 800);
	s->border(1);
	Util::run(*s);
}
#endif
