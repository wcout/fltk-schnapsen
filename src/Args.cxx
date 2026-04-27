//
// Command line argument handling
//
#include "Args.h"

#include "Card.h"
#include "Util.h"

#include <filesystem>
#include <FL/fl_ask.H>

using namespace Schnapsen;

/*static*/
std::string Args::arg0;


namespace {

void list_cardbacks(std::ostringstream &os_, int id_ = 0)
{
	std::filesystem::path back(Util::home_dir() + cardDir + "/back");
	if (!id_)
		os_ << "\navailaible card backs (--cardback):\n";
	int id = 0;
	for (auto const &dir_entry : std::filesystem::directory_iterator(back))
	{
		++id;
		if (dir_entry.is_regular_file())
		{
			if (id_ && id_ == id)
				os_ << dir_entry.path().filename().c_str(); // NOTE: c_str() to get name unquoted
			else if (!id_)
				os_ << "[" << id << "]" << "\t" << dir_entry.path().filename() << "\n";
		}
	}
}

void list_cardsets(std::ostringstream &os_, int id_ = 0)
{
	std::string svg_cards(Util::home_dir() + cardDir);
	if (!id_)
		os_ << "\navailaible cardsets (--cardset):\n";
	int id = 0;
	for (auto const &dir_entry : std::filesystem::directory_iterator(svg_cards))
	{
		std::filesystem::path card(dir_entry.path());
		card /= Card(QUEEN, HEART).filename();
		std::filesystem::path card_png(dir_entry.path());
		card_png /= Card(QUEEN, HEART).filename(".png");
		if (dir_entry.is_directory() &&
			(std::filesystem::exists(card) || std::filesystem::exists(card_png)))
		{
			++id;
			if (id_ && id_ == id)
				os_ << dir_entry.path().filename().c_str(); // NOTE: c_str() to get name unquoted
			else if (!id_)
				os_ << "[" << id << "]"<< "\t" << dir_entry.path().filename() << "\n";
		}
	}
}

std::string make_help(const string_map &la_, const string_map &sa_, bool list_ = false)
{
	std::ostringstream os;
	if (list_ == false)
	{
		os << APPLICATION << " " << VERSION << "\n\n";
		os << "Usage:\n";
		for (const auto &[option, description] : la_)
		{
			os << "--" << option << "\t" << description << "\n";
		}
		os << "\n";
		for (const auto &[option, description] : sa_)
		{
			os << "-" << option << "\t" << description << "\n";
		}
	}
	else
	{
		list_cardsets(os);
		list_cardbacks(os);
		os << "\nSpecify name or [id] in command line";
	}
	return os.str();
}

void fix_name(std::string &name_, void (*f_)(std::ostringstream&, int))
{
	if (std::isdigit(name_[0]))
	{
		int id = std::atoi(name_.c_str());
		if (id == 0)
			name_ = "default";
		else if (id > 0)
		{
			std::ostringstream os;
			f_(os, id);
			std::string name = os.str();
			name_ = name;
		}
	}
}

bool process(const std::string &arg_, const std::string &value_)
{
	std::string value(value_);
	static const string_map long_args =
	{
		{ "strict", "{strictness}\tconfirm strict(er) to the rules [0-2] 0=off, 2=all" },
		{ "animate", "{level}\t\tanimate card moves [0-2] 0=off, 2=all" },
		{ "cards", "\t{cards-string}\tuse this cards to play (for debugging only)" },
		{ "cardback", "{file}\t\tuse cardback image [svg]" },
		{ "cardset", "{directory}\tuse cardset [name]" },
		{ "background", "{name/number}\tset background image or color [imagepath/[0-255]]" },
		{ "loglevel", "{level}\t\tset loglevel [0-2]" },
		{ "lang", "\t{id}\t\tset language [de,en]" }
	};
	static const string_map short_args =
		{
		{ "h", "this help" },
		{ "l", "list available cardsets/cardbacks" },
		{ "v", "display version" },
		{ "R", "random first move" },
		{ "S", "draw text with shadow" },
		{ "s", "faster response" },
		{ "r", "start with AI move" },
		{ "t", "sort trumps to begin" },
		{ "w", "show welcome screen" },
		{ "f", "run fullscreen" },
		{ "d", "enable debug" },
		{ "C", "default config values" }
	};

	auto find = [](const std::string& arg_, const string_map& list_) -> std::string
	{
		std::string arg;
		int unique = 0;
		for (auto &a : list_)
		{
			if (a.first.find(arg_) == 0)
			{
				arg = a.first;
				unique++;
			}
		}
		return unique == 1 ? arg : "";
	};

	auto check_short_arg = [](const std::string& arg_, const string_map& list_) -> int
	{
		if (list_.find(arg_.substr(1, 1)) != list_.end())
		{
			int repeat = 0;
			std::string arg(arg_.substr(1));
			// count how often first letter is repeated
			for (size_t i = 1; i < arg.size(); i++)
			{
				if (arg[i] != arg[0])
					return 0; // must be same letter
				repeat++;
			}
			return repeat + 1;
		}
		return 0;
	};

	std::string arg = find(arg_.substr(2), long_args); // accept incomplete arg specification
	if (value.size() && arg.size())
	{
		if (arg == "cardset") fix_name(value, list_cardsets);
		if (arg == "cardback") fix_name(value, list_cardbacks);
		Util::config(arg, value);
	}
	else if (arg_.size() >= 2 && arg_[0] == '-' && check_short_arg(arg_, short_args))
	{
		switch (arg_[1])
		{
			case 'd':
				debug += check_short_arg(arg_, short_args); // allow "-ddd" syntax
				break;
			case 'f':
				Util::config("fullscreen", "1");
				break;
			case 'r':
				first_to_move = AI;
				break;
			case 's':
				Util::config("fast", "1");
				break;
			case 't':
				Util::config("trump-sort", "1");
				break;
			case 'w':
				Util::config("welcome", "1");
				break;
			case 'h':
				OUT(make_help(long_args, short_args));
				OUT(make_help(long_args, short_args, true));
				return false;
			case 'l':
				fl_message_font_ = FL_COURIER;
				fl_alert("%s", make_help(long_args, short_args, true).c_str());
				return false;
			case 'v':
				OUT(APPLICATION << " " << VERSION << "\n");
				return false;
			case 'R':
				first_to_move = random() % 2 ? PLAYER : AI;
				break;
			case 'S':
				Util::config("text-shadow", "1");
				break;
			case 'C':
				Util::config().clear();
		}
	}
	else
	{
		fl_message_font_ = FL_COURIER;
		fl_alert("Invalid argument '%s'!\n\n%s", arg_.c_str(), make_help(long_args, short_args).c_str());
		return false;
	}
	return true;
}

}; // namespace

/*static*/
bool Args::parse(int argc_, char *argv_[])
{
	arg0 = argv_[0];
	for (int i = 1; i < argc_; i++)
	{
		std::string arg = argv_[i];
		std::string value;
		if (arg.starts_with("--"))
		{
			if (i + 1 < argc_)
			{
				i++;
				value = argv_[i];
			}
		}
		if (value.size() && value[0] == '-')
			value.erase();
		if (value.empty() && i + 1 == argc_ && arg[0] != '-')
		{
			game_to_load = arg;
			continue;
		}
		if (process(arg, value) == false)
		{
			return false;
		}
	}
	return true;
}
