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
	Rect (const Fl_Widget& wgt_, Fl_Boxtype box_ = FL_NO_BOX) :
		x(wgt_.x() + Fl::box_dx(box_)),
		y(wgt_.y() + Fl::box_dy(box_)),
		w(wgt_.w() - Fl::box_dw(box_)),
		h(wgt_.h() - Fl::box_dh(box_))
	{}
	bool includes(int x_, int y_) const { return x_ >= x && y_ >= y && x_ < x + w && y_ < y + h; }
	std::pair<int, int> center() const { return std::make_pair(x + w / 2, y + h / 2); }
	Rect& get(int &x_, int &y_, int &w_, int &h_) { x_ = x; y_ = y; w_ = w; h_ = h; return *this; }
};
