#pragma once

#include <map>
#include <string>

enum class Message
{
	NO_MESSAGE,
	YOU_CHANGED,
	YOU_CLOSED,
	YOUR_GAME,
	YOUR_TRICK,
	YOUR_TURN,
	YOU_LEAD,
	YOU_NOT_ENOUGH,
	AI_CHANGED,
	AI_CLOSED,
	AI_GAME,
	AI_TRICK,
	AI_TURN,
	AI_LEADS,
	AI_NOT_ENOUGH,
	TRUMP,
	TITLE,
	GAMEBOOK,
	GB_HEADLINE,
	YOU_WIN,
	YOU_LOST,
	INVALID_SUITE,
	MUST_TRICK_WITH_SUITE,
	MUST_TRICK_WITH_TRUMP,
	NO_CLOSE,
	NO_CHANGE,
	REDEAL,
	WELCOME,
	GAMES_WON,
	MATCHES_WON,
	YOU_MARRIAGE_20,
	YOU_MARRIAGE_40,
	AI_MARRIAGE_20,
	AI_MARRIAGE_40,
	SHUFFLE,
	AI_SLEEP
};

using enum Message;

std::map<Message, std::string> messages_de = {
	{NO_MESSAGE, ""},
	{YOU_CHANGED, "Du hast den Buben getauscht"},
	{YOU_CLOSED, "Du hast zugedreht"},
	{YOUR_GAME, "👍Gratuliere, dein Spiel!"},
	{YOUR_TRICK, "Dein Stich"},
	{YOUR_TURN, "Du bist dran"},
	{YOU_LEAD, "Du spielst aus"},
	{YOU_NOT_ENOUGH, "Du hast nicht genug"},
	{AI_CHANGED, "AI hat den Buben getauscht"},
	{AI_CLOSED, "AI hat zugedreht"},
	{AI_GAME, "AI gewinnt das Spiel!"},
	{AI_TRICK, "AI hat gestochen"},
	{AI_TURN, "AI ist am Spiel ... 💭"},
	{AI_LEADS, "AI spielt aus ... 💭"},
	{AI_NOT_ENOUGH, "AI hat nicht genug"},
	{TRUMP, "Trumpf"},
	{TITLE, "Schnapsen zu zweit"},
	{GAMEBOOK, "**Spielebuch**"},
	{GB_HEADLINE, "  DU      AI"},
	{YOU_WIN, "♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦\n"
	          "♥                                  ♠\n"
	          "♣           Glückwunsch!           ♥\n"
	          "♦                                  ♣\n"
	          "♠                                  ♦\n"
	          "♥   Du hast die Partie gewonnen!   ♠\n"
	          "♣                                  ♥\n"
	          "♦                                  ♣\n"
	          "♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦"},
	{YOU_LOST, "\n\nDie AI hat dir ein\nBummerl angehängt!\n\n"},
	{INVALID_SUITE, "Du must Farbe geben"},
	{MUST_TRICK_WITH_SUITE, "Du must mit Farbe stechen"},
	{MUST_TRICK_WITH_TRUMP, "Du must mit Atout stechen"},
	{NO_CLOSE, "Zudrehen nicht mehr erlaubt"},
	{NO_CHANGE, "Tauschen nicht mehr erlaubt"},
	{REDEAL, "MISCHEN"},
	{WELCOME, "Servas Oida!\n\nHast Lust auf\na Bummerl?"},
	{GAMES_WON, "Spiele gewonnen (PL/AI): "},
	{MATCHES_WON, "Partien: "},
	{AI_SLEEP, "😴!!"}
};

std::map<Message, std::string> messages_en = {
	{YOU_CHANGED, "You changed the jack"},
	{YOU_CLOSED, "You closed the game"},
	{YOUR_GAME, "👍Congrats, your game!"},
	{YOUR_TRICK, "Your trick"},
	{YOUR_TURN, "Your turn"},
	{YOU_LEAD, "Your lead"},
	{AI_CHANGED, "AI has changed the jack"},
	{AI_CLOSED, "AI has closed"},
	{AI_GAME, "AI wins the game!"},
	{AI_TRICK, "AI makes trick"},
	{AI_TURN, "AI playing ... 💭"},
	{AI_LEADS, "AI leading ... 💭"},
	{TRUMP, "Trump"},
	{TITLE, "Schnapsen for two"},
	{GAMEBOOK, "**Game book**"},
	{GB_HEADLINE, "  YOU     AI"},
	{YOU_WIN, "♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦\n"
	          "♥                                  ♠\n"
	          "♣       Wow, your lucky day!       ♥\n"
	          "♦                                  ♣\n"
	          "♠                                  ♦\n"
	          "♥   You have won the whole lot!    ♠\n"
	          "♣                                  ♥\n"
	          "♦                                  ♣\n"
	          "♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦♠♥♣♦"},
	{YOU_LOST, "\n\nYou got the bummerl!\n\n"},
	{INVALID_SUITE, "You must give suite"},
	{MUST_TRICK_WITH_SUITE, "You must trick with suite"},
	{MUST_TRICK_WITH_TRUMP, "You must trick with trump"},
	{NO_CLOSE, "You can't close any more"},
	{NO_CHANGE, "You can't change any more"},
	{REDEAL, "REDEAL"},
	{WELCOME, "Hey dude!\n\nDo you want\na 'bummerl'?"},
	{GAMES_WON, "Games won (PL/AI): "},
	{MATCHES_WON, "Matches: "},
	{AI_SLEEP, "😴!!"}
};

std::map<Message, std::string> sound = {
	{NO_MESSAGE, "ding"},
	{YOU_CHANGED, "change"},
	{AI_CHANGED, "change"},
	{YOU_CLOSED, "close"},
	{AI_CLOSED, "close"},
	{YOU_MARRIAGE_20, "marriage"},
	{YOU_MARRIAGE_40, "marriage"},
	{AI_MARRIAGE_20, "marriage"},
	{AI_MARRIAGE_40, "marriage"},
	{YOUR_GAME, "your_game"},
	{AI_GAME, "ai_game"},
	{YOU_WIN, "you_win"},
	{YOU_LOST, "you_lost"},
	{NO_CLOSE, "not_allowed"},
	{NO_CHANGE, "not_allowed"},
	{INVALID_SUITE, "not_allowed"},
	{MUST_TRICK_WITH_TRUMP, "not_allowed"},
	{MUST_TRICK_WITH_SUITE, "not_allowed"},
	{WELCOME, "welcome"},
	{SHUFFLE, "shuffle"}
};
