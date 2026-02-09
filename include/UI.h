#pragma once

#include "messages.h"

enum class Player;

class UI
{
public:
	UI() : _playing(true) {}
	virtual void update() {}
	virtual void animate_move() {}
	virtual void animate_shuffle() {}
	virtual void animate_trick() {}
	virtual void animate_deal([[maybe_unused]] Player player_) {}
	virtual void animate_change([[maybe_unused]]bool from_hand_ = false) {}
	virtual void wait([[maybe_unused]]double s_) {}
	virtual void message([[maybe_unused]]Message m_, [[maybe_unused]]bool bell_ = false) {}
	virtual void bell([[maybe_unused]]Message m_ = NO_MESSAGE) {}
	virtual void prepare_game() {}
	virtual void player_move() {}
	virtual void ai_move() {}
	virtual void show_win_msg() {}
	virtual void show_lost_msg() {}
	virtual bool playing() { return _playing; }
private:
	bool _playing;
};
