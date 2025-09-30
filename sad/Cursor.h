#pragma once
#include <vector>
#include <string>
#include "Common.h"

std::ostream& operator<<(std::ostream& os, const TextBuffer& obj);

struct IVec2 {
public:
	size_t x, y;

	IVec2();
	IVec2(size_t x, size_t y);

	bool operator==(const IVec2& that) const;
	bool operator>(const IVec2& that) const;
	bool operator<(const IVec2& that) const;
	bool operator>=(const IVec2& that) const;
	bool operator<=(const IVec2& that) const;


	void syncCursor(const TextBuffer& buffer);
	IVec2 getGhotsPos(const TextBuffer& buffer) const;

	char getPrev(const TextBuffer& buffer) const;
	char getNext(const TextBuffer& buffer) const;

	bool up(const TextBuffer& buffer);
	bool down(const TextBuffer& buffer);
	bool left(const TextBuffer& buffer);
	bool right(const TextBuffer& buffer);
};

struct Cursor {
public:
	IVec2 start;
	IVec2 end;

	Cursor();
	Cursor(size_t x, size_t y);

	bool isSelection() const;
	IVec2 selectionStart(const TextBuffer& buffer) const;
	IVec2 selectionEnd(const TextBuffer& buffer) const;

	bool up(const TextBuffer& buffer);
	bool down(const TextBuffer& buffer);
	bool left(const TextBuffer& buffer);
	bool right(const TextBuffer& buffer);

	void collapseToSelectionStart(const TextBuffer& buffer);
	void collapseToSelectionEnd(const TextBuffer& buffer);
};
