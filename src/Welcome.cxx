//
// Part of "Schnapsen for 2" card game.
//
// (c) 2026 Christian Grabner
//
// Display the "welcome/copyright/status" screen.
//

#ifdef STANDALONE
constexpr char APPLICATION[] = "fltk-schnapsen";
#include <FL/Enumerations.H>
Fl_Font CustomFont = FL_HELVETICA;
#endif
#include "debug.h"
#include "Welcome.h"
#include "Util.h"
#include "Card.h"
#include "AnimText.h"

#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Shared_Image.H>

using enum CardSuite;
using enum CardFace;
extern std::map<CardSuite, std::string> suite_symbols;

static std::vector<std::string> load_texts(const std::string& name_)
{
	std::vector<std::string> texts;
	std::ifstream ifs(Util::rsc_dir() + name_ + "_" + Util::config("lang") + ".txt", std::ios::binary);
	std::string text;
	while (std::getline(ifs, text))
	{
		size_t count = 0;
		for (unsigned int c : text) if (c > 128) count++;
		if (count > text.size() / 2)
		{
			for (size_t i = 0; i < text.size(); i++)
				text[i] = text[i] - 64;
		}
		std::string_view t = Util::trim(text);
		if (t.empty()) continue;
		texts.push_back(std::string(t));
	}
	return texts;
};

Welcome::Welcome(int w_, int h_, bool saying_/* = true_*/) : Fl_Double_Window(w_, h_),
	_saying(nullptr)
{
	clear_border();
	set_modal();
	box(FL_UP_BOX);
	color(FL_WHITE);
	redraw_timer(this);

	static std::vector<std::string> sayings{load_texts("sayings")};
	if (sayings.size() && saying_)
		_saying = new AnimText(sayings[random() % sayings.size()], *this, 1./20);

	static std::vector<std::string> welcomes{load_texts("welcomes")};
	if (welcomes.size())
		_welcome = welcomes[random() % welcomes.size()];
}

Welcome::~Welcome()
{
	delete _saying;
	Fl::remove_timeout(redraw_timer, this);
}

void Welcome::update()
{
	redraw();
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

#include <string>
#include <vector>
#include <fstream>

void Welcome::draw()
{
	static const std::vector<CardSuite> suites = { HEART, SPADE, DIAMOND, CLUB };
	static const std::vector<CardFace> faces = { ACE, TEN, KING, QUEEN, JACK };
	static int frame = 0;

	fl_draw_box(box(), 0, 0, w(), h(), color());
	Rect r(*this, box());
	fl_push_clip(r.x - x(), r.y - y(), r.w, r.h);
	Fl_Image *border = Util::get_shared_image("border.svg", w(), h());
	fl_font(FL_COURIER_BOLD, h() / 42);
	int stat_h = fl_height() + fl_descent();

	static CardFace face = QUEEN;
	static CardSuite suite = HEART;

	// disable symbol "shower" and card change during saying animation
	if (!_saying || _saying->done())
	{
		fl_font(FL_COURIER_BOLD, h() / 7);
		for (int i = 0; i < 30; i++)
		{
			int x = random() % w();
			int y = random() % h();
			auto s = random() % suites.size();
			if (suites[s] == HEART || suites[s] == DIAMOND)
				fl_color(fl_lighter(fl_lighter(FL_RED)));
			else
				fl_color(fl_lighter(fl_lighter(FL_BLACK)));
			Util::draw_string(suite_symbols[suites[s]], x, y + fl_height());
		}

		if (++frame % 25 == 0)
		{
			face = faces[random() % faces.size()];
			suite = suites[random() % suites.size()];
		}
	}

	// draw border
	if (border != nullptr)
	{
		border->draw(0, 0);
	}

	// draw card
	Card c(face, suite);
	int W = w() / 2 - w() / 10;
	int H = 1.5 * W;
	int Y = h() / 4;
	if (Y + H > h() - stat_h - 4)
	{
		H = h() - Y - stat_h - 4;
	}
	c.image(W, H)->draw(w() / 40, Y);

	// draw (c)
	fl_font(CustomFont != FL_HELVETICA ? CustomFont : FL_HELVETICA_BOLD, w() / 10);
	fl_color(FL_BLACK);
	static constexpr char title[] = "^rF^BL^rT^BK^r S^BC^rH^BN^rA^BP^rS^BE^rN^B";
	Util::draw_string(w(), title, 0, h() / 7, true);

	Y = h() / 7 + h() / 14;
	fl_color(FL_BLUE);
	fl_font(FL_HELVETICA_BOLD, w() / 26);
	static constexpr char cr[] = "^r(c) 2025-2026^B Christian Grabner^. <wcout@gmx.net>";
	Util::draw_string(w(), cr, 0, Y);

	// draw message box
	fl_color(FL_BLACK);
	fl_font(FL_HELVETICA_BOLD, h() / 16);
	int X = w() / 40 + W;
	std::string welcome = _welcome.size() ? _welcome : Util::message(WELCOME);
	class Formatter : public Fl_Box
	{
	public:
		Formatter(int x_, int y_, int w_, int h_) : Fl_Box(x_, y_, w_, h_) {}
		void draw() override { Fl_Box::draw(); } // allows to access protected Fl_Box::draw()
	};
	int margin = w() / 20;
	if (_saying)
	{
		for (Fl_Font ft = FL_FREE_FONT; ft < Fl::set_fonts(); ft++)
		{
			std::string name = Fl::get_font_name(ft);
//			if (name.find("Caveat Bold") != std::string::npos)
			if (name.find("Caveat") != std::string::npos) // NOTE: WIN11 lists font without "Bold"???
			{
				fl_font(ft, fl_size());
				break;
			}
		}
	}
	Formatter f(X  + margin, Y, w() - X - 2 * margin, h() - Y - stat_h);
	f.color(fl_color());
	f.labelfont(fl_font());
	f.labelsize(fl_size());
	f.label(_saying ? _saying->text().c_str() : welcome.c_str());
	f.align(FL_ALIGN_CENTER | FL_ALIGN_WRAP); // show text centered and auto wrapped
	f.draw();

	// draw stats
	fl_font(FL_COURIER_BOLD, h() / 42);
	fl_draw_box(FL_FLAT_BOX, 0, h() - stat_h, w(), stat_h, fl_lighter(fl_lighter(FL_YELLOW)));
	fl_color(FL_BLACK);
	Util::draw_string(w(), _stats, 0, h() - stat_h + fl_height() - 2);
	fl_pop_clip();
}

#ifdef STANDALONE
#undef STANDALONE
#include "CardImage.cxx"
#include "Card.cxx"
#include "Util.cxx"
#include "AnimText.cxx"
int main(int argc_, char *argv_[])
{
	fl_register_images();
	Util::load_config();
	Welcome *w = new Welcome(800, 800, argc_ > 1);
	w->border(1);
	w->resizable(w);
	w->stats("Game statistics...");
	Util::run(*w);
}
#endif
