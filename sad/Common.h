#pragma once
#include <string>
#include <vector>

typedef std::vector<std::string> TextBuffer;

TextBuffer splitString(const std::string& text, const char& delimiter = '\n');