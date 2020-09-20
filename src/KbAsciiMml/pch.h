#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
// Windows 7 Level
#define WINVER 0x0601
#define _WIN32_WINNT 0x0601
#include <Windows.h>
#include <shlwapi.h>

#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_file_iterator.hpp>
#include <boost/spirit/include/classic_symbols.hpp>
#include <boost/spirit/include/classic_utility.hpp>
#include <cctype>
#include <cmath>
#include <exception>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <sstream>
#include <stack>
#include <string>
#include <vector>
