#pragma once

#include <FL/Fl_Double_Window.H>
#include <string>

class Rect;
class CardImage;

class Selector : public Fl_Double_Window
{
public:
	Selector(int w_, int h_);
	int handle(int e_) override;
	void draw() override;
private:
	size_t _card_index;
	size_t _back_index;
	std::vector<std::string> _cardsets;
	std::vector<std::string> _cardbacks;
	Rect _card_rect;
	Rect _back_rect;
	CardImage _card;
	CardImage _back;
	int _face;
	int _suite;
};
