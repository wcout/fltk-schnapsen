#include "Cards.h"

#include <sstream>
#include <algorithm>
#include <cassert>
#include <utility>
#include <random>

Cards::Cards() {}

Cards::Cards(const Cards_ &cards_)
{
	*this = cards_;
}

Cards::Cards(const Card &card_)
{
	this->clear();
	this->push_back(card_);
}

/*explicit*/
Cards::Cards(const std::string &s_)
{
	*this = from_string(s_);
}

Cards Cards::operator = (const std::string &s_)
{
	*this = from_string(s_);
	return *this;
}

Cards Cards::operator += (const Cards &c_)
{
	for (auto c : c_) push_back(c);
	return *this;
}

Cards Cards::operator + (const Cards &c_) const
{
	Cards res(*this);
	for (auto c : c_) res.push_back(c);
	return res;
}

Cards Cards::operator -= (const Cards &c_)
{
	for (auto &c : c_)
	{
		auto i = find_pos(c);
		if (i)
		{
			erase(begin() + i.value());
		}
	}
	return *this;
}

Cards Cards::operator &= (const Cards &c_)
{
	for (auto &c : c_)
	{
		while (1)
		{
			auto i = find_pos(c);
			if (i)
			{
				erase(begin() + i.value());
			}
			else break;
		}
	}
	return *this;
}

Cards Cards::operator |= (const Cards &c_)
{
	for (auto &c : c_)
	{
		while (1)
		{
			auto i = find_pos(c);
			if (i)
			{
				erase(begin() + i.value());
			}
			else break;
		}
		push_back(c);
	}
	return *this;
}

Cards Cards::operator - (const Cards &c_) const
{
	Cards res(*this);
	for (auto &c : c_)
	{
		auto i = res.find_pos(c);
		if (i)
		{
			res.erase(res.begin() + i.value());
		}
	}
	return res;
}

Cards Cards::operator += (const Card &c_)
{
	push_back(c_);
	return *this;
}

Cards Cards::operator + (const Card &c_) const
{
	Cards res(*this);
	res.push_back(c_);
	return res;
}

Cards Cards::operator -= (const Card &c_)
{
	auto i = find_pos(c_);
	if (i)
	{
		erase(begin() + i.value());
	}
	return *this;
}

Cards Cards::operator &= (const Card &c_)
{
	while (1)
	{
		auto i = find_pos(c_);
		if (i)
		{
			erase(begin() + i.value());
		}
		else break;
	}
	return *this;
}

Cards Cards::operator |= (const Card &c_)
{
	while (1)
	{
		auto i = find_pos(c_);
		if (i)
		{
			erase(begin() + i.value());
		}
		else break;
	}
	push_back(c_);
	return *this;
}

Cards Cards::operator - (const Card &c_) const
{
	Cards res(*this);
	auto i = res.find_pos(c_);
	if (i)
	{
		res.erase(res.begin() + i.value());
	}
	return res;
}

bool Cards::operator == (const std::string& s_)
{
	std::ostringstream os;
	printOn(os);
	return os.str() == s_;
}

Cards& Cards::from_string(const std::string &s_)
{
	// parse card-string in format: '|T♣|Q♦|T♦|Q♣|J♦|Q♠|T♠|Q♥|J♠|A♦|K♥|J♣|K♠|J♥|T♥|A♥|A♣|A♠|K♣|K♦|'
	Cards cards;
	std::string s(s_);
	while (s.size())
	{
		assert(s[0] == '|');
		s.erase(0, 1);
		size_t next_card = s.find("|");
		if (next_card == std::string::npos) break;
		std::string c = s.substr(0, next_card);
		assert(c.size() > 1);
		std::string face_str = c.substr(0, 1);
		std::string suite_str = c.substr(1);
		s.erase(0, next_card);
		for (auto &[suite, sym] : suite_symbols)
		{
			if (sym == suite_str)
			{
				for (auto &[face, fabbr] : face_abbrs)
				{
					if (fabbr == face_str)
					{
						cards.push_back(Card(face, suite));
						break;
					}
				}
			}
		}
	}
	*this = std::move(cards);
	return *this;
}

