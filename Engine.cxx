#include "Engine.h"

using enum Player;
using enum CardState;
using enum Message;
using enum Closed;
using enum Marriage;


size_t Engine::best_trick_card(const Card &c_, Cards &tricks_) const
{
	size_t move = NO_MOVE;
	assert(tricks_.size());

	// try to find a game winning trick
	for (auto &c : tricks_)
	{
		if (c.value() + c_.value() + _ai.score + _ai.pending >= 66)
		{
			move = find(c, tricks_);
			break;
		}
	}
	if (move == NO_MOVE)
	{
		// try to find a trick that pushes score beyond 32
		for (auto c : tricks_)
		{
			if (c.value() + c_.value() + _ai.score + _ai.pending >= 33)
			{
				move = find(c, tricks_);
				break;
			}
		}
	}
	if (move == NO_MOVE)
	{
		move = lowest_card_that_tricks(c_, tricks_);
	}
	assert(move != NO_MOVE);
	DBG("best_trick_card: " << tricks_ << " - " << c_ << " => " << tricks_[move] << "\n")
	return move;
}

bool Engine::has_suite(const Cards &cards_, CardSuite suite_) const
{
	for (auto &c : cards_)
	{
		if (c.suite() == suite_) return true;
	}
	return false;
}

bool Engine::can_trick_with_suite(const Card &c_, const Cards &cards_) const
{
	for (auto &c : cards_)
	{
		if (c.suite() != c_.suite()) continue;
		if (card_tricks(c, c_)) return true;
	}
	return false;
}

bool Engine::card_tricks(const Card &c1_, const Card &c2_) const
{
	// does card c1 trick card c2?
	bool result( false );
	if (c1_.suite() == c2_.suite())
	{
		if (c1_.value() > c2_.value()) result = true;
	}
	else if (c1_.suite() == _game.trump)
	{
		result = true;
	}
	return result;
}

Cards Engine::all_cards_that_trick(const Card &c_, const Cards &cards_) const
{
	Cards res;
	Cards cards(cards_);
	cards.sort(_game.trump); // sort with trumps in first place
	for (auto c : cards)
	{
		if (card_tricks(c, c_))
		{
			res.push_front(c); // valuable trump tricks go to end
		}
	}
	DBG("all_cards_that_trick: " << cards_ << " - " << c_ << " => " << res << "\n");
	return res;
}

size_t Engine::lowest_card(Cards &cards_, bool no_trump_/* = true*/) const
{
	// return the lowest card, but no trump if possible
	int lowest_value = 20;
	int lowest_value_trump = 20;
	size_t lowest = NO_MOVE;
	size_t lowest_trump = NO_MOVE;
	for (size_t i=0; i < cards_.size(); i++)
	{
		if (cards_[i].suite() == _game.trump)
		{
			if (cards_[i].value() < lowest_value_trump)
			{
				lowest_value_trump = cards_[i].value();
				lowest_trump = i;
			}
		}
		else if (cards_[i].value() < lowest_value)
		{
			lowest_value = cards_[i].value();
			lowest = i;
		}
	}
	if (lowest == NO_MOVE)
		lowest = lowest_trump;
	if (no_trump_ == true && lowest != NO_MOVE)
		return lowest;
	return lowest_trump;
}

size_t Engine::lowest_card_that_tricks(const Card &c_, const Cards &cards_) const
{
	int lowest_value = 999;
	size_t lowest = NO_MOVE;
	for (size_t i = 0; i < cards_.size(); i++)
	{
		if (card_tricks(cards_[i], c_))
		{
			int value = cards_[i].value();
			if (cards_[i].suite() == _game.trump)
				value += 100;
			if (value < lowest_value)
			{
				lowest_value = value;
				lowest = i;
			}
		}
	}
	return lowest;
}

