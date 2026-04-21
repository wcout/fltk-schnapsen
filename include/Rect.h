#pragma once
#include <utility> // std::pair()
#include <FL/Fl_Widget.H>

struct Rect
{
	Rect(int x_, int y_, int w_, int h_) : x(x_), y(y_), w(w_), h(h_) {}
	int x;
	int y;
	int w;
	int h;
	explicit Rect (const Fl_Widget& wgt_, Fl_Boxtype box_ = FL_NO_BOX) :
		x(wgt_.x() + Fl::box_dx(box_)),
		y(wgt_.y() + Fl::box_dy(box_)),
		w(wgt_.w() - Fl::box_dw(box_)),
		h(wgt_.h() - Fl::box_dh(box_))
	{}
	Rect() : x(0), y(0), w(0), h(0) {}
	Rect(int w_, int h_) : x(0), y(0), w(w_), h(h_) {}
	bool includes(int x_, int y_) const { return x_ >= x && y_ >= y && x_ < x + w && y_ < y + h; }
	Rect inset(int d_) const { return Rect(x + d_, y + d_, w - 2 * d_, h - 2 * d_); }
	Rect center() const { return Rect(x + w / 2, y + h / 2, 1, 1); }
	Rect& get(int &x_, int &y_, int &w_, int &h_) { x_ = x; y_ = y; w_ = w; h_ = h; return *this; }
	int baseline() const { return center().y + fl_height() / 2 - fl_descent(); }
	bool defined() { return w > 0 && h > 0; }
};
