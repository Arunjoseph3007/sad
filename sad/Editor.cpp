#include <string>
#include <assert.h>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <cctype>
#include "Editor.h"
#include "imgui.h"

enum CharType {
	Alphabet,
	Number,
	SpecialChar
};

static CharType getCharType(const char& c) {
	if (c >= 'a' && c <= 'z') return CharType::Alphabet;
	if (c >= 'A' && c <= 'Z') return CharType::Alphabet;
	if (c >= '0' && c <= '9') return CharType::Number;
	return CharType::SpecialChar;
}

static inline TextBuffer splitString(const std::string& text, const char& delimiter = '\n') {
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

static Edit diffBuffers(const TextBuffer& a, const TextBuffer& b) {
	if (a == b) return Edit(0, {}, {});

	size_t start, offset;
	for (start = 0;start < a.size() && start < b.size();start++) {
		if (a[start] != b[start]) break;
	}

	for (offset = 0;offset < a.size() && offset < b.size();offset++) {
		if (a[a.size() - 1 - offset] != b[b.size() - 1 - offset]) break;
	}
	TextBuffer plus(b.begin() + start, b.end() - offset);
	TextBuffer minus(a.begin() + start, a.end() - offset);
	return Edit(start, plus, minus);
}

static void debugCur(Cursor c) {
	printf("x: %d, y: %d, xp: %d, yp: %d\n", c.start.x, c.start.y, c.end.x, c.end.y);
}

Editor::Editor() {
	this->cursor = Cursor(0, 0);
	this->buffer = { "" };
}

bool Editor::startTransaction() {
	if (this->transactionRefCount == 0) {
		this->oldBuffer = this->buffer;
		this->transactionRefCount++;
		return true;
	}
	this->transactionRefCount++;
	return false;
}

bool Editor::endTransaction() {
	this->transactionRefCount--;

	if (this->transactionRefCount > 0) return false;

	this->redoHistory.clear();

	Edit ed = diffBuffers(this->oldBuffer, this->buffer);
	this->undoHistory.push_back(ed);
	this->oldBuffer.clear();

	while (this->undoHistory.size() > MAX_UNDO_HISTORY_SIZE) {
		this->undoHistory.pop_front();
	}
	return true;
}

bool Editor::undo() {
	if (this->undoHistory.empty()) return false;

	Edit ed = this->undoHistory.back();
	this->undoHistory.pop_back();
	this->redoHistory.push_back(ed);

	ed.undo(this->buffer);
	return true;
}

bool Editor::redo() {
	if (this->redoHistory.empty()) return false;

	Edit ed = this->redoHistory.back();
	this->redoHistory.pop_back();
	this->undoHistory.push_back(ed);

	ed.redo(this->buffer);
	return true;
}

IVec2 Editor::getCursorEnd() {
	return this->cursor.end;
}
IVec2 Editor::getGhostEnd() {
	return this->cursor.end.getGhotsPos(this->buffer);
}
IVec2 Editor::getCursorStart() {
	return this->cursor.start;
}
IVec2 Editor::getGhostStart() {
	return this->cursor.start.getGhotsPos(this->buffer);
}


void Editor::syncCusrorEnd() {
	this->cursor.end.syncCursor(this->buffer);
}
void Editor::syncCusrorStart() {
	this->cursor.start.syncCursor(this->buffer);
}

bool Editor::up() {
	if (this->cursor.isSelection()) {
		this->cursor.collapseToSelectionStart(this->buffer);
		return true;
	}
	return cursor.end.up(buffer) && cursor.start.up(buffer);
}

bool Editor::down() {
	if (this->cursor.isSelection()) {
		this->cursor.collapseToSelectionEnd(this->buffer);
		return true;
	}
	return cursor.end.down(buffer) && cursor.start.down(buffer);
}

bool Editor::left() {
	if (this->cursor.isSelection()) {
		this->cursor.collapseToSelectionStart(this->buffer);
		return true;
	}
	return cursor.end.left(buffer) && cursor.start.left(buffer);
}

bool Editor::right() {
	if (this->cursor.isSelection()) {
		this->cursor.collapseToSelectionEnd(this->buffer);
		return true;
	}
	return cursor.end.right(buffer) && cursor.start.right(buffer);
}

void Editor::leftWord() {
	if (this->cursor.isSelection()) {
		this->cursor.collapseToSelectionStart(this->buffer);
	}

	if (!this->left()) return;

	while (getCharType(this->cursor.start.getPrev(this->buffer)) == CharType::Alphabet) {
		if (!this->left()) break;
	}
}

void Editor::rightWord() {
	if (this->cursor.isSelection()) {
		this->cursor.collapseToSelectionEnd(this->buffer);
	}

	if (!this->right()) return;

	while (getCharType(this->cursor.start.getNext(this->buffer)) == CharType::Alphabet) {
		if (!this->right()) break;
	}
}

void Editor::selectUp() {
	this->cursor.end.up(this->buffer);
}
void Editor::selectDown() {
	this->cursor.end.down(this->buffer);
}
void Editor::selectLeft() {
	this->cursor.end.left(this->buffer);
}
void Editor::selectRight() {
	this->cursor.end.right(this->buffer);
}
std::string Editor::getSelectionString() const {
	IVec2 selStart = this->cursor.selectionStart(this->buffer);
	IVec2 selEnd = this->cursor.selectionEnd(this->buffer);

	if (selStart.y == selEnd.y) {
		return this->buffer[selStart.y].substr(selStart.x, selEnd.x - selStart.x);
	}
	else {
		std::string result;
		result += this->buffer[selStart.y].substr(selStart.x);
		result += '\n';
		for (int i = selStart.y + 1;i < selEnd.y - 1;i++) {
			result += this->buffer[i];
			result += '\n';
		}
		result += this->buffer[selEnd.y].substr(0, selEnd.x);
		return result;
	}
}

void Editor::emptySelection() {
	IVec2 selStart = this->cursor.selectionStart(this->buffer);
	IVec2 selEnd = this->cursor.selectionEnd(this->buffer);

	this->cursor.collapseToSelectionStart(this->buffer);

	if (selStart.y == selEnd.y) {
		this->buffer[selStart.y].erase(selStart.x, selEnd.x - selStart.x);
	}
	else {
		this->buffer[selStart.y].erase(selStart.x, this->buffer[selStart.y].size() - selStart.x);
		this->buffer[selStart.y] += this->buffer[selEnd.y].substr(selEnd.x);
		this->buffer.erase(this->buffer.begin() + selStart.y + 1, this->buffer.begin() + selEnd.y + 1);
	}
}


void Editor::insertBefore(const char c) {
	this->startTransaction();

	if (this->cursor.isSelection()) {
		this->emptySelection();
	}
	this->syncCusrorEnd();

	IVec2 gPos = this->getGhostEnd();

	this->buffer[gPos.y].insert(gPos.x, 1, c);

	this->cursor.end.right(this->buffer);
	this->cursor.start.right(this->buffer);

	this->endTransaction();
}

void Editor::insertBefore(const std::string& text) {
	TextBuffer segments = splitString(text);
	for (size_t i = 0;i < segments.size();i++) {
		for (const char& c : segments[i]) {
			this->insertBefore(c);
		}
		if (i != segments.size() - 1) this->enter();
	}
}

void Editor::insertAfter(const char c) {
	if (this->cursor.isSelection()) {
		this->emptySelection();
	}
	this->syncCusrorEnd();
	IVec2 gPos = this->getGhostEnd();

	this->buffer[gPos.y].insert(gPos.x, 1, c);
}

void Editor::insertAfter(const std::string& text) {
	TextBuffer segments = splitString(text);
	for (const std::string segment : segments) {
		for (const char& c : segment) {
			this->insertAfter(c);
		}
		this->enter();
	}
}

std::unordered_map<ImGuiKey, std::pair<char, char>> SpecialKeyMapping = {
	{ImGuiKey_Apostrophe,   {'\'',  '"'}},
	{ImGuiKey_Comma,        {',',   '<'}},
	{ImGuiKey_Minus,        {'-',   '_'}},
	{ImGuiKey_Period,       {'.',   '>'}},
	{ImGuiKey_Slash,        {'/',   '?'}},
	{ImGuiKey_Semicolon,    {';',   ':'}},
	{ImGuiKey_Equal,        {'=',   '+'}},
	{ImGuiKey_LeftBracket,  {'[',   '{'}},
	{ImGuiKey_Backslash,    {'\\',  '|'}},
	{ImGuiKey_RightBracket, {']',   '}'}},
	{ImGuiKey_GraveAccent,  {'`',   '~'}},
	// Numbered
	{ImGuiKey_0, {'0', ')'}},
	{ImGuiKey_1, {'1', '!'}},
	{ImGuiKey_2, {'2', '@'}},
	{ImGuiKey_3, {'3', '#'}},
	{ImGuiKey_4, {'4', '$'}},
	{ImGuiKey_5, {'5', '%'}},
	{ImGuiKey_6, {'6', '^'}},
	{ImGuiKey_7, {'7', '&'}},
	{ImGuiKey_8, {'8', '*'}},
	{ImGuiKey_9, {'9', '('}},
};

std::unordered_map<char, char> ClosablesChars = {
	{'\'', '\''},
	{'"', '"'},
	{'(', ')'},
	{'[', ']'},
	{'{', '}'},
};
void Editor::charInsertBefore(int ch, bool shift) {
	//assert(ch >= ImGuiKey_Apostrophe && ch <= ImGuiKey_GraveAccent && "Invalid character");

	std::pair<char, char> cPair = SpecialKeyMapping[(ImGuiKey)ch];
	char c = shift ? cPair.second : cPair.first;
	this->insertBefore(c);

	if (ClosablesChars.find(c) == ClosablesChars.end()) return;

	this->insertAfter(ClosablesChars[c]);
}

// TODO remove closing brackets/quotes/braces... if empty
bool Editor::backspace() {
	if (this->cursor.isSelection()) {
		this->emptySelection();

		return true;
	}
	IVec2 gPos = this->getGhostEnd();
	this->syncCusrorEnd();

	if (gPos.x > 0) {
		this->cursor.end.x--;
		this->cursor.start.x--;
		this->buffer[gPos.y].erase(gPos.x - 1, 1);

		return true;
	}
	else if (gPos.y > 0) {
		this->cursor.end.y--;
		this->cursor.start.y--;
		this->cursor.end.x = this->buffer[this->cursor.end.y].size();
		this->cursor.start.x = this->buffer[this->cursor.end.y].size();

		this->buffer[this->cursor.end.y] += this->buffer[this->cursor.end.y + 1];
		this->buffer.erase(this->buffer.begin() + this->cursor.end.y + 1);
		return true;
	}
	return false;
}

bool Editor::eDelete() {
	if (this->cursor.isSelection()) {
		this->emptySelection();
		return true;
	}
	IVec2 gPos = this->getGhostEnd();
	this->syncCusrorEnd();

	if (gPos.x < this->buffer[gPos.y].size()) {
		this->buffer[gPos.y].erase(gPos.x, 1);

		return true;
	}
	else if (gPos.y < this->buffer.size() - 1) {
		this->buffer[this->cursor.end.y] += this->buffer[this->cursor.end.y + 1];
		this->buffer.erase(this->buffer.begin() + this->cursor.end.y + 1);
		return true;
	}
	return false;
}

void Editor::enter() {
	if (this->cursor.isSelection()) {
		this->emptySelection();
	}
	IVec2 gPos = this->getGhostEnd();

	std::string before = this->buffer[gPos.y].substr(0, gPos.x);
	std::string after = this->buffer[gPos.y].substr(gPos.x);

	this->buffer[gPos.y] = before;
	this->buffer.insert(this->buffer.begin() + gPos.y + 1, after);

	this->cursor.end.y++;
	this->cursor.start.y++;
	this->cursor.end.x = 0;
	this->cursor.start.x = 0;
}

bool Editor::home() {
	if (this->cursor.end.x == 0) {
		return false;
	}
	this->cursor.end.x = 0;
	this->cursor.start.x = 0;
	return true;
}
bool Editor::end() {
	if (this->cursor.end.x == this->buffer[this->cursor.end.y].size()) {
		return false;
	}
	this->cursor.end.x = this->buffer[this->cursor.end.y].size();
	this->cursor.start.x = this->buffer[this->cursor.end.y].size();
	return true;
}

void Editor::debug() {
	std::cout << "--------------------------\n";
	for (const std::string& l : buffer) std::cout << l.size() << " : " << l << "\n";
	std::cout << "--------------------------\n";
}
