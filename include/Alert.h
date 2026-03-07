#pragma once

#include "Rect.h"

#include <FL/Fl_Double_Window.H>

class Alert : public Fl_Double_Window
{
public:
	Alert(const char *msg_, const char *title_=nullptr);
	int handle(int e_);
	Alert& border(size_t border_size_, Fl_Color border_color_);
	Alert& message(const char *text_);
	Alert& set_bg_image(const char *img_);
	Alert& center_on(const Rect &r_);
	void run();
private:
	class Canvas;
	Canvas *_msg;
};
