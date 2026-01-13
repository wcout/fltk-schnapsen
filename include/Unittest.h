#pragma once

#include "Engine.h"

class Unittest
{
public:
	explicit Unittest(GameData &game_, GameState &player_, GameState &ai_, Engine &engine_) :
		_game(game_), _player(player_), _ai(ai_), _engine(engine_)
	{
	}
	bool run();
private:
	GameData &_game;
	GameState &_player;
	GameState &_ai;
	Engine &_engine;
};
