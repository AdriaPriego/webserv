
#pragma once

#include <vector>
#include <iostream>
#include <sstream>

#define KO -1
#define NOT_INIT 0
#define MISS_KEY 1
#define OPEN_KEY 2
#define COMENT 3
#define EMPTY 4
#define CLOSE_KEY 5
#define VALID_ARG 10

#define BYTES 0
#define KILOS 1
#define MEGAS 2
#define GIGAS 3

std::vector<std::string> split(const std::string& s, char delimiter);
bool isInt(const std::string & value);