#include "Alert.h"
#include "Util.h"

#include <FL/Fl.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_Anim_GIF_Image.H>
#include <FL/Fl_Box.H>
#include <FL/fl_draw.H>

class Alert::Canvas : public Fl_Box
{
public:
	Canvas(int x_, int y_, int w_, int h_) : Fl_Box(x_, y_, w_, h_),
		_border_size(fl_message_size_ / 2),
		_border_color(FL_BACKGROUND_COLOR)
	{
		labelsize(16);
		labelfont(FL_COURIER_BOLD);
	}
	~Canvas()
	{
		if (image())
			image()->release();
	}
	void draw()
	{
		fl_color(FL_BLACK);
		Fl_Box::draw_box();
		fl_font(labelfont(), labelsize());
		if (label())
			Util::draw_string(label(), x() + border_size(), y() + border_size() + fl_height() - fl_descent());
		if (_border_size && _border_color != FL_BACKGROUND_COLOR)
		{
			fl_color(_border_color);
			fl_line_style(FL_SOLID, _border_size);
			fl_rect(x(), y(), w(), h());
			fl_line_style(0);
		}
	}
	Canvas& set_bg_image(const char *name_)
	{
		if (!name_ || !name_[0]) return *this;
		Fl_GIF_Image::animate = true;
		Fl_Shared_Image *i = Fl_Shared_Image::get(name_);
		if (!i || !i->image()) return *this;
		i->scale(w(), h(), 1, 1);
		i->color_average(FL_WHITE, 0.5);
		Fl_Anim_GIF_Image *anim = dynamic_cast<Fl_Anim_GIF_Image *>((Fl_Image *)i->image());
		if (anim)
			anim->canvas(this, Fl_Anim_GIF_Image::DONT_RESIZE_CANVAS);
		image(i);
		return *this;
	}
	Canvas& border(size_t border_size_, Fl_Color border_color_)
	{
		_border_size = border_size_;
		_border_color = border_color_;
		return *this;
	}
	size_t border_size() const { return _border_size; }
private:
	size_t _border_size;
	Fl_Color _border_color;
};

Alert::Alert(const char *msg_, const char *title_/* = nullptr*/) :
	Fl_Double_Window(400, 300, (title_ ? title_ : Util::message(TITLE).c_str())),
	_msg(nullptr)
{
	color(FL_BACKGROUND_COLOR);
	_msg = new Canvas(0, 0, w(), h());
	_msg->labelcolor(FL_BLACK);
	_msg->labelsize(fl_message_size_);
	_msg->labelfont(fl_message_font_);
	_msg->align(FL_ALIGN_IMAGE_BACKDROP);
	end();
	if (msg_)
		message(msg_);
	set_modal();
}

int Alert::handle(int e_)
{
	if (e_ == FL_NO_EVENT) return 1;
	if (e_ == FL_KEYDOWN || e_ == FL_PUSH)
	{
		hide();
		return 1;
	}
	return Fl_Double_Window::handle(e_);
}

Alert& Alert::border(size_t border_size_, Fl_Color border_color_)
{
	_msg->border(border_size_, border_color_);
		return *this;
}

Alert& Alert::message(const char *text_)
{
	_msg->copy_label(text_);
	fl_font(_msg->labelfont(), _msg->labelsize());
	int W = 0, H = 0;
	Util::string_size(text_, W, H);
	size(W + 2 *_msg->border_size(), H + 2 * _msg->border_size());
	_msg->size(w(), h());
	return *this;
}

Alert& Alert::set_bg_image(const char *img_)
{
	_msg->set_bg_image(img_);
	return *this;
}

Alert& Alert::center_on(const Rect& r_)
{
	position(r_.center().x - w() / 2, r_.center().y - h() / 2);
	return *this;
}

void Alert::run()
{
	show();
	wait_for_expose();
	while (shown())
	{
		Fl::wait();
	}
	delete this;
}
