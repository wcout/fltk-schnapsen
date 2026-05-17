//
// Part of "Schnapsen for 2" card game.
//
// (c) 2026 Christian Grabner
//
// Make some basic "unit tests".
//

#include "Unittest.h"
#include "Engine.h"

#include <cassert>

using enum Player;
using enum CardState;
using enum Message;
using enum Closed;
using enum Marriage;

bool Unittest::run()
{
	CardSuite trump = _game.trump;
	_game.trump = SPADE;
	Cards temp;
	temp.push_front(Card(QUEEN, CLUB));
	temp.push_front(Card(KING, CLUB));
	temp.push_front(Card(TEN, CLUB));
	temp.push_front(Card(ACE, SPADE));
	Card c(ACE, CLUB);
	assert(_engine.can_trick_with_suite(c, temp) == false);
	assert(_engine.can_trick(c, temp) == true);
	Cards res(_game.cards);
	res += _game.cards;
	assert(res.size() == 40);
	res -= _game.cards;
	assert(res.size() == 20);
	res = _game.cards + _game.cards;
	assert(res.size() == 40);
	Cards c2("|Q♠|K♥|");
	res = _game.cards - c2;
	assert(res.size() == 18);
	temp = "|K♣|Q♣|T♣|K♥|A♠|";
	temp.sort();
	assert(temp[0] == Card(ACE, SPADE));
	assert(temp[1] == Card(KING, HEART));
	assert(temp[2] == Card(TEN, CLUB));
	assert(temp[3] == Card(KING, CLUB));
	assert(temp[4] == Card(QUEEN, CLUB));
	Cards clubs = _engine.suites_in_hand(CLUB, temp);
	assert(clubs[0] == Card(QUEEN, CLUB)); // lowest first!
	assert(_engine.lowest_card_that_tricks(Card(JACK, CLUB), temp) == 4); // 4=QUEEN/CLUB
	assert(_engine.highest_card_that_tricks(Card(JACK, CLUB), temp) == 0); // 0=ACE/SPADE (_game.trump=SPADE)
	assert(_engine.have_20(temp)[0] == CLUB);
	_game.trump = CLUB;
	assert(_engine.have_40(temp)[0] == CLUB);
	_game.trump = HEART;
	assert(_engine.highest_card_that_tricks(Card(JACK, CLUB), temp) == 2); // 2=TEN/CLUB (_game.trump=HEART)
	_game.trump = DIAMOND;
	assert(_engine.highest_card_that_tricks(Card(JACK, CLUB), temp) == 2); // 2=TEN/CLUB (_game.trump=DIAMOND)

	temp = _game.cards;
	_game.cards.clear();
	Cards c3("|A♠|K♥|K♣|Q♣|A♠|");
	_player.deck = "|T♣|";
	_ai.deck = "|A♣|";
	res = _engine.highest_cards_of_suite_in_hand(c3, CLUB);
	assert(res.size() == 2 && (res[0] == Card(KING, CLUB)) && (res[1] == Card(QUEEN, CLUB)));
	_ai.deck.clear();
	res = _engine.highest_cards_of_suite_in_hand(c3, CLUB);
	assert(res.size() == 0);
	_player.deck.clear();

	Cards c4("|A♦|T♦|K♦|Q♦|J♦|");
	_player.deck = "|T♣|J♣|";
	_ai.deck = "|T♣|K♣|K♥|Q♥|";
	_game.cards.clear();
	res = _engine.highest_cards_in_hand(c4);
	assert(res.size() == 5);
	_ai.deck.clear();
	_player.deck.clear();

	Cards c5("|K♠|K♥|K♣|Q♣|T♥|");
	_player.deck = "|T♣|T♠";
	_ai.deck = "|A♣|K♥";
	_game.trump = HEART;
	_game.cards = "|J♣|A♥|"; // game trump is Ace of hearts, so player hold highest hearts too
	res = _engine.highest_cards_in_hand(c5);
	assert(res == "|T♥|K♥|K♣|Q♣|");
	_ai.deck.clear();
	_player.deck.clear();
	_game.cards = temp;

	_game.trump = SPADE;
	temp = _game.cards;
	_game.cards.clear();
	Cards p1("|A♠|Q♥|Q♦|Q♣|J♣|");
	Cards a1("|K♠|Q♠|K♥|K♦|A♣|");
	Cards pull = _engine.pull_trump_cards(a1, p1);
	assert(pull.size() == 0);
	_game.trump = HEART;
	p1 = "|K♠|T♥|K♥|Q♥|K♦|";
	a1 = "|Q♠|A♥|J♥|A♦|T♣|";
	pull = _engine.pull_trump_cards(a1, p1);
	assert(pull == "|T♣|");
	p1 = "|J♠|T♥|K♥|Q♥|K♦|";
	a1 = "|Q♠|A♥|J♥|A♦|T♣|";
	pull = _engine.pull_trump_cards(a1, p1);
	assert(pull == "|T♣|");
	p1 = "|J♠|T♥|K♥|Q♥|K♣|";
	a1 = "|Q♠|A♥|J♥|A♦|T♣|";
	pull = _engine.pull_trump_cards(a1, p1);
	assert(pull == "|A♦|");
	_game.cards = temp;

	_game.trump = SPADE;
	Cards p2("|A♠|Q♥|Q♦|Q♣|J♣|");
	assert(_engine.lowest_card(p2, true) == 4);
	p2 = "|J♠|Q♥|Q♦|Q♣|J♣|";
	assert(_engine.lowest_card(p2, true) == 4);
	p2 = "|J♠|Q♥|Q♦|Q♣|K♣|";
	assert(_engine.lowest_card(p2, false) == 0);
	_game.cards = temp;

	assert(_engine.cards_to_claim(Cards("|T♥|K♥|Q♣|"), Cards("|A♠|T♠|K♠|Q♠|A♥|T♦|A♣|T♣|K♣|J♣|"), CLUB) == Cards());
	assert(_engine.cards_to_claim(Cards("|T♥|K♥|Q♣|"), Cards("|A♠|T♠|K♠|Q♠|A♥|T♦|J♣|"), CLUB) == Cards("|Q♣|"));
	assert(_engine.cards_to_claim(Cards("|T♥|K♥|A♣|Q♣|"), Cards("|A♠|T♠|K♠|Q♠|A♥|T♦|K♣|J♣|"), CLUB) == Cards("|A♣|"));
	assert(_engine.cards_to_claim(Cards("|T♥|K♥|Q♣|A♣|"), Cards("|A♠|T♠|K♠|Q♠|A♥|T♦|K♣|J♣|"), CLUB) == Cards("|A♣|"));
	assert(_engine.cards_to_claim(Cards("|T♥|K♥|A♣|K♣|"), Cards("|A♠|T♠|K♠|Q♠|A♥|T♦|J♣|"), CLUB) == Cards("|A♣|"));
	assert(_engine.cards_to_claim(Cards("|T♥|A♥|A♣|K♣|"), Cards("|A♠|K♠|Q♠|Q♥|T♦|J♣|")) == Cards("|A♥|A♣|"));
	assert(_engine.cards_to_claim(Cards("|T♥|A♦|A♣|K♣|"), Cards("|A♠|K♠|Q♠|Q♥|T♦|J♣|")) == Cards("|A♦|A♣|T♥|"));
//	assert(_engine.cards_to_claim(Cards("|A♠|Q♠|A♥|T♥|Q♥|"), Cards("|K♠|J♠|T♦|A♣|T♣|K♣|Q♣|J♣|")) == Cards("|A♠|Q♠|")); // WRONG
	assert(_engine.cards_to_claim(Cards("|A♠|Q♠|A♥|T♥|Q♥|"), Cards("|K♠|J♠|T♦|A♣|T♣|K♣|Q♣|J♣|")) == Cards("|A♠|"));	// OK
	assert(_engine.cards_to_claim(Cards("|T♠|Q♠|A♣|"), Cards("|A♠|K♠|Q♥|")) == Cards());

	_game.trump = SPADE;
	Cards acards("|T♦|K♦|J♦|T♣|K♣|");
	Cards pcards("|A♦|Q♦|T♥|A♣|J♣|");
	Cards move = _engine.closed_lead_no_trick(acards, pcards);
	assert(move == "|K♦|K♣|");
	pcards = "|A♦|Q♦|T♥|A♥|J♣|";
	move = _engine.closed_lead_no_trick(acards, pcards);
	assert(move == "|K♦|");
	pcards = "|A♦|Q♦|T♥|Q♣|J♣|"; // not allowed because trick possible
	move = _engine.closed_lead_no_trick(acards, pcards);
	assert(move == "|K♦|K♣|");	// nevertheless ok

	Cards tcards("|T♦|K♦|J♦|T♣|K♦|K♣|");
	tcards &= Card(KING, DIAMOND); // remove (all instances of this) card from set
	assert(tcards == "|T♦|J♦|T♣|K♣|");

	tcards = "|T♦|K♦|J♦|T♣|K♦|T♣|K♣|";
	tcards &= Cards("|K♦|T♣|"); // remove (all instances of this) cards from set
	assert(tcards == "|T♦|J♦|K♣|");

	tcards = "|T♦|J♦|K♦|T♣|K♣|";
	tcards -= Cards("|K♦|T♣|"); // remove single cards from set
	assert(tcards == "|T♦|J♦|K♣|");

	_game.trump = trump;
	LOG("Unittests run successfully.\n");
	return true;
}
