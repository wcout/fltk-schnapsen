//
// Part of "Schnapsen for 2" card game.
//
// (c) 2026 Christian Grabner
//
// Implements idea of animating text by
// letting appear the characters one by one.
//
#include "AnimText.h"
#include <FL/Fl.H>
#include <FL/fl_utf8.h> // fl_utf8len1()

#ifdef STANDALONE
class UI
{
public:
	UI() {}
	virtual void update() {}
};
#endif

AnimText::AnimText(const std::string &m_, UI &ui_, double speed_/* = 1./60*/) :
	_m(m_), _ui(ui_), _speed(speed_), _display_len(0), _needs_update(true)
{
	Fl::add_timeout(_speed, cb_anim, this);
}

AnimText::~AnimText()
{
	Fl::remove_timeout(cb_anim, this);
}

void AnimText::on_animate()
{
	if (!done())
	{
		_display_len++;
		Fl::repeat_timeout(_speed, cb_anim, this);
		_needs_update = true;
		_ui.update();
	}
}

std::string &AnimText::text()
{
	if (_needs_update == false)
		return _res; // string already "cached"

	std::string m(_m);
	// replace all ^|xxx| seq. with '~'
	size_t pos = m.find("^|");
	while (pos != std::string::npos)
	{
		size_t end = m.find('|', pos + 2);
		if (end == std::string::npos)
		{
			m.erase(pos);
		}
		else
		{
			m.erase(pos, end - pos + 1);
			m.insert(pos, std::string(end - pos + 1, '~'));
		}
		pos = m.find("^|");
	}
	// replace all ^n seq. with '~'
	pos = m.find('^');
	while (pos != std::string::npos)
	{
		if (pos + 1 == m.size())
			m.erase(pos);
		else
		{
			m[pos] = '~';
			m[pos + 1] = '~';
		}
		pos = m.find('^');
	}
	// calc. utf8 length of _display_len
	size_t utf8_len = 0;
	size_t len = _display_len;
	while (len > 0 && utf8_len < m.size())
	{
		utf8_len += fl_utf8len1(m[utf8_len]);
		while (m[utf8_len] == '~')
			utf8_len++;
		len--;
	}
	while (m[utf8_len] == '~')
		utf8_len++;

	_res = _m.substr(0, utf8_len);
	_needs_update = false;
	return _res;
}

bool AnimText::done() const
{
	return _display_len >= _m.size();
}





#ifdef STANDALONE
#undef STANDALONE

#include <cstdio>
class Test : public UI
{
public:
	Test() : _anim(nullptr) {}
	void update() override
	{
		printf("\t'%s'\n", _anim->text().c_str());
	}
	void text(const std::string &text_, bool wait_ = true)
	{
		_text = text_;
		delete _anim;
		printf("Test '%s'\n", text_.c_str());
		_anim = new AnimText(text_, *this);
		if (wait_)
			wait();
	}
	void wait()
	{
		while (_anim->done() == false) Fl::check();
		if ( _anim->text() == _text)
			printf("\033[0;32m==OK==\033[0m\n\n");
		else
			printf("\033[0;31m***FAILURE***\033[0m\n\n");
	}
private:
	AnimText *_anim;
	std::string _text;
};

int main()
{
	Test test;
	test.text("Das ist ein Test");
	test.text("Das ist ein ^|Test"); // incomplete seq. will be truncated
	test.text("Das ^bist^@ ein Test ^"); // dito
	test.text("Das ist ein Test ^|1234|");
	test.text("👍Das ist ein 😴 Test");
	test.text("Das ist ^|9999| ein Test ^|1234|");
	Test test2;
	test2.text("Ümläute soweiß daß Äuge räucht!\360\237\230\204");
}

#endif
