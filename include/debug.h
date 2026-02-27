#pragma once

#include "Util.h"

#include <cstdlib>
#include <iostream>
#include <fstream>

// Very simple logging interface
constexpr auto DBG_PREFIX = "\033[38;5;240m";  // light grey
constexpr auto LOG_PREFIX = "\033[1m";         // bold
constexpr auto WNG_PREFIX = "\033[38;5;196m";  // bright red
constexpr auto RESET_ATTR = "\033[0m";
#undef OUT
#define OUT(x) { std::cout << x; }
#define LOG(x) { if (Util::logstream().good()) Util::logstream() << x; if (Util::config_as_int("loglevel") > 0) std::cout << LOG_PREFIX << x << RESET_ATTR; }
#define DBG(x) { if (Util::logstream().good()) Util::logstream() << x; if (Util::config_as_int("loglevel") > 1) std::cout << DBG_PREFIX << x << RESET_ATTR; }
#define WNG(x) { if (Util::logstream().good()) Util::logstream() << "!" << x << "\n"; std::cout << WNG_PREFIX << x << RESET_ATTR << "\n"; }
