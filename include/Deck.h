#pragma once

// for readability of std::vector/dequeu index results
constexpr size_t NO_MOVE = (size_t)-1;

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

enum class Result
{
	NO_WIN,
	PLAYER_WINS_BY_SCORE,
	AI_WINS_BY_SCORE,
	PLAYER_WINS_BY_LAST_TRICK,
	AI_WINS_BY_LAST_TRICK,
	PLAYER_WINS_CLOSED_GAME,
	AI_WINS_CLOSED_GAME,
	PLAYER_WINS_AI_CLOSED_NOT_ENOUGH,
	AI_WINS_PLAYER_CLOSED_NOT_ENOUGH
};