size_t Engine::highest_card_that_tricks(const Card &c_, const Cards &cards_) const
{
	// NOTE: seems unused currently
	int highest_value = 0;
	size_t highest = NO_MOVE;
	for (size_t i = 0; i < cards_.size(); i++)
	{
		if (card_tricks(cards_[i], c_))
		{
			int value = cards_[i].value();
			if (cards_[i].suite() == _game.trump)
				value -= 1;
			if (value > highest_value)
			{
				highest_value = value;
				highest = i;
			}
		}
	}
	return highest;
}

Suites Engine::have_40(const Cards &cards_)
{
	Suites result;
	auto trump_queen = cards_.find(Card(QUEEN, _game.trump));
	auto trump_king = cards_.find(Card(KING, _game.trump));
	if (trump_queen && trump_king)
	{
		result.push_back(_game.trump);
	}
	// trump or empty()
	return result;
}

Suites Engine::have_20(const Cards &cards_)
{
	Suites result;
	auto find_20 = [&] (CardSuite suite) -> bool
	{
		if (cards_.find(Card(QUEEN, suite)) && cards_.find(Card(KING, suite)))
		{
			result.push_back(suite);
			return true;
		}
		return false;
	};
	if (_game.trump != SPADE) find_20(SPADE);
	if (_game.trump != HEART) find_20(HEART);
	if (_game.trump != DIAMOND) find_20(DIAMOND);
	if (_game.trump != CLUB) find_20(CLUB);
	// list of suites or empty
	return result;
}

size_t Engine::find(const Card &c_, const Cards &cards_) const
{
	auto i = cards_.find_pos(c_);
	if (i) return i.value();
	return NO_MOVE;
}

bool Engine::test_change(GameState &player_, bool change_/*=false*/)
{
	if (_game.cards.size() < 4 || _game.closed != NOT) return false;
	auto i = player_.cards.find_pos(Card(JACK, _game.cards.back().suite()));
	if (change_ == false) return !!i;
	assert(i);
	// make change
	LOG((_game.move == AI ? "AI" : "Player") << " changes jack for " << _game.cards.back() << "\n");
	Card jack = player_.cards[i.value()];
	player_.cards.erase(player_.cards.begin() + i.value());

	if (player_.card != jack)
	{
		player_.card = jack;
		_ui.animate_change(true); // jack from hand to deck
	}

	_ui.message(CHANGED, true);
	_ui.update();

	Card c = _game.cards.back();
	_game.cards.pop_back();
	_game.cards.push_back(player_.card);

	player_.card = c;
	_ui.animate_change();		// trump from deck to hand

	player_.cards.push_back(c);
	player_.cards.sort();
	player_.changed = c;
	_ui.wait(1.5);
	return true;
}

size_t Engine::ai_play_20_40()
{
	size_t move = NO_MOVE;
	Suites suites = have_40(_ai.cards);
	if (suites.size())
	{
		// ai has 40, play out queen
		_game.marriage = MARRIAGE_40;
		_ui.bell(AI_MARRIAGE_40);
		size_t trump_queen = find(Card(QUEEN, _game.trump), _ai.cards);
		move = trump_queen;
		LOG("AI declares 40 with " << _ai.cards[move] << "\n");
		_ai.s20_40.push_front(_ai.cards[move].suite());
		if (_ai.deck.empty())
		{
			_ai.pending += 40;
		}
		else
		{
			_ai.score += 40;
		}
	}
	else if ((suites = have_20(_ai.cards)).size())
	{
		_game.marriage = MARRIAGE_20;
		_ui.bell(AI_MARRIAGE_20);
		size_t first_suite_queen = find(Card(QUEEN, suites[0]), _ai.cards);
		move = first_suite_queen;
		LOG("AI declares 20 with " << _ai.cards[move] << "\n");
		_ai.s20_40.push_front(_ai.cards[move].suite());
		if (_ai.deck.empty())
		{
			_ai.pending += 20;
		}
		else
		{
			_ai.score += 20;
		}
	}
	return move;
}

