#pragma once

#include <string>
#include <vector>
#include <deque>
#include <utility>

// score points count down for "bummerl"
constexpr int MATCH_SCORE = 7; // or 11?

class GameBook : public std::vector<std::pair<int, int>>
{
public:
	GameBook() : _current(0) {}
	int player_score() const;
	int ai_score() const;
	std::string str() const;
	std::vector<std::pair<int, int>> to_value(const std::string &str_);
	GameBook& from_str(const std::string &str_);
	GameBook& history(const std::string &history_);
	void draw(int x_, int y_, int w_, int h_);
	void reset_current() { _current = 0; }
	GameBook& next_current();
private:
	std::deque<std::vector<std::pair<int, int>>> _history;
	size_t _current;
};
