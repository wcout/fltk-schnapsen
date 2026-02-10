#include "Welcome.h"
#include "Util.h"

#include <FL/Fl.H>
#include <FL/fl_draw.H>

Welcome::Welcome(int w_, int h_) : Fl_Double_Window(w_, h_)
{
	clear_border();
	set_modal();
	box(FL_UP_BOX);
	color(FL_WHITE);
	redraw_timer(this);
}

Welcome::~Welcome()
{
	Fl::remove_timeout(redraw_timer, this);
}

/*static*/
void Welcome::redraw_timer(void *d_)
{
	(static_cast<Welcome *>(d_))->redraw();
	Fl::add_timeout(0.2, redraw_timer, d_);
}

int Welcome::handle(int e_)
{
	if (e_ == FL_NO_EVENT) return 1;
	if (e_ == FL_PUSH || e_ == FL_KEYDOWN)
	{
		hide();
		return 1;
	}
	return Fl_Double_Window::handle(e_);
}

void Welcome::draw()
{
	fl_draw_box(box(), 0, 0, w(), h(), color());
	Rect r(*this, box());
	fl_push_clip(r.x - x(), r.y - y(), r.w, r.h);
	fl_font(FL_HELVETICA_BOLD, h() / 7);
	for (int i = 0; i < 30; i++)
	{
		static const std::vector<CardSuite> suites = { HEART, SPADE, DIAMOND, CLUB };
		int x = random() % w();
		int y = random() % h();
		auto s = random() % suites.size();
		if (suites[s] == HEART || suites[s] == DIAMOND)
			fl_color(fl_lighter(fl_lighter(FL_RED)));
		else
			fl_color(fl_lighter(fl_lighter(FL_BLACK)));
		Util::draw_string(suite_symbols[suites[s]], x, y + fl_height());
	}
	fl_font(FL_COURIER_BOLD, h() / 42);
	int stat_h = fl_height() + fl_descent();

	Card c(QUEEN, HEART);
	int W = w() / 2 - w() / 10;
	int H = 1.5 * W;
	int Y = h() / 4;
	if (Y + H > h() - stat_h - 4)
		H = h() - Y - stat_h - 4;
	c.image(W, H)->draw(w() / 40, Y);
	fl_font(FL_HELVETICA_BOLD, w() / 10);
	fl_color(FL_BLACK);
	static constexpr char title[] = "^rF^BL^rT^BK^r S^BC^rH^BN^rA^BP^rS^BE^rN^B";
	Util::draw_color_text(title, (w() - fl_width("FLTK SCHNAPSEN")) / 2, h() / 7);
	fl_color(FL_BLUE);
	fl_font(FL_HELVETICA_BOLD, w() / 26);
	static constexpr char cr[] = "(c) 2025 Christian Grabner <wcout@gmx.net>";
	Util::draw_string(cr, (w() - fl_width(cr)) / 2, h() / 7 + h() / 14);
	fl_color(FL_BLACK);
	fl_font(FL_HELVETICA_BOLD, h() / 16);
	Util::draw_string(Util::message(WELCOME), w() / 2 + w() / 60, h() / 2);
	// draw stats
	fl_font(FL_COURIER_BOLD, h() / 42);
	fl_draw_box(FL_FLAT_BOX, 0, h() - stat_h, w(), stat_h, fl_lighter(fl_lighter(FL_YELLOW)));
	fl_color(FL_BLACK);
	Util::draw_string(_stats, (w() - fl_width(_stats.c_str())) / 2, h() - stat_h + fl_height() - 2);
	fl_pop_clip();
}

void Welcome::run()
{
	while (shown())
	{
		Fl::wait();
	}
	delete this;
}