Cards Engine::suites_in_hand(CardSuite suite_, const Cards &cards_) const
{
	// TODO: push_back() to keep sort order highest -> lowest?
	Cards res;
	for (auto &c : cards_)
		if (c.suite() == suite_) res.push_front(c);
	return res;
}

Cards Engine::highest_cards_of_suite_in_hand(const Cards &cards_, CardSuite suite_)
{
	Cards res;
	// all cards of 'suite' that were already played
	Cards played_suites(suites_in_hand(suite_, _ai.deck + _player.deck));

	// cards of suite in (ai) hand
	Cards suites(suites_in_hand(suite_, cards_));

	// check all cards in hand if there are higher cards that are not already played
	for (auto c : suites)
	{
		Cards temp(played_suites);
		temp += suites; // include self
		temp.erase(temp.begin() + find(c, temp)); // but not current card!
		if (c.face() == ACE) res.push_back(c);
		if (c.face() == TEN && temp.find_face(ACE)) res.push_back(c); // ACE already played (or in own hand)
		if (c.face() == KING && temp.find_face(TEN) && temp.find_face(ACE)) res.push_back(c); // TEN & ACE already played (or in own hand)
		if (c.face() == QUEEN && temp.find_face(KING) && temp.find_face(TEN) && temp.find_face(ACE)) res.push_back(c);
		if (c.face() == JACK && temp.find_face(QUEEN) && temp.find_face(KING) && temp.find_face(TEN) && temp.find_face(ACE)) res.push_back(c);
	}
	res.sort();
	DBG("highest_cards_of_suite_in_hand " << suite_symbols[suite_] << ": "<< res << "\n")
	return res;
}

Cards Engine::highest_cards_in_hand(const Cards &cards_)
{
	Cards res;
	res += highest_cards_of_suite_in_hand(cards_, HEART);
	res += highest_cards_of_suite_in_hand(cards_, SPADE);
	res += highest_cards_of_suite_in_hand(cards_, DIAMOND);
	res += highest_cards_of_suite_in_hand(cards_, CLUB);
	res.sort();
	DBG("highest_cards_in_hand :" << res << "\n")
	return res;
}

bool Engine::ai_test_close()
{
	// test if ai should close
	if (_game.closed == NOT && _player.move_state == NONE && _ai.move_state == MOVING &&
	    _game.cards.size() >= 4)
	{
		int maybe_score = highest_cards_in_hand(_ai.cards).value() + _ai.score + _ai.pending;
		DBG("maybe_score: " << maybe_score << "\n")
		if (maybe_score >= 60)
		{
			LOG("closed by AI!\n");
			_game.closed = BY_AI;
			_ui.message(CLOSED, true);
			_ui.update();
			_ui.wait(1.5);
			return true;
		}
	}
	return false;
}

Cards Engine::cards_to_claim(CardSuite suite_/* = ANY_SUITE*/) const
{
	Cards res;
	// in use at end game playout ("allowed" to use _player.cards)
	Cards player_cards = Cards::fullcards() - _player.deck - _ai.deck - _ai.cards;
	if (_game.cards.size())
		player_cards -= _game.cards.back(); // open trump is certainly not in player cards
	player_cards.sort();
	DBG("assumed player cards: " << player_cards << "\n")
	for (const auto &c : _ai.cards)
	{
		if (suite_ != ANY_SUITE && c.suite() != suite_) continue;
		for (size_t i = 0; i < player_cards.size(); i++)
		{
			const Card &pc = player_cards[i];
			if (pc.suite() != c.suite()) continue;
			if (card_tricks(pc, c)) break;
			res.push_back(c);
			player_cards.erase(player_cards.begin() + i);
			break;
		}
	}
	// TODO: sort by value or trump?
	res.sort_by_value();
	DBG("cards_to_claim: " << res << "\n")
	return res;
}

