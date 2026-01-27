#pragma once

#include <vector>
#include <utility>

class GameBook : public std::vector<std::pair<int, int>>
{
public:
	GameBook() {}
	std::string str() const;
};
