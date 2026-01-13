#pragma once

#include "Util.h"

#include <cstdlib>
#include <iostream>

// Very simple logging interface
std::string DBG_PREFIX = "\033[38;5;240m";  // light grey
std::string LOG_PREFIX = "\033[1m";         // bold
//std::string LOG_PREFIX = "\033[38;5;0m";    // black
std::string RESET_ATTR = "\033[0m";
#undef OUT
#define OUT(x) { std::cout << x; }
#define LOG(x) { if (atoi(Util::config("loglevel").c_str()) > 0) std::cout << LOG_PREFIX << x << RESET_ATTR; }
#define DBG(x) { if (atoi(Util::config("loglevel").c_str()) > 1) std::cout << DBG_PREFIX << x << RESET_ATTR; }
