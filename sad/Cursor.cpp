#include <string>
#include <iostream>
#include "Cursor.h"

std::ostream& operator<<(std::ostream& os, const TextBuffer& tb) {
	os << "TextBuffer(";
	for (int i = 0;i < tb.size();i++) {
		os << "\"" << tb[i] << "\"";

		if (i < tb.size() - 1) os << ", ";
	}
	os << ")";
	return os;
}

IVec2::IVec2() { x = 0;y = 0; }

IVec2::IVec2(size_t x, size_t y) {
	this->x = x;
	this->y = y;
}

bool IVec2::operator==(const IVec2& that) const {
	return this->x == that.x && this->y == that.y;
}

IVec2 IVec2::getGhotsPos(const TextBuffer& buffer)  const {
	size_t y = std::min(buffer.size() - 1, this->y);
	size_t x = std::min(buffer[y].size(), this->x);
	return { x, y };
}

void IVec2::syncCursor(const TextBuffer& buffer) {
	this->y = std::min(buffer.size() - 1, this->y);
	this->x = std::min(buffer[y].size(), this->x);
}

char IVec2::getPrev(const TextBuffer& buffer) const {
	if (this->x == 0 && this->y == 0) return 0;
	if (this->x == 0) return '\n';

	return buffer[y][x - 1];
}
char IVec2::getNext(const TextBuffer& buffer) const {
	if (this->y == buffer.size() - 1 && this->x == buffer[this->y].size()) return 0;
	if (this->x == buffer[this->y].size()) return '\n';

	return buffer[y][x];
}


bool IVec2::up(const TextBuffer& buffer) {
	if (this->y > 0) {
		this->y--;
		return true;
	}
	else if (this->x != 0) {
		this->x = 0;
		return true;
	}
	return true;
}

bool IVec2::down(const TextBuffer& buffer) {
	if (this->y < buffer.size() - 1) {
		this->y++;
		return true;
	}
	else if (this->x != buffer[buffer.size() - 1].size()) {
		this->x = buffer[buffer.size() - 1].size();
		return true;
	}
	return false;
}

bool IVec2::left(const TextBuffer& buffer) {
	this->syncCursor(buffer);

	if (this->x > 0) {
		this->x--;
		return true;
	}
	else if (this->y > 0) {
		this->y--;
		this->x = buffer[this->y].size();
		return true;
	}
	return false;
}

bool IVec2::right(const TextBuffer& buffer) {
	this->syncCursor(buffer);

	if (this->x < buffer[this->y].size()) {
		this->x++;
		return true;
	}
	else if (this->y < buffer.size() - 1) {
		this->x = 0;
		this->y++;
		return true;
	}
	return false;
}

Cursor::Cursor(size_t cx, size_t cy) {
	this->start = IVec2(cx, cy);
	this->end = IVec2(cx, cy);
}

Cursor::Cursor() {
	this->start = IVec2();
	this->end = IVec2();
}

bool Cursor::isSelection() const {
	return !(start == end);
}

IVec2 Cursor::selectionStart(const TextBuffer& buffer) const {
	IVec2 selStart = this->start.getGhotsPos(buffer);
	IVec2 selEnd = this->end.getGhotsPos(buffer);

	if (!this->isSelection()) return selStart;

	if (selStart.y == selEnd.y) return selStart.x < selEnd.x ? selStart : selEnd;

	return selStart.y < selEnd.y ? selStart : selEnd;
}

IVec2 Cursor::selectionEnd(const TextBuffer& buffer) const {
	IVec2 selStart = this->start.getGhotsPos(buffer);
	IVec2 selEnd = this->end.getGhotsPos(buffer);

	if (!this->isSelection()) return selStart;

	if (selStart.y == selEnd.y) return selStart.x > selEnd.x ? selStart : selEnd;

	return selStart.y > selEnd.y ? selStart : selEnd;
}

bool Cursor::up(const TextBuffer& buffer) {
	return this->start.up(buffer) && this->end.up(buffer);
}
bool Cursor::down(const TextBuffer& buffer) {
	return this->start.down(buffer) && this->end.down(buffer);
}
bool Cursor::left(const TextBuffer& buffer) {
	return this->start.left(buffer) && this->end.left(buffer);
}
bool Cursor::right(const TextBuffer& buffer) {
	return this->start.right(buffer) && this->end.right(buffer);
}

void Cursor::collapseToSelectionStart(const TextBuffer& buffer) {
	IVec2 selStart = this->selectionStart(buffer);

	this->start = selStart;
	this->end = selStart;
}

void Cursor::collapseToSelectionEnd(const TextBuffer& buffer) {
	IVec2 selEnd = this->selectionEnd(buffer);

	this->start = selEnd;
	this->end = selEnd;
}