Cards Engine::trumps_to_claim() const
{
	// NOTE: seeem unused currently
	return cards_to_claim(_game.trump);
}

size_t Engine::must_give_color_or_trick(const Card &c_, Cards &cards_) const
{
	Cards same_suite;
	Cards trump_suite;
	for (auto &c : cards_)
	{
		if (c.suite() == c_.suite())
			same_suite.push_back(c);
	}
	if (same_suite.empty())
	{
		// we don't have this suite, maybe trick with trump?
		if (c_.suite() != _game.trump)
		{
			for (auto c : cards_)
			{
				if (c.suite() == _game.trump)
					trump_suite.push_back(c);
			}
			if (trump_suite.empty())
			{
				// we don't even have a trump
				return lowest_card(cards_);
			}
			Card best_trick = trump_suite[best_trick_card(c_, trump_suite)];
			return find(best_trick, cards_);
		}
		return lowest_card(cards_);
	}
	// we have this suite
	Cards tricks;
	for (auto &c : same_suite)
	{
		if (card_tricks(c, c_))
			tricks.push_back(c);
	}
	if (tricks.empty())
	{
		// we can't trick, so return lowest card of suite
		size_t i = lowest_card(same_suite);
		return find(same_suite[i], cards_);
	}
	Card best_trick = tricks[best_trick_card(c_, tricks)];
	return find(best_trick, cards_);
}

Cards Engine::count_played_suite(CardSuite suite_) const
{
	// counts all cards of suite 'suite_', that are
	// "knowable" by AI
	Cards res = suites_in_hand(suite_, _ai.deck) +
	            suites_in_hand(suite_, _player.deck);
	// include visible trump of pack (if still there and not closed)
	if (_game.closed == NOT && _game.cards.size() && _game.cards.back().suite() == suite_)
			res.push_front(_game.cards.back());
	return res;
}

int Engine::cards_in_play(CardSuite suite_) const
{
	return 5 - count_played_suite(suite_).size();
}

int Engine::max_cards_player(CardSuite suite_) const
{
	return cards_in_play(suite_) - suites_in_hand(suite_, _ai.cards).size();
}

int Engine::max_trumps_player() const
{
	return cards_in_play(_game.trump) - suites_in_hand(_game.trump, _ai.cards).size();
}

Cards Engine::pull_trump_cards(Cards cards_, Cards from_) const
{
	// all no-non trump cards in cards_, that can not
	// be tricket by higher card of suite, but need a trump.
	// Useable only when closed.
	Cards res;
	// does from_ (=player) even have trumps?
	Cards trumps = suites_in_hand(_game.trump, from_);
	if (trumps.empty())
	{
		return res; // no trumps to pull
	}
	for (auto &c : cards_)
	{
		if (c.suite() == _game.trump) continue; // skip trump suite
		// check if card can be tricked by player with suite
		// NOTE: With CLOSE_AUTO remaining cards are added to
		//       player hand, so the result is only a 'probable'
		//       result.
		Cards suites = suites_in_hand(c.suite(), from_ + _game.cards) ;
		bool no_trick = true;
		for (auto &s : suites)
		{
			if (!card_tricks(s, c)) continue;
			no_trick = false;
			break;
		}
		// player can't trick, so this card can be used to pull a trump
		if (no_trick == true)
		{
			res.push_back(c);
		}
	}
	res.sort_by_value(false); // sort low to high
	DBG("pull trump cards: " << res << "\n");
	return res;
}

