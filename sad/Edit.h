#pragma once
#include <string>
#include <vector>
#include "Cursor.h"
#include "Common.h"
#include <deque>

struct Edit {
public:
	static Edit diffBuffers(const TextBuffer& a, const TextBuffer& b);

	int start;
	TextBuffer plus, minus;
	std::vector<Cursor> startCursors, endCursors;

	Edit(int start, TextBuffer plus, TextBuffer minus) : start(start), plus(plus), minus(minus) {}

	void undo(TextBuffer& buffer) const;
	void redo(TextBuffer& buffer) const;
};

typedef std::deque<Edit> History;

std::ostream& operator<<(std::ostream& os, const Edit& obj);