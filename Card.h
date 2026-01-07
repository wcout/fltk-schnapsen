#pragma once

#include "Rect.h"
#include "CardImage.h"
#include <map>

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

class Card
{
public:
	Card();
	explicit Card(CardFace f_, CardSuite s_);
	Card& load();
	Fl_RGB_Image *image(int w_ = 0, int h_ = 0) { load(); return _images.image(name(), w_, h_); }
	Fl_RGB_Image *quer_image() { load(); return _images.quer_image(name()); }
	Fl_RGB_Image *skewed_image() { load(); return _images.skewed_image(name()); }
	Card &rect(const Rect &rect_) { _rect = rect_; return *this; }
	CardSuite suite() const { return _s; }
	CardFace face() const { return _f; }
	const Rect &rect() const { return _rect; }
	int value() const;
	std::string face_name() const;
	std::string face_abbr() const;
	std::string suite_name() const;
	std::string name() const { return face_name() + " of " + suite_name(); }
	std::string filename(std::string ext_ = ".svg") const { return face_name() + "_of_" + suite_name() + ext_; }
	int suite_weight() const;
	std::string suite_symbol() const;
	static std::string suite_symbol(CardSuite suite_);
	static std::string suite_symbol_image(CardSuite suite_);
	bool is_black_suite() const;
	bool is_red_suite() const { return !is_black_suite(); }
	bool includes(int x_, int y_) const { return rect().includes(x_, y_); }
	bool operator == (const Card c_);
	virtual std::ostream &printOn(std::ostream &os_) const;
	Card &set_pixel_size(int w_, int h_);
private:
	CardFace _f;
	CardSuite _s;
	CardImage _images;
	Rect _rect;
};
