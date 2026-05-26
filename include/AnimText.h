#pragma once

#include <string>

class UI;

class AnimText
{
public:
	AnimText(const std::string &m_, UI &ui_, double speed_ = 1./60);
	~AnimText();
	static void cb_anim(void *d_) { (static_cast<AnimText *>(d_))->on_animate(); }
	void on_animate();
	std::string &text();
	bool done() const;
private:
	std::string _m;
	UI &_ui;
	double _speed;
	size_t _display_len;
	bool _needs_update;
	std::string _res;
};
