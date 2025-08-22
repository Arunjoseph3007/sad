#pragma once
#include <string>
#include <vector>

typedef std::vector<std::string> TextBuffer;

struct Edit {
public:
	int start;
	TextBuffer plus, minus;

	Edit(int start, TextBuffer plus, TextBuffer minus) : start(start), plus(plus), minus(minus) {}

	void undo(TextBuffer& buffer) const;
	void redo(TextBuffer& buffer) const;
};

std::ostream& operator<<(std::ostream& os, const Edit& obj);