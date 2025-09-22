#pragma once
#include <string>
#include <vector>

typedef std::vector<std::string> TextBuffer;

TextBuffer splitString(const std::string& text, const char& delimiter = '\n');

void todo(std::string name);

void unreachable(std::string name);

void hack(std::string name);


#define TODO() todo(__FUNCTION__)
#define UNREACHABLE() unreachable(__FUNCTION__)

#define repeat(n) for(size_t __repeat_iter_i = 0; __repeat_iter_i < n; __repeat_iter_i++)