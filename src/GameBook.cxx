#include "GameBook.h"
#include "Util.h"
#include "Rect.h"
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
		v.emplace_back(player_score, ai_score);
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
	LOG("GameBook history size: " << _history.size() << "\n");
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

void GameBook::draw(Rect r_)
{
	int X, Y, W, H;
	r_.get(X, Y, W, H);

	// Draw a "thickness" depending on history size
	for (size_t i = _current; i <= _history.size(); i++)
	{
		int d = i - _current;
		double h = (double)W / 200;
		if (h > 1.) h = (int)h;
		double x = (double)X + d * h + .5;
		double y = (double)Y + d * h + .5;
		fl_color(FL_BLACK);
		fl_line_style(FL_DASH); // FL_DASH seems best to get a non solid look
		fl_line(floor(x) + W, floor(y), floor(x) + W, floor(y) + H);
		fl_line(floor(x) + W, floor(y) + H , floor(x), floor(y) + H);
		fl_color(FL_BLACK);
		fl_line_style(FL_SOLID);
		fl_point(floor(x) + W, floor(y) + H ); // draw manually the edge
	}
	fl_line_style(0);
	fl_color(_current == 0 ? fl_lighter(fl_lighter(FL_YELLOW)) : GRAY);
	fl_rectf(X, Y, W, H);
	fl_color(_current == 0 ? GRAY : FL_BLACK);
	fl_rect(X, Y, W, H);
	if (_current < _history.size())
	{
		int D = W / 12;
		fl_rect(X + W - D, Y + H - D, D, D);
		fl_line(X + W - D, Y + H - 1, X + W - 1, Y + H - D);
	}
	fl_color(FL_BLACK);
	fl_font(FL_COURIER, H / 14);
	X += W / 20;
	Y += fl_descent() + fl_height();
	Util::draw_color_text(Util::message(GAMEBOOK), X, Y);
	fl_line_style(FL_SOLID, 2);
	fl_line(X, Y + fl_descent(), X + W - W / 10, Y + fl_descent());
	Y += H / 10;
	Util::draw_color_text(Util::message(GB_HEADLINE), X, Y);
	int h = H - H / 5;
	fl_line_style(0);
	int w = W - W / 10;
	fl_line(X, Y + fl_descent(), X + w, Y + fl_descent());
	fl_line(X + w / 2, Y - fl_height(), X + w / 2, Y + h - fl_descent());
	Y += fl_descent();

	int player_score = 0;
	int ai_score = 0;

	auto draw_score = [&](std::pair<int, int> s) -> void
	{
		char pbuf[20];
		char abuf[20];
		if (MATCH_SCORE - player_score <= 0 || MATCH_SCORE - ai_score <= 0) return;
		fl_color(FL_DARK_BLUE);
		if (!s.first && !s.second)
		{
			snprintf(pbuf, sizeof(pbuf), "%d", MATCH_SCORE - player_score);
			snprintf(abuf, sizeof(abuf), "%d", MATCH_SCORE - ai_score);
		}
		else
		{
			snprintf(pbuf, sizeof(pbuf), "%d", MATCH_SCORE - player_score);
#ifdef USE_SVG_DIGITS
			if (!s.first || pbuf[0] == '0') pbuf[0] = '0';
#else
			if (!s.first || pbuf[0] == '0') pbuf[0] = '-';
#endif
			snprintf(abuf, sizeof(abuf), "%d", MATCH_SCORE - ai_score);
#ifdef USE_SVG_DIGITS
			if (!s.second || abuf[0] == '0') abuf[0] = '0';
#else
			if (!s.second || abuf[0] == '0') abuf[0] = '-';
#endif
		}
		std::ostringstream os;
#ifdef USE_SVG_DIGITS
		os << "   ^|" << pbuf << "|       ^|" << abuf << "|";
#else
		os << "  " << std::setw(2) << pbuf << "      " << std::setw(2) << abuf;
#endif
		Y += H / 12;
		Util::draw_string(os.str(), X, Y);
	};

	std::vector<std::pair<int, int>> value = *this;
	if (_current > 0)
	{
		value = _history[_current - 1];
	}
	value.insert(value.begin(), std::make_pair(0, 0));
	// limit display to last 8 scores
	size_t first = value.size() > 8 ? value.size() - 8 : 0;
	for (size_t i = 0; i < value.size(); i++)
	{
		auto s = value[i];
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
#ifdef USE_SVG_DIGITS
			(ai_score >= MATCH_SCORE ? "^|bum|" : " "), (player_score >= MATCH_SCORE ? "^|bum|" : " "));
#else
			(ai_score >= MATCH_SCORE ? "●" : " "), (player_score >= MATCH_SCORE ? "●" : " "));
#endif
		fl_color(FL_BLACK);
		Util::draw_string(buf, X, Y);
	}
}
