#pragma once

#include <string>
#include <deque>
#include <iostream>

typedef std::deque<Card> Cards_;
typedef std::deque<CardSuite> Suites;

class Cards : public Cards_
{
public:
	Cards();
	Cards(const Cards_ &cards_);
	explicit Cards(const std::string &s_);
	Cards operator = (const std::string &s_);
	Cards operator += (const Cards &c_);
	Cards operator + (const Cards &c_) const;
	Cards operator -= (const Cards &c_);
	Cards operator - (const Cards &c_) const;
	Cards operator += (const Card &c_);
	Cards operator + (const Card &c_) const;
	Cards operator -= (const Card &c_);
	Cards operator - (const Card &c_) const;
	bool operator == (const std::string& s_);
	Cards &from_string(const std::string &s_);
	void check();
	std::optional<size_t> find_face(CardFace f_) const;
	std::optional<size_t> find_pos(const Card &c_) const;
	std::optional<Card> find(const Card &c_) const;
	void shuffle();
	void sort();
	void sort(const CardSuite trump_);
	void sort_by_value(bool high_to_low = true);
	int value() const;
	static Cards fullcards();
	virtual std::ostream &printOn(std::ostream &os_) const;
};
