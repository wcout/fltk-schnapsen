#include "Engine.h"

#include <ranges>

using enum Player;
using enum CardState;
using enum Message;
using enum Closed;
using enum Marriage;

#include "Unittest.cxx"
bool Engine::unit_tests()
{
	Unittest ut(_game, _player, _ai, *this);
	return ut.run();
}

Engine& Engine::sort_cards(Cards &cards_)
{
	(_game.trump_sort ? cards_.sort(_game.trump) : cards_.sort());
	return *this;
}

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
		if (_ai.score < 33)
		{
			// try to find a trick that pushes score beyond 32
			for (auto &c : tricks_)
			{
				if (c.value() + c_.value() + _ai.score + _ai.pending >= 33)
				{
					move = find(c, tricks_);
					break;
				}
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

size_t Engine::best_trick_card_or_no_move(const Card &c_, Cards &tricks_) const
{
	size_t move = best_trick_card(c_, tricks_);
	assert(move != NO_MOVE);

	if (move == lowest_card_that_tricks(c_, tricks_))
	{
		// that was the default move
		Card c = tricks_[move];
		// Do not trick with trump a low non-trump
		if (c.suite() == _game.trump && c_.value() <= 4)
		{
			// with some exceptions:
			if (_player.score + c.value() + c_.value() < 66)
			{
				WNG("Don't waste trump " << c << " on low non-trump " << _player.card);
				return NO_MOVE;
			}
		}
	}

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

bool Engine::can_trick(const Card &c_, const Cards &cards_) const
{
	for (auto &c : cards_)
	{
		if (card_tricks(c, c_)) return true;
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
	bool result(false);
	if (c1_.suite() == c2_.suite())
	{
		if (c1_.value() > c2_.value()) result = true;
	}
	else if (c1_.suite() == _game.trump)
	{
		result = true;
	}
//	DBG("== (" << Card::suite_symbol(_game.trump) << ") does card " << c1_ << " trick card " << c2_ << ": " << (result==true ? "yes" : "no") << "\n");
	return result;
}

Cards Engine::all_cards_that_trick(const Card &c_, const Cards &cards_) const
{
	Cards res;
	Cards cards(cards_);
	cards.sort(_game.trump); // sort with trumps in first place
	for (auto &c : cards)
	{
		if (card_tricks(c, c_))
		{
			res.push_front(c); // valuable trump tricks go to end
		}
	}
	DBG("all_cards_that_trick: " << cards_ << " - " << c_ << " => " << res << "\n");
	return res;
}

size_t Engine::lowest_card(const Cards &cards_, bool no_trump_/* = true*/) const
{
	// return the lowest card, but no trump if possible
	Cards cards(cards_);
	cards.sort_by_value(); // hi->low
	Card lowest = cards.back(); // default, can also be trump
	if (no_trump_ == true)
	{
		while (cards.size() && cards.back().suite() == _game.trump)
		{
			cards.pop_back();
		}
		if (cards.size())
			lowest = cards.back();
	}
	return find(lowest, cards_);
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
	sort_cards(player_.cards);
	player_.changed = c;

	_ui.message(CHANGED);
	_ui.update();

	if (_game.move == AI)
		_ui.wait(1.5);
	return true;
}

int Engine::gain(const Cards &player_, const Cards &opponent_)
{
	// only for closed game!
	// calculate how many points the cards in player_ would minimal score
	// when played out against opponent_ cards.
	// player_ should normally contain 'highest_cards_in_hand()` of leader.
	int points = 0;
	Cards opponent(opponent_);
	for (auto &c : player_)
	{
		if (can_trick_with_suite(c, opponent) == false)
		{
			points += c.value();
			Cards suite = suites_in_hand(c.suite(), opponent);
			if (suite.size())
			{
				suite.sort_by_value(false);
				Card r = suite[0];
				points += r.value();
				opponent -= r;
			}
		}
		else
		{
			WNG("gain() used with trickable card " << c);
		}
	}
	return points;
}

size_t Engine::ai_play_20_40()
{
	size_t move = NO_MOVE;
	Suites suites = have_40(_ai.cards);
	if (suites.size())
	{
		// ai has 40
		// optimization: if having 40 and player still may have trumps
		// try first to pull a trump with A or 10
		Cards highest = highest_cards_in_hand();
		if (max_trumps_player() && _ai.score + 44 < 66)
		{
			Cards highest_trumps = trumps_in_hand(highest);
			if (highest_trumps.size() && highest_trumps[0].value() >= 10)
			{
				WNG("***try pull trump before playing 40!\n");
				move = find(highest_trumps[0], _ai.cards);
				return move;
			}

			// also if having highest cards, maybe try to play these before
			highest -= Card(QUEEN, _game.trump);
			highest -= Card(KING, _game.trump);
			if (highest.size() && _game.closed != NOT)
			{
				int points_needed = 66 - 40 - _ai.score - _ai.pending;
				int gain_points = gain(highest, assumed_player_cards());
				if (points_needed > 0) // 40 alone is enough to win!
				{
					DBG(points_needed << " points needed after 40 - possible gain with highest: " << gain_points << "\n");
					// NOTE: don't care about gain_points currently
					move = find(highest[0], _ai.cards);
					return move;
				}
			}
		}

		// play the 40, check if using king wins game
		_game.marriage = MARRIAGE_40;
		_ui.bell(AI_MARRIAGE_40);
		if (_ai.score + 43 == 65)
		{
			move = find(Card(KING, _game.trump), _ai.cards);
		}
		else
		{
			// otherwise use queen
			move = find(Card(QUEEN, _game.trump), _ai.cards);
		}
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
	// NOTE: returns cards in low -> high order!
	// Use: push_back() to keep sort order highest -> lowest?
	Cards res;
	for (auto &c : cards_)
		if (c.suite() == suite_) res.push_front(c);
	return res;
}

Cards Engine::trumps_in_hand(const Cards &cards_) const
{
	return suites_in_hand(_game.trump, cards_);
}

Cards Engine::highest_cards_of_suite_in_hand(const Cards &cards_, CardSuite suite_)
{
	Cards res;
	// all cards of 'suite' that were already played
	Cards played = _ai.deck + _player.deck;
	if (_game.cards.size())
		played += _game.cards.back(); // including open trump certainly not in play
	Cards played_suites(suites_in_hand(suite_, played));

	// cards of suite in (ai) hand
	Cards suites(suites_in_hand(suite_, cards_));

	// check all cards in hand if there are higher cards that are not already played
	for (auto &c : suites)
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

void Engine::do_close(GameState &player_)
{
	// close game
	LOG("*** closed by " << (_game.move == PLAYER ? "player" : "AI") << "!\n");
	_ui.animate_close();
	_game.closed = _game.move == PLAYER ? BY_PLAYER : BY_AI;
	player_.score_closed = player_.score; // memorize score at close time
	_ui.message(CLOSED, true);
	_ui.update();
}

bool Engine::ai_test_close()
{
	// test if ai should close
	if (_game.closed == NOT && _player.move_state == NONE && _ai.move_state == MOVING &&
	    _game.cards.size() >= 4)
	{
		int maybe_score = _ai.score + _ai.pending;
		// NOTE: this is normally already done, except when pulling trump before 40.
		//       So we check, if a marriage is already declared
		if (_game.marriage == NO_MARRIAGE)
		{
			if (have_40().size()) maybe_score += 40;
			else if (have_20().size()) maybe_score += 20;
		}
		bool do_close = maybe_score >= 66; // that's a sure thing!
		if (!do_close)
		{
			// test if cards are good enough
			Cards highest = highest_cards_in_hand();
			maybe_score += highest.value();
			maybe_score += highest.size() * 3; // at average expect win of a queen per trick
			do_close = maybe_score >= 60 &&
				(size_t)max_trumps_player() <= trumps_in_hand(_ai.cards).size();
		}
		DBG("maybe_score: " << maybe_score << "\n")
		if (do_close)
		{
			this->do_close(_ai);
			_ui.wait(1.5);
			return true;
		}
	}
	return false;
}

Cards Engine::assumed_player_cards() const
{
	// in use at end game playout ("allowed" to use _player.cards)
	Cards player_cards = Cards::fullcards() - _player.deck - _ai.deck - _ai.cards;
	if (_game.cards.size())
		player_cards -= _game.cards.back(); // open trump is certainly not in player cards
	player_cards.sort();
	DBG("assumed player cards: " << player_cards << "\n")
	return player_cards;
}

Cards Engine::cards_to_claim(CardSuite suite_/* = ANY_SUITE*/) const
{
	Cards res;
	Cards player_cards = assumed_player_cards();
	for (auto &c : _ai.cards)
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
			for (auto &c : cards_)
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

Cards Engine::hinder_20_40()
{
	if (_game.closed == NOT)
	{
		WNG("hinder_20_40 should be used only in end game!");
	}
	// Use only in normal Endgame:
	// check if player holds 20 or 40
	// check if it is possible to hinder that
	Suites s20 = have_20(_player.cards);
	Suites s40 = have_40(_player.cards);
	if (s20.empty() && s40.empty())
	{
		return {};
	}
	if (s20.size() > 1)
	{
		// don't deal with more than one 20 suites
		return {};
	}

	if (s40.size())
	{
		// Player holds 40
		Cards suites = suites_in_hand(s40[0], _player.cards);
		if (suites.size() > 2)
		{
			return {};
		}
		return all_cards_that_trick(Card(QUEEN, s40[0]), _ai.cards);
	}

	if (s20.size())
	{
		// Player holds single 20
		Cards suites = suites_in_hand(s20[0], _ai.cards);
		if (suites.size() > 2)
		{
			return {};
		}
		return all_cards_that_trick(Card(QUEEN, s20[0]), _ai.cards);
	}
	return {};
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

Cards Engine::closed_lead_no_trick(Cards leader_, Cards follower_)
{
	// Finds out if there is no immedeate trick possible,
	// which card of suite to play, to gain the highest card of suite
	// in next move.
	auto for_suite = [&](CardSuite suite_) -> Cards
	{
		Cards cards;
		Cards leader = suites_in_hand(suite_, leader_);
		Cards follower = suites_in_hand(suite_, follower_);
		if (leader.size() < 2 || follower.size() < 2) return cards;
		leader.sort();
		follower.sort();
//		DBG("leader: " << leader << " follower: " << follower << "\n");
		for (const auto& c : leader | std::views::reverse)
		{
			if (card_tricks(c, follower[1]))
			{
				cards.push_back(c);
				break;
			}
		}
		cards.sort();
		return cards;
	};
	Cards res;
	res += for_suite(SPADE);
	res += for_suite(HEART);
	res += for_suite(DIAMOND);
	res += for_suite(CLUB);
	DBG("closed_lead_no_trick: " << res << "\n");
	return res;
}

Cards Engine::valid_moves(const Cards &hand_, const Card &lead_)
{
	//
	// Return valid moves *in closed state* for hand_ with move lead_
	//

	Cards res;

	for (auto &c : hand_)
	{
		if (c.suite() == lead_.suite())
		{
			// if having suite, we must trick with it, if we can
			if (c.value() > lead_.value())
				res.push_back(c);
		}
	}
	if (res.empty())
	{
		for (auto &c : hand_)
		{
			// if having suite (and we can't trick) we must give color
			if (c.suite() == lead_.suite())
			{
				if (c.value() < lead_.value())
					res.push_back(c);
			}
		}
	}
	if (res.empty())
	{
		for (auto &c : hand_)
		{
			// otherwise we must trick with trump
			if (c.suite() == _game.trump && card_tricks(c, lead_))
			{
				res.push_back(c);
			}
		}
	}
	if (res.empty())
	{
		// otherwise any card is valid
		res = hand_;
	}
	DBG("valid moves for hand " << hand_ << " with lead " << lead_  << ": " << res << "\n");
	return res;
}

size_t Engine::ai_play_for_last_trick_lead()
{
	//
	// Test if winning last trick is possible (with last 2 cards in hand)
	//

	assert( _player.cards.size() == 2 && _ai.cards.size() == 2);
	Cards player = assumed_player_cards();
	Cards ai_cards = _ai.cards;

	Cards good;

	for (size_t i = 0; i < ai_cards.size(); i++) // NOTE: 0-1
	{
		Cards valid = valid_moves(player, ai_cards[i]);
		for (auto &p : valid)
		{
			if (card_tricks(p, ai_cards[i]))
			{
				// player wins trick, and plays out second card
				Card ai_card = (ai_cards - ai_cards[i])[0];
				Card player_card = (player - p)[0];
				if (card_tricks(ai_card, player_card))
				{
					// if player would win with that trick, it's no good anyway
					if (_player.score + _player.pending + ai_card.value() + player_card.value() < 66)
						good.push_back(ai_cards[i]);
				}
			}
			else
			{
				// ai wins trick, and plays out second card
				if (!card_tricks((player - p)[0], (ai_cards - ai_cards[i])[0]))
					good.push_back(ai_cards[i]);
			}
		}
	}

	DBG("good: " << good << "\n");
	if (good.size())
	{
		return find(good[0], _ai.cards);
	}
	return NO_MOVE;
}

size_t Engine::ai_play_for_closed_lead()
{
	// Test if it is of advantage to have the lead after
	// the pack is cleared.

	if (_player.move_state == ON_TABLE)
	{
		// following last trick before pack clear
		DBG("ai_play_for_closed_lead: following\n");

		bool should_trick = false;
		Cards player_cards = assumed_player_cards();
		if (have_40(player_cards).size()) // TODO: need a 'bool' method too
		{
			// player will have 40 if he wins this trick
			should_trick = true;
		}

		Cards tricks = all_cards_that_trick(_player.card, _ai.cards);
		if (tricks.size())
		{
			if (should_trick)
			{
				WNG("We must trick, otherwise player will gain 40!");
				size_t move = best_trick_card(_player.card, tricks);
				assert(move != NO_MOVE);
				return find(tricks[move], _ai.cards);
			}

			// we could trick, but that's maybe not what we want..
			DBG("ai_play_for_closed_lead: we could trick with: " << tricks << "\n");
			if ((_game.cards.back().face() == QUEEN && _ai.cards.find(Card(KING, _game.trump))) ||
			    (_game.cards.back().face() == KING  && _ai.cards.find(Card(QUEEN, _game.trump))))
			{
				// we could gain 40, if we do **not** trick
				Cards not_tricks = _ai.cards - tricks;
				if (not_tricks.size())
				{
					not_tricks.sort_by_value(false); // low-high
					DBG("ai_play_for_closed_lead: we could gain 40, when **not** tricking!\n");
					return find(not_tricks[0], _ai.cards);
				}
			}

			// is back card (talon) valuable for us?
			if ((int)trumps_in_hand(_ai.cards).size() <
			    max_trumps_player() + 1/*the one, player would get if tricking*/)
			{
				// let's say yes (TODO: look into more detail)
				Cards not_tricks = _ai.cards - tricks;
				if (not_tricks.size())
				{
					not_tricks.sort_by_value(false); // low-high
					DBG("ai_play_for_closed_lead: want trump, so **not** tricking!\n");
					return find(not_tricks[0], _ai.cards);
				}
			}
		}
	}
	else
	{
		// leading last trick before pack clear
		DBG("ai_play_for_closed_lead: leading\n");
		Cards player_cards = assumed_player_cards();
		if (have_40(player_cards).size()) // TODO: need a 'bool' method too
		{
			// player will have 40 if he wins this trick
			Cards trumps = trumps_in_hand(player_cards);
			trumps -= Card(QUEEN, _game.trump);
			trumps -= Card(KING, _game.trump);
			if (trumps.empty())
			{
				// player has no additional trumps
				Cards highest = highest_cards_in_hand(_ai.cards);
				if (highest.size())
				{
					// so play card he must either trick with trump or leave
					highest.sort_by_value(false); // low->hi
					DBG("highest sorted by value: " << highest << "\n");
					WNG("Use " << highest[0] << " to prevent player getting 40");
					return find(highest[0], _ai.cards);
				}
			}
		}
	}
	return NO_MOVE;
}

size_t Engine::winning_move()
{
	 // assumed_player_cards() is only valid when auto closed
	if (_game.closed != AUTO)
		return NO_MOVE;
	// This should be the cards player has in hand
	Cards player_cards(assumed_player_cards());
	// test all ai cards, if a trick would push the score to win
	for (size_t m = 0; m < _ai.cards.size(); m++)
	{
		Card &c = _ai.cards[m];
		if (can_trick(c, player_cards)) continue;
		// This is a card player can't trick
		Card player_card;
		Cards same_suite = suites_in_hand(c.suite(), player_cards);
		if (same_suite.size())
		{
			// if player has that suite he will give the lowest of it
			player_card = same_suite[0];
		}
		else
		{
			// otherwise he gives his lowest card in hand
			player_card = player_cards[lowest_card(player_cards, false)];
		}
		assert(player_card.suite() != NO_SUITE);
		// now with that card lets calculate the new ai score:
		bool is_winning = player_card.value() + _ai.score + _ai.pending + c.value() >= 66;
		if (is_winning)
		{
			// this card will win game
			DBG("winning_move: " << c << "\n");
			return m;
		}
	}
	return NO_MOVE;
}

void Engine::ai_move_closed_lead()
{
	// end game, ai plays out
	size_t move = NO_MOVE;

	size_t m = winning_move();
	if (m != NO_MOVE)
	{
		// this move wins the game..
		_move = m;
		return;
	}

	if (_ai.cards.size() == 2)
	{
		// special case, before last trick
		size_t m = ai_play_for_last_trick_lead();
		if (m != NO_MOVE)
		{
			DBG("ai_play_for_last_trick suggested: " << _ai.cards[m] << "\n");
			move = m;
		}
	}

	m = ai_play_20_40();
	if (m != NO_MOVE)
	{
		move = m;
	}
	else
	{
		Cards hinder = hinder_20_40();
		if (hinder.size())
		{
			move = find(hinder[0], _ai.cards);
			assert(move != NO_MOVE);
			WNG("hinder 20/40 with " << _ai.cards[move] << " from " << hinder);
		}
		else if (!(_ai.cards.size() == 2 && move != NO_MOVE)) // play for last move!
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
					if (_game.cards.size())
						claim.sort(_game.trump); // trumps first
					DBG("claim: " << claim << "\n");
					move = find(claim[0], _ai.cards);
				}
			}
		}
	}

	if (move == NO_MOVE)
	{
		Cards pull = pull_trump_cards(_ai.cards, _player.cards);
		if (pull.size())
			move = find(pull[0], _ai.cards);
	}

	if (move == NO_MOVE && _game.cards.empty())
	{
		Cards no_trick = closed_lead_no_trick(_ai.cards, _player.cards);
		if (no_trick.size())
			move = find(no_trick[0], _ai.cards);
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
	if (test_change(_ai))
	{
		test_change(_ai, true); // make change
		if (_move != NO_MOVE)	// if we had a default move, it needs to be redone
		{
			_move = default_move();
			assert(_move != NO_MOVE);
			DBG("new default move: " << _ai.cards[_move] << "\n");
		}
	}

	if (_game.cards.size() == 2)
	{
		// special case, before pack clearing
		// TODO: implement
		size_t m = ai_play_for_closed_lead();
		if (m != NO_MOVE)
			_move = m;
	}

	size_t m = ai_play_20_40();
	if (m != NO_MOVE)
		_move = m;

	if (ai_test_close())
	{
		if (_game.marriage == NO_MARRIAGE)
		{
			ai_move_closed_lead(); // use closed routine!
		}
	}
}

void Engine::ai_move_follow()
{
	// normal game, player has moved, ai to follow
	if (_game.cards.size() == 2)
	{
		// special case, before pack clearing
		// TODO: implement
		size_t m = ai_play_for_closed_lead();
		if (m != NO_MOVE)
		{
			WNG("ai_play_for_closed_lead suggested " << _ai.cards[m]);
			_move = m;
		}
	}

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
			size_t move = best_trick_card_or_no_move(_player.card, tricks);
			if (move != NO_MOVE)
				_move = find(tricks[move], _ai.cards);
			else
			{
				// another exception: do rather trick, than give away a high card
				assert(_move != NO_MOVE);
				if (_ai.cards[_move].value() >= 10)
				{
					WNG("Nevertheless trick, rather than giving away " << _ai.cards[_move]);
					_move = find(tricks[best_trick_card(_player.card, tricks)], _ai.cards);
				}
			}
		}
	}
}

size_t Engine::default_move(const Cards &cards_) const
{
	return lowest_card(cards_); // default move is the lowest card
}

size_t Engine::ai_move()
{
	_game.marriage = NO_MARRIAGE;
	assert(_ai.cards.size());

	_move = default_move();
	assert(_move != NO_MOVE);
	DBG("default move: " << _ai.cards[_move] << "\n");

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
