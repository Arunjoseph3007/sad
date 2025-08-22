#pragma once
#include <vector>
#include <string>

typedef std::vector<std::string> TextBuffer;

std::ostream& operator<<(std::ostream& os, const TextBuffer& obj);

struct IVec2 {
public:
	int x, y;

	IVec2();
	IVec2(int x, int y);

	bool operator==(const IVec2& that) const;

	void syncCursor(TextBuffer buffer);
	IVec2 getGhotsPos(TextBuffer buffer) const;

	char getPrev(TextBuffer buffer) const;
	char getNext(TextBuffer buffer) const;

	bool up(TextBuffer buffer);
	bool down(TextBuffer buffer);
	bool left(TextBuffer buffer);
	bool right(TextBuffer buffer);
};

struct Cursor {
public:
	IVec2 start;
	IVec2 end;

	Cursor();
	Cursor(int x, int y);

	bool isSelection() const;
	IVec2 selectionStart(TextBuffer buffer) const;
	IVec2 selectionEnd(TextBuffer buffer) const;

	void collapseToSelectionStart(TextBuffer buffer);
	void collapseToSelectionEnd(TextBuffer buffer);
};