bool Cards::check()
{
	for (auto s : { SPADE, HEART, DIAMOND, CLUB } )
	{
		for (auto f : { JACK, QUEEN, KING, TEN, ACE } )
		{
			Card c(f, s);
			if (!find(c))
			{
				LOG("Card " << c << " not found!\n");
				return false;
			}
		}
	}
	return true;
}

std::optional<size_t> Cards::find_face(CardFace f_) const
{
	for (size_t i = 0; i < size(); i++)
	{
		if (at(i).face() == f_)
			return i;
	}
	return {};
}

std::optional<size_t> Cards::find_pos(const Card &c_) const
{
	for (size_t i = 0; i < size(); i++)
	{
		if (at(i).face() == c_.face() && at(i).suite() == c_.suite())
			return i;
	}
	return {};
}

std::optional<Card> Cards::find(const Card &c_) const
{
	auto card = find_pos(c_);
	if (card)
		return at(card.value());
	return {};
}

Cards& Cards::shuffle()
{
	LOG("shuffle\n");
	assert(size());
	std::string cards = Util::config()["cards"];
	if (cards.size() == 101)
	{
		from_string(cards);
		LOG("Using predefind card set!\n");
	}
	else
	{
		static std::random_device rd;
		static std::mt19937 gen(rd());
		std::shuffle(begin(), end(), gen);
	}
	check();
	return *this;
}

Cards& Cards::sort()
{
	auto sortRuleCards = [] (Card const &c1_, Card const &c2_) -> bool
	{
		if (c1_.suite() == c2_.suite()) return c1_.value() > c2_.value();
		return c1_.suite_weight() > c2_.suite_weight();
	};
	std::sort(begin(), end(), sortRuleCards);
	return *this;
}

Cards& Cards::sort(const CardSuite trump_)
{
	auto sortRuleCards = [&] (Card const &c1_, Card const &c2_) -> bool
	{
		if (c1_.suite() == c2_.suite()) return c1_.value() > c2_.value();
		int sw1 = c1_.suite_weight();
		int sw2 = c2_.suite_weight();
		if (c1_.suite() == trump_) sw1 *= 100;
		if (c2_.suite() == trump_) sw2 *= 100;
		return sw1 > sw2;
	};
	std::sort(begin(), end(), sortRuleCards);
	return *this;
}

Cards& Cards::sort_by_value(bool high_to_low/* = true*/)
{
	auto sortRuleCards = [&] (Card const &c1_, Card const &c2_) -> bool
	{
		return high_to_low ? (c1_.value() > c2_.value()) : (c2_.value() > c1_.value());
	};
	sort(); // first bring suites in defined order
	std::sort(begin(), end(), sortRuleCards);
	return *this;
}

int Cards::value() const
{
	int value = 0;
	for (auto &c : *this)
		value += c.value();
	return value;
}

/*static*/
Cards Cards::fullcards(CardSuite suite_/* = ANY_SUITE*/)
{
	Cards cards;
	for (auto s : { SPADE, HEART, DIAMOND, CLUB } )
	{
		if (suite_ != ANY_SUITE && s != suite_) continue;
		for (auto f : { JACK, QUEEN, KING, TEN, ACE } )
		{
			cards.push_back(Card(f, s));
			cards.back().load();
		}
	}
	assert(cards.size() == (suite_ == ANY_SUITE ? 20 : 5));
	return cards;
}

/*virtual*/
std::ostream& Cards::printOn(std::ostream &os_) const
{
	if (size())
		os_ << "|";
	for (auto c : *this)
	{
		os_ << c << "|";
	}
	return os_;
}

inline std::ostream &operator<<(std::ostream &os_, const Cards &cards_)
{
	return cards_.printOn(os_);
}
