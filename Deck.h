#pragma once

// for readability of std::vector/dequeu index results
constexpr size_t NO_MOVE = (size_t)-1;

// score points count down for "bummerl"
constexpr int MATCH_SCORE = 7; // or 11?


enum class Player
{
	PLAYER,
	AI
};

enum class Closed
{
	NOT,
	BY_PLAYER,
	BY_AI,
	AUTO
};

enum class CardState
{
	NONE,
	MOVING,
	ON_TABLE
};

enum class Marriage
{
	NO_MARRIAGE,
	MARRIAGE_20,
	MARRIAGE_40
};
