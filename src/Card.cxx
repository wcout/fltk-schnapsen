#include "Card.h"
#include "debug.h"
#include "Util.h"

using enum CardSuite;
using enum CardFace;

std::map<CardFace, std::string> face_names = { {TEN, "10"}, {JACK, "jack"}, {QUEEN, "queen"}, {KING, "king"}, {ACE, "ace"} };
std::map<CardFace, std::string> face_abbrs = { {TEN, "T"}, {JACK, "J"}, {QUEEN, "Q"}, {KING, "K"}, {ACE, "A"} };
std::map<CardSuite, std::string> suite_names = { {CLUB, "clubs"}, {DIAMOND, "diamonds"}, {HEART, "hearts"}, {SPADE, "spades"} };
std::map<CardFace, int> card_value = { {TEN, 10}, {JACK, 2}, {QUEEN, 3}, {KING, 4}, {ACE, 11} };
std::map<CardSuite, int> suite_weights = { {SPADE, 4}, {HEART, 3}, {DIAMOND, 2}, {CLUB, 1} };
std::map<CardSuite, std::string> suite_symbols = { {SPADE, "♠"}, {HEART, "♥"}, {DIAMOND, "♦"}, {CLUB, "♣"} };
std::map<CardSuite, std::string> suite_symbols_image = { {SPADE, "laub"}, {HEART, "herz"}, {DIAMOND, "schelle"}, {CLUB, "eichel"} };


Card::Card() : _f(NO_FACE), _s(NO_SUITE),
	_rect(0, 0, 0, 0)
{}

/*explicit*/
Card::Card(CardFace f_, CardSuite s_) :
	_f(f_),
	_s(s_),
	_rect(0, 0, 0, 0)
{}

Card& Card::load()
{
	if (face() != NO_FACE && !_images.image(name()))
	{
		std::string pathname = Util::cardset_dir() + filename();
		DBG("load '" << pathname << "'\n");
		_images.image(name(), pathname);
	}
	return *this;
}

bool Card::operator == (const Card c_)
{

	return face() == c_.face() && suite() == c_.suite();
}

int Card::value() const
{
	return card_value[face()];
}

std::string Card::face_name() const
{
	return face_names[face()];
}

std::string Card::face_abbr() const
{
	return face_abbrs[face()];
}

std::string Card::suite_name() const
{
	return suite_names[suite()];
}

int Card::suite_weight() const
{
	return suite_weights[suite()];
}

std::string Card::suite_symbol() const
{
	return suite_symbols[suite()];
}

/*static*/
std::string suite_symbol(CardSuite suite_)
{
	return suite_symbols[suite_];
}

/*static*/
std::string suite_symbol_image(CardSuite suite_)
{
	return suite_symbols_image[suite_];
}

bool Card::is_black_suite() const
{
	return _s == SPADE || _s == CLUB;
}

/*virtual*/
std::ostream& Card::printOn(std::ostream &os_) const
{
	std::string abbr = face_abbr();
	os_ << abbr << suite_symbol();
	return os_;
}

Card& Card::set_pixel_size(int w_, int h_)
{
	_images.set_pixel_size(w_, h_);
	return *this;
}

inline std::ostream &operator << (std::ostream &os_, const Card &c_)
{
	return c_.printOn(os_);
}


#ifdef STANDALONE
// Compile: fltk-config --use-images --compile src/Card.cxx -std=c++20 -Iinclude -DSTANDALONE
#include "system.h"
const char APPLICATION[] = "Card-Test";
#include <iostream>
#include "CardImage.cxx"
#include "Util.cxx"

using enum CardSuite;
using enum CardFace;


int main()
{
	Card test(QUEEN, HEART);
	std::cout << test.face_name() << "/" << test.suite_name() << ": " << test.name() << "\n";
	std::cout << "value: " << test.value() << "\n";
}
#endif
