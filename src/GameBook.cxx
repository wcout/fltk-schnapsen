#include "GameBook.h"
#include "Util.h"
#include <sstream>

#include <FL/fl_draw.H>

int GameBook::player_score() const
{
	int score = 0;
	for (const auto &s : *this)
		score += s.first;
	return score;
}

int GameBook::ai_score() const
{
	int score = 0;
	for (const auto &s : *this)
		score += s.second;
	return score;
}

std::string GameBook::str() const
{
	std::ostringstream os;
	for (auto &[player, ai] : *this)
	{
		os << player << " " << ai << ",";
	}
	std::string s(os.str());
	s.pop_back(); // remove last ','
	return s;
}

GameBook& GameBook::from_str(const std::string &str_)
{
	clear();
	std::vector<std::pair<int, int>> value = to_value(str_);
	for (const auto &v : value)
		push_back(v);
	return *this;
}

std::vector<std::pair<int, int>> GameBook::to_value(const std::string &str_)
{
	std::stringstream ss(str_);
	std::vector<std::pair<int, int>> v;
	while (!ss.fail())
	{
		int player_score = -1;
		int ai_score = -1;
		unsigned char c;
		ss >> player_score;
		ss >> ai_score;
		ss >> c;	// ','
		if (player_score < 0 || ai_score < 0) break;
//		LOG("\t" << player_score << " | " << ai_score << "\n");
		v.push_back(std::make_pair(player_score, ai_score));
	}
	return v;
}

GameBook& GameBook::history(const std::string &history_)
{
	size_t pos;
	std::string h(history_);
	_history.clear();
	while ((pos = h.find(';')) != std::string::npos)
	{
		std::string hs = h.substr(0, pos);
		h.erase(0, pos + 1);
//		LOG("history: '" << hs << "'" << "\n");
		_history.push_front(to_value(hs));
	}
//	LOG("history: '" << h << "'" << "\n");
	if (h.size())
		_history.push_front(to_value(h));
	return *this;
}

GameBook& GameBook::next_current()
{
	_current++;
	if (_current > _history.size())
	{
		_current = 0;
	}
	return *this;
}

void GameBook::draw(int x_, int y_, int w_, int h_)
{
	int X(x_);
	int Y(y_);
	int W(w_);
	int H(h_);

	fl_color(_current == 0 ? fl_lighter(fl_lighter(FL_YELLOW)) : GRAY);
	fl_rectf(X, Y, W, H);
	fl_color(_current == 0 ? GRAY : FL_BLACK);
	fl_rect(X, Y, W, H);
	fl_color(FL_BLACK);
	fl_font(FL_COURIER, H / 14);
	X += W / 20;
	Y += fl_descent() + fl_height();
	Util::draw_color_text(Util::message(GAMEBOOK), X, Y, text_colors);
	fl_line_style(FL_SOLID, 2);
	fl_line(X, Y + fl_descent(), X + W - W / 10, Y + fl_descent());
	Y += H / 10;
	Util::draw_color_text(Util::message(GB_HEADLINE), X, Y);
	int h = H - H / 5;
	fl_line_style(FL_SOLID, 1);
	int w = W - W / 10;
	fl_line(X, Y + fl_descent(), X + w, Y + fl_descent());
	fl_line(X + w / 2, Y - fl_height(), X + w / 2, Y + h - fl_descent());
	Y += fl_descent();

	int player_score = 0;
	int ai_score = 0;

	auto draw_score = [&](std::pair<int, int> s) -> void
	{
		char buf[50];
		char pbuf[20];
		char abuf[20];
		if (MATCH_SCORE - player_score <= 0 || MATCH_SCORE - ai_score <= 0) return;
		if (!s.first && !s.second)
		{
			snprintf(pbuf, sizeof(pbuf), "%d", MATCH_SCORE - player_score);
			snprintf(abuf, sizeof(abuf), "%d", MATCH_SCORE - ai_score);
		}
		else
		{
			snprintf(pbuf, sizeof(pbuf), "%d", MATCH_SCORE - player_score);
			if (!s.first || pbuf[0] == '0') pbuf[0] = '-';
			snprintf(abuf, sizeof(abuf), "%d", MATCH_SCORE - ai_score);
			if (!s.second || abuf[0] == '0') abuf[0] = '-';
		}
		snprintf(buf, sizeof(buf),"  %2s      %2s", pbuf, abuf);
		Y += H / 12;
		Util::draw_color_text(buf, X, Y);
	};

	std::vector<std::pair<int, int>> *value = this;
	if (_current > 0)
	{
		value = &_history[_current - 1];
	}
	// limit display to last 8 scores
	size_t first = value->size() > 8 ? value->size() - 8 : 0;
	if (first == 0)
		draw_score(std::make_pair(0, 0));
	for (size_t i = 0; i < value->size(); i++)
	{
		auto s = value->at(i);
		player_score += s.first;
		ai_score += s.second;
		if (i < first) continue;
		draw_score(s);
	}
	if (ai_score >= MATCH_SCORE || player_score >= MATCH_SCORE)
	{
		// draw "bummerl"
		char buf[40];
		Y += H / 12;
		snprintf(buf, sizeof(buf),"   %s       %s",
			(ai_score >= MATCH_SCORE ? "●" : " "), (player_score >= MATCH_SCORE ? "●" : " "));
		Util::draw_color_text(buf, X, Y);
	}
}
