#pragma once

#include "UI.h"
#include "Cards.h"
#include "Deck.h"
#include <vector>
#include <utility>

struct GameState
{
	GameState() : score(0), pending(false),
	              message(Message::NO_MESSAGE), deck_info(false), games_won(0),
	              matches_won(0), display_score(false), move_state(CardState::NONE) {}
	Cards     cards;    // hand
	Cards     deck;     // stack
	Card      card;     // card that is moved to play
	int       score;    // current score
	int       pending;  // from 20/40 will score *after* won trick
	Suites    s20_40;
	Message   message;
	bool      deck_info;
	int       games_won;
	int       matches_won;
	bool      display_score;
	CardState move_state;
	Card      last_drawn;
	Card      changed;
};

struct GameData
{
	GameData() : marriage(Marriage::NO_MARRIAGE),
	             closed(Closed::NOT), move(Player::PLAYER) {}
	Cards     cards;    // remaining cards
	CardSuite trump;    // trump suite (initially NO_SUITE)
	Marriage  marriage;
	Closed    closed;
	Player    move;
	std::vector<std::pair<int, int>> book;
};

class Engine
{
public:
	explicit Engine(GameData &game_, GameState &player_, GameState &ai_, UI &ui_) :
		_game(game_), _player(player_), _ai(ai_), _ui(ui_), _move(NO_MOVE)
	{
	}
	size_t ai_move();
	void ai_move_follow();
	void ai_move_lead();
	void ai_move_closed_follow();
	void ai_move_closed_lead();

	size_t ai_play_for_last_trick_lead();
	size_t ai_play_for_last_trick_follow();
	size_t ai_play_for_closed_lead();

	Suites have_20(const Cards &cards_);
	Suites have_40(const Cards &cards_);
	size_t find(const Card &c_, const Cards &cards_) const;
	size_t lowest_card(Cards &cards_, bool no_trump_ = true) const;
	size_t lowest_card_that_tricks(const Card &c_, const Cards &cards_) const;
	size_t highest_card_that_tricks(const Card &c_, const Cards &cards_) const;
	Cards all_cards_that_trick(const Card &c_, const Cards &cards_) const;
	bool has_suite(const Cards &cards_, CardSuite suite_) const;
	bool can_trick(const Card &c_, const Cards &cards_) const;
	bool can_trick_with_suite(const Card &c_, const Cards &cards_) const;
	bool card_tricks(const Card &c1_, const Card &c2_) const;
	size_t best_trick_card(const Card &c_, Cards &tricks_) const;
	bool test_change(GameState &player_, bool change_ = false);
	size_t ai_play_20_40();
	bool ai_test_close();
	Cards highest_cards_of_suite_in_hand(const Cards &cards_, CardSuite suite_);
	Cards highest_cards_in_hand(const Cards &cards_);
	Cards suites_in_hand(CardSuite suite_, const Cards &cards_) const;
	Cards trumps_in_hand(const Cards &cards_) const;
	size_t must_give_color_or_trick(const Card &c_, Cards &cards_) const;
	Cards cards_to_claim(CardSuite suite_ = ANY_SUITE) const;
	Cards trumps_to_claim() const;
	Cards count_played_suite(CardSuite suite_) const;
	int cards_in_play(CardSuite suite_) const;
	int max_cards_player(CardSuite suite_) const;
	int max_trumps_player() const;
	Cards pull_trump_cards(Cards cards_, Cards from_) const;
	Cards closed_lead_no_trick(Cards leader_, Cards follower_);
	bool unit_tests();
private:
	GameData &_game;
	GameState &_player;
	GameState &_ai;
	UI &_ui;
	size_t _move;
};
