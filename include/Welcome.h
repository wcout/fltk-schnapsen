#pragma once

#include "UI.h"

#include <FL/Fl_Double_Window.H>
#include <string>

class AnimText;

class Welcome : public Fl_Double_Window, public UI
{
public:
	Welcome(int w_, int h_, bool saying_ = true);
	~Welcome();
	int handle(int e_) override;
	void draw() override;
	void stats(std::string stats_) { _stats = stats_; }
	virtual void update() override;
private:
	static void redraw_timer(void *d_);
	std::string _stats;
	AnimText *_saying;
	std::string _welcome;
};
