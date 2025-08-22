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

IVec2::IVec2(int x, int y) {
	this->x = x;
	this->y = y;
}

bool IVec2::operator==(const IVec2& that) const {
	return this->x == that.x && this->y == that.y;
}

IVec2 IVec2::getGhotsPos(TextBuffer buffer)  const {
	int y = std::min((int)buffer.size(), this->y);
	int x = std::min((int)buffer[y].size(), this->x);
	return { x, y };
}

void IVec2::syncCursor(TextBuffer buffer) {
	this->y = std::min((int)buffer.size(), this->y);
	this->x = std::min((int)buffer[y].size(), this->x);
}

char IVec2::getPrev(TextBuffer buffer) const {
	if (this->x == 0 && this->y == 0) return 0;
	if (this->x == 0) return '\n';

	return buffer[y][x - 1];
}
char IVec2::getNext(TextBuffer buffer) const {
	if (this->y == buffer.size() - 1) {
		if (this->x == buffer[this->y].size() - 1) return 0;

		return '\n';
	}
	return buffer[y][x];
}


bool IVec2::up(TextBuffer buffer) {
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

bool IVec2::down(TextBuffer buffer) {
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

bool IVec2::left(TextBuffer buffer) {
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

bool IVec2::right(TextBuffer buffer) {
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

Cursor::Cursor(int cx, int cy) {
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

IVec2 Cursor::selectionStart(TextBuffer buffer) const {
	IVec2 selStart = this->start.getGhotsPos(buffer);
	IVec2 selEnd = this->end.getGhotsPos(buffer);

	if (!this->isSelection()) return selStart;

	if (selStart.y == selEnd.y) return selStart.x < selEnd.x ? selStart : selEnd;

	return selStart.y < selEnd.y ? selStart : selEnd;
}

IVec2 Cursor::selectionEnd(TextBuffer buffer) const {
	IVec2 selStart = this->start.getGhotsPos(buffer);
	IVec2 selEnd = this->end.getGhotsPos(buffer);

	if (!this->isSelection()) return selStart;

	if (selStart.y == selEnd.y) return selStart.x > selEnd.x ? selStart : selEnd;

	return selStart.y > selEnd.y ? selStart : selEnd;
}

void Cursor::collapseToSelectionStart(TextBuffer buffer) {
	IVec2 selStart = this->selectionStart(buffer);

	this->start = selStart;
	this->end = selStart;
}

void Cursor::collapseToSelectionEnd(TextBuffer buffer) {
	IVec2 selEnd = this->selectionEnd(buffer);

	this->start = selEnd;
	this->end = selEnd;
}
