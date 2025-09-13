#include "Common.h"

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