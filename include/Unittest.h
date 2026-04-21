#pragma once

#include "Engine.h"

class Unittest
{
public:
	explicit Unittest(GameData &game_, PlayerData &player_, PlayerData &ai_, Engine &engine_) :
		_game(game_), _player(player_), _ai(ai_), _engine(engine_)
	{
	}
	bool run();
private:
	GameData &_game;
	PlayerData &_player;
	PlayerData &_ai;
	Engine &_engine;
};
