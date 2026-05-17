#pragma once

#include <FL/Fl_Double_Window.H>
#include <string>

class Welcome : public Fl_Double_Window
{
public:
	Welcome(int w_, int h_);
	~Welcome();
	int handle(int e_) override;
	void draw() override;
	void stats(std::string stats_) { _stats = stats_; }
private:
	static void redraw_timer(void *d_);
	std::string _stats;
};
