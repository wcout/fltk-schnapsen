#pragma once

#include <FL/Fl_Double_Window.H>

class Welcome : public Fl_Double_Window
{
public:
	Welcome(int w_, int h_);
	void run();
	void stats(std::string stats_) { _stats = stats_; }
	~Welcome();
	static void redraw_timer(void *d_);
	int handle(int e_) override;
	void draw() override;
private:
	std::string _stats;
};
