#pragma once

#include "messages.h"

class UI
{
public:
	UI() {}
	virtual void update() {}
	virtual void animate_ai_move() {}
	virtual void animate_change([[maybe_unused]]bool from_hand_ = false) {}
	virtual void wait([[maybe_unused]]double s_) {}
	virtual void ai_message([[maybe_unused]]Message m_, [[maybe_unused]]bool bell_ = false) {}
	virtual void bell([[maybe_unused]]Message m_ = NO_MESSAGE) {}
};