void Engine::ai_move_closed_lead()
{
	// end game, ai plays out
	size_t move = NO_MOVE;
	size_t m = ai_play_20_40();
	if (m != NO_MOVE)
	{
		move = m;
	}
	else
	{
		Cards trump_claim = cards_to_claim(_game.trump);
		if (trump_claim.size() && (int)trump_claim.size() >= max_trumps_player())
		{
			move = find(trump_claim[0], _ai.cards);
		}
		else
		{
			Cards claim = cards_to_claim();
			if (claim.size())
			{
				move = find(claim[0], _ai.cards);
			}
		}
	}
	if (move == NO_MOVE)
	{
		Cards pull = pull_trump_cards(_ai.cards, _player.cards);
		if (pull.size())
			move = find(pull[0], _ai.cards);
	}
	if (move != NO_MOVE)
		_move = move;
}

void Engine::ai_move_closed_follow()
{
	// end game, player has moved, ai to follow
	_move = must_give_color_or_trick(_player.card, _ai.cards);
	assert(_move != NO_MOVE);
}

void Engine::ai_move_lead()
{
	// normal game, ai plays out
	test_change(_ai) && test_change(_ai, true);

	size_t m = ai_play_20_40();
	if (m != NO_MOVE)
		_move = m;

	if (ai_test_close())
	{
		if (_game.marriage == NO_MARRIAGE)
		{
			_move = find(highest_cards_in_hand(_ai.cards)[0], _ai.cards);
		}
	}
}

void Engine::ai_move_follow()
{
	// normal game, player has moved, ai to follow
	Suites s20 = have_20(_ai.cards);
	Suites s40 = have_40(_ai.cards);
	if (s20.size() || s40.size() || _player.card.value() >= 10)
	{
		// clumsily try to not destroy a 40 by tricking
		// TODO: better method for such things...
		Cards temp = _ai.cards;
		if (s40.size())
		{
			temp.erase(temp.begin() + find(Card(QUEEN,_game.trump), temp));
			temp.erase(temp.begin() + find(Card(KING,_game.trump), temp));
		}
		size_t m = lowest_card_that_tricks(_player.card, temp);
		if (m != NO_MOVE)
		{
			_move = find(temp[m], _ai.cards);
		}
	}
	else
	{
		Cards tricks = all_cards_that_trick(_player.card, _ai.cards);
		if (tricks.size())
		{
			size_t m = find(tricks[best_trick_card(_player.card, tricks)], _ai.cards);
			const Card &c = _ai.cards[m];
			// trick or not trick?
			bool trick(false);
			int score = _ai.cards[m].value() + _player.card.value() + _ai.pending;
			if (_ai.score < 33 && _ai.score + score >= 33)
				trick = true;
			else if (_ai.score >= 33 && _ai.score + score >= 66)
				trick = true;
			else if (_game.cards.size() <= 2 && _ai.score <= 50 && _ai.score + score >= 60)
				trick = true;
			else if (_player.card.suite() != _game.trump && c.suite() != _game.trump)
				trick = true;
#if 0
			else if (_game.cards.size() > 2 && _player.score + _player.pending + _player.card.value() + _ai.cards[lowest_card(_ai.cards)].value() >= 52)
			{
				DBG("trick because player is about to win..\n");
				trick = true;
			}
#endif
			if (trick)
				_move = m;
		}
	}
}

size_t Engine::ai_move()
{
	_game.marriage = NO_MARRIAGE;
	assert(_ai.cards.size());

	_move = lowest_card(_ai.cards); // default move lowest card
	assert(_move != NO_MOVE);

	if (_game.closed != NOT && _player.move_state == ON_TABLE)
	{
		ai_move_closed_follow();
	}
	else if (_game.closed != NOT)
	{
		ai_move_closed_lead();
	}
	else
	{
		if (_player.move_state == ON_TABLE)
		{
			ai_move_follow();
		}
		else
		{
			ai_move_lead();
		}
	}
	assert(_move != NO_MOVE);
	_ai.card = _ai.cards[_move];

	_ai.cards.erase(_ai.cards.begin() + _move);
	_ui.animate_move();

	_ai.move_state = ON_TABLE;
	_ui.update();
	LOG("AI move: " << _ai.card << "\n");
	return _move;
}
