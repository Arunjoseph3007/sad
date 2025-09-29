#include "Common.h"
#include <iostream>

TextBuffer splitString(const std::string& text, const char& delimiter) {
	TextBuffer result;
	std::string segment;
	for (size_t i = 0; i < text.size(); i++) {
		if (text[i] == delimiter) {
			result.push_back(segment);
			segment.clear();
			continue;
		}
		segment += text[i];
	}
	result.push_back(segment);

	return result;
}

CharType getCharType(const char& c) {
	if (c >= 'a' && c <= 'z') return CharType::Alphabet;
	if (c >= 'A' && c <= 'Z') return CharType::Alphabet;
	if (c >= '0' && c <= '9') return CharType::Number;
	return CharType::SpecialChar;
}



void todo(std::string name) {
	std::cout << "[TODO]: Unimplmented method " << name << " called\n";
	exit(1);
}

void unreachable(std::string name) {
	std::cout << "[UNREACHABLE]: Reached unreachable code " << name << "\n";
	exit(1);
}

void hack(std::string name) {
	std::cout << "[HACK]: this is a temporary hack for problem (" << name << ")\n";
}
