#include <string>
#include <assert.h>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <cctype>
#include "Editor.h"
#include "imgui.h"
#include "Timer.h"

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

static void debugCur(Cursor c) {
	printf("x: %zu, y: %zu, xp: %zu, yp: %zu\n", c.start.x, c.start.y, c.end.x, c.end.y);
}

Editor::Editor() {
	this->cursors = { Cursor(0, 0) };
	this->buffer = { "" };

	this->buffer.reserve(1024);
}

std::string Editor::getText() const {
	std::string result;

	for (int i = 0;i < this->buffer.size();i++) {
		result += this->buffer[i];

		if (i != this->buffer.size() - 1) result += '\n';
	}

	return result;
}

bool Editor::startTransaction() {
	if (this->transactionRefCount == 0) {
		this->oldBuffer = this->buffer;
		this->oldCursors = this->cursors;
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

	Edit ed = Edit::diffBuffers(this->oldBuffer, this->buffer);
	ed.startCursors = this->oldCursors;
	ed.endCursors = this->cursors;
	this->undoHistory.push_back(ed);
	this->oldBuffer.clear();
	this->oldCursors = {};

	while (this->undoHistory.size() > MAX_UNDO_HISTORY_SIZE) {
		this->undoHistory.pop_front();
	}

	// after change re calculate tokens
	this->tokens = this->grammar.parseTextBuffer(this->buffer);

	return true;
}

bool Editor::undo() {
	if (this->undoHistory.empty()) return false;

	Edit ed = this->undoHistory.back();
	this->undoHistory.pop_back();
	// I dont think we need to check size here, 
	// it cannot grow longer than the undoHistory
	this->redoHistory.push_back(ed);

	ed.undo(this->buffer);
	this->cursors = ed.startCursors;

	// since text changed re calculate tokens
	this->tokens = this->grammar.parseTextBuffer(this->buffer);

	return true;
}

bool Editor::redo() {
	if (this->redoHistory.empty()) return false;

	Edit ed = this->redoHistory.back();
	this->redoHistory.pop_back();
	this->undoHistory.push_back(ed);

	ed.redo(this->buffer);
	this->cursors = ed.endCursors;

	// since text changed re calculate tokens
	this->tokens = this->grammar.parseTextBuffer(this->buffer);

	return true;
}

IVec2 Editor::getCursorEnd(size_t idx) {
	return this->cursors[idx].end;
}
IVec2 Editor::getGhostEnd(size_t idx) {
	return this->cursors[idx].end.getGhotsPos(this->buffer);
}
IVec2 Editor::getCursorStart(size_t idx) {
	return this->cursors[idx].start;
}
IVec2 Editor::getGhostStart(size_t idx) {
	return this->cursors[idx].start.getGhotsPos(this->buffer);
}


void Editor::syncCusrorEnd(size_t idx) {
	this->cursors[idx].end.syncCursor(this->buffer);
}
void Editor::syncCusrorStart(size_t idx) {
	this->cursors[idx].start.syncCursor(this->buffer);
}

bool Editor::up() {
	for (Cursor& curs : this->cursors) {
		if (curs.isSelection()) {
			curs.collapseToSelectionStart(this->buffer);
		}
		else {
			curs.end.up(buffer);
			curs.start.up(buffer);
		}
	}

	this->collapseOverlappingCursosr();

	return true; // TODO
}

bool Editor::down(size_t idx) {
	if (this->cursors[idx].isSelection()) {
		this->cursors[idx].collapseToSelectionEnd(this->buffer);

	}
	else {
		this->cursors[idx].end.down(buffer);
		this->cursors[idx].start.down(buffer);
	}
	return true;
}
bool Editor::down() {
	for (size_t i = 0;i < this->cursors.size();i++) {
		this->down(i);
	}

	this->collapseOverlappingCursosr();

	return true; // TODO
}

bool Editor::left(size_t idx) {
	if (this->cursors[idx].isSelection()) {
		this->cursors[idx].collapseToSelectionStart(this->buffer);
	}
	else {
		this->cursors[idx].end.left(buffer);
		this->cursors[idx].start.left(buffer);
	}
	return true;
}
bool Editor::left() {
	for (size_t ci = 0;ci < this->cursors.size();ci++) {
		this->left(ci);
	}

	this->collapseOverlappingCursosr();

	return true; // TODO
}

bool Editor::right(size_t idx) {
	if (this->cursors[idx].isSelection()) {
		this->cursors[idx].collapseToSelectionEnd(this->buffer);
	}
	else {
		this->cursors[idx].end.right(buffer);
		this->cursors[idx].start.right(buffer);
	}
	return true;
}
bool Editor::right() {
	for (size_t ci = 0;ci < this->cursors.size();ci++) {
		this->right(ci);
	}

	this->collapseOverlappingCursosr();

	return true; // TODO
}

void Editor::leftWord() {
	for (size_t ci = 0;ci < this->cursors.size();ci++) {
		Cursor& curs = this->cursors[ci];
		if (curs.isSelection()) {
			curs.collapseToSelectionStart(this->buffer);
		}

		if (!curs.left(this->buffer)) continue;

		while (getCharType(curs.start.getPrev(this->buffer)) == CharType::Alphabet) {
			if (!this->left(ci)) break;
		}
	}
}

void Editor::rightWord() {
	for (size_t ci = 0;ci < this->cursors.size();ci++) {
		Cursor& curs = this->cursors[ci];
		if (curs.isSelection()) {
			curs.collapseToSelectionEnd(this->buffer);
		}

		if (!curs.right(this->buffer)) continue;

		while (getCharType(curs.start.getNext(this->buffer)) == CharType::Alphabet) {
			if (!this->right(ci)) break;
		}
	}
}

void Editor::selectUp() {
	for (Cursor& curs : this->cursors) {
		curs.end.up(this->buffer);
	}

	this->collapseOverlappingCursosr();
}
void Editor::selectDown() {
	for (Cursor& curs : this->cursors) {
		curs.end.down(this->buffer);
	}

	this->collapseOverlappingCursosr();
}
void Editor::selectLeft() {
	for (Cursor& curs : this->cursors) {
		curs.end.left(this->buffer);
	}

	this->collapseOverlappingCursosr();
}
void Editor::selectRight() {
	for (Cursor& curs : this->cursors) {
		curs.end.right(this->buffer);
	}

	this->collapseOverlappingCursosr();
}

std::string Editor::getSelectionString(size_t idx) const {
	const Cursor& cursor = this->cursors[idx];
	IVec2 selStart = cursor.selectionStart(this->buffer);
	IVec2 selEnd = cursor.selectionEnd(this->buffer);

	if (selStart.y == selEnd.y) {
		return this->buffer[selStart.y].substr(selStart.x, selEnd.x - selStart.x);
	}
	else {
		std::string result;
		result += this->buffer[selStart.y].substr(selStart.x);
		result += '\n';
		for (size_t i = selStart.y + 1;i < selEnd.y - 1;i++) {
			result += this->buffer[i];
			result += '\n';
		}
		result += this->buffer[selEnd.y].substr(0, selEnd.x);
		return result;
	}
}

void Editor::emptySelection(size_t idx) {
	this->startTransaction();

	IVec2 selStart = this->cursors[idx].selectionStart(this->buffer);
	IVec2 selEnd = this->cursors[idx].selectionEnd(this->buffer);

	this->cursors[idx].collapseToSelectionStart(this->buffer);

	if (selStart.y == selEnd.y) {
		this->buffer[selStart.y].erase(selStart.x, selEnd.x - selStart.x);
	}
	else {
		this->buffer[selStart.y].erase(selStart.x, this->buffer[selStart.y].size() - selStart.x);
		this->buffer[selStart.y] += this->buffer[selEnd.y].substr(selEnd.x);
		this->buffer.erase(this->buffer.begin() + selStart.y + 1, this->buffer.begin() + selEnd.y + 1);
	}

	this->endTransaction();
}

void Editor::insertLine(size_t idx, const std::string& line) {
	// TODO not sure if this should be a transaction or not
	this->buffer.insert(this->buffer.begin() + idx, line);

	for (Cursor& curs : this->cursors) {
		if (curs.start.y >= idx) {
			curs.start.y++;
		}
		if (curs.end.y >= idx) {
			curs.end.y++;
		}
	}
}

void Editor::insertBefore(const char c, size_t idx) {
	this->startTransaction();

	Cursor& cursor = this->cursors[idx];
	if (cursor.isSelection()) {
		this->emptySelection(idx);
	}
	this->syncCusrorEnd(idx);

	IVec2 gPos = this->getGhostEnd(idx);

	this->buffer[gPos.y].insert(gPos.x, 1, c);

	cursor.end.right(this->buffer);
	cursor.start.right(this->buffer);


	// Realign cursors
	for (Cursor& scurs : this->cursors) {
		if (scurs.start.y == cursor.start.y && scurs.start.x > cursor.start.x) {
			scurs.start.x++;
		}
		if (scurs.end.y == cursor.start.y && scurs.end.x > cursor.start.x) {
			scurs.end.x++;
		}
	}

	this->endTransaction();
}
void Editor::insertBefore(const char c) {
	this->startTransaction();

	// TODO handle multiple cursors in same line

	for (size_t i = 0;i < this->cursors.size();i++) {
		this->insertBefore(c, i);
	}

	this->endTransaction();
}

void Editor::insertBefore(const TextBuffer& segments, size_t idx) {
	this->startTransaction();

	Cursor& cursor = this->cursors[idx];
	// make sure its empty
	if (cursor.isSelection()) {
		this->emptySelection(idx);
	}

	// insert first line
	for (int i = 0;i < segments[0].size();i++) {
		this->insertBefore(segments[0][i], idx);
	}

	// if multiline, enter at cursor
	if (segments.size() > 1) this->enter(idx);

	// enter all full line
	for (int i = 1;i < segments.size() - 1;i++) {
		this->insertLine(cursor.end.y, segments[i]);
	}

	// insert last line
	if (segments.size() > 1) {
		for (int i = 0;i < segments[segments.size() - 1].size();i++) {
			this->insertBefore(segments[segments.size() - 1][i], cursor.end.y);
		}
	}

	this->endTransaction();
}
void Editor::insertBefore(const std::string& text) {
	TextBuffer segments = splitString(text);
	if (segments.size() == 0) return;

	this->startTransaction();


	for (size_t ci = 0;ci < this->cursors.size();ci++) {
		this->insertBefore(segments, ci);
	}

	this->endTransaction();
}

void Editor::insertAfter(const char c, size_t idx) {
	this->startTransaction();

	if (this->cursors[idx].isSelection()) {
		this->emptySelection(idx);
	}
	this->syncCusrorEnd(idx);
	IVec2 gPos = this->getGhostEnd(idx);

	this->buffer[gPos.y].insert(gPos.x, 1, c);

	// Realign cursors
	Cursor& cursor = this->cursors[idx];
	for (Cursor& scurs : this->cursors) {
		if (scurs.start.y == cursor.start.y && scurs.start.x > cursor.start.x) {
			scurs.start.x++;
		}
		if (scurs.end.y == cursor.start.y && scurs.end.x > cursor.start.x) {
			scurs.end.x++;
		}
	}
	this->endTransaction();
}
void Editor::insertAfter(const char c) {
	for (size_t ci = 0;ci < this->cursors.size();ci++) {
		this->insertAfter(c, ci);
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
	this->startTransaction();

	std::pair<char, char> cPair = SpecialKeyMapping[(ImGuiKey)ch];
	char c = shift ? cPair.second : cPair.first;
	this->insertBefore(c);

	if (ClosablesChars.find(c) == ClosablesChars.end()) {
		this->endTransaction();
		return;
	}

	this->insertAfter(ClosablesChars[c]);

	this->endTransaction();
}

// TODO remove closing brackets/quotes/braces... if empty
bool Editor::backspace(size_t idx) {
	Cursor& cursor = this->cursors[idx];
	if (cursor.isSelection()) {
		this->emptySelection(idx);

		return true;
	}

	IVec2 gPos = this->getGhostEnd(idx);
	this->syncCusrorEnd(idx);

	if (gPos.x > 0) {
		this->startTransaction();

		cursor.end.x--;
		cursor.start.x--;
		this->buffer[gPos.y].erase(gPos.x - 1, 1);

		// Realign cursors
		for (Cursor& scurs : this->cursors) {
			if (scurs.start.y == cursor.start.y && scurs.start.x > cursor.start.x) {
				scurs.start.x--;
			}
			if (scurs.end.y == cursor.start.y && scurs.end.x > cursor.start.x) {
				scurs.end.x--;
			}
		}

		this->endTransaction();
		return true;
	}
	else if (gPos.y > 0) {
		this->startTransaction();

		size_t prevLineLen = this->buffer[cursor.end.y - 1].size();
		// Realign cursors in this line
		for (Cursor& scurs : this->cursors) {
			if (scurs.start.y == cursor.start.y) {
				scurs.start.x += prevLineLen;
			}
			if (scurs.end.y == cursor.start.y) {
				scurs.end.x += prevLineLen;
			}
		}

		// append to previous line, and remove it
		this->buffer[cursor.end.y - 1] += this->buffer[cursor.end.y];
		this->buffer.erase(this->buffer.begin() + cursor.end.y);

		for (Cursor& scurs : this->cursors) {
			if (scurs.start.y >= cursor.start.y) {
				scurs.start.y--;
			}
			if (scurs.end.y >= cursor.start.y) {
				scurs.end.y--;
			}
		}

		this->endTransaction();
		return true;
	}
	return false;
}
bool Editor::backspace() {
	this->startTransaction();

	for (size_t i = 0;i < this->cursors.size();i++) {
		this->backspace(i);
	}

	this->endTransaction();

	return true;
}

bool Editor::backspaceWord() {
	this->startTransaction();

	for (size_t i = 0;i < this->cursors.size();i++) {
		Cursor& cursor = this->cursors[i];
		if (cursor.isSelection()) {
			this->emptySelection(i);
			continue;
		}


		if (!this->backspace(i)) continue;

		while (getCharType(cursor.start.getPrev(this->buffer)) == CharType::Alphabet) {
			if (!this->backspace(i)) break;
		}
	}

	this->endTransaction();
	return true;
}

bool Editor::del(size_t idx) {
	Cursor& cursor = this->cursors[idx];

	if (cursor.isSelection()) {
		this->emptySelection(idx);
		return true;
	}
	IVec2 gPos = this->getGhostEnd(idx);
	this->syncCusrorEnd(idx);

	if (gPos.x < this->buffer[gPos.y].size()) {
		this->startTransaction();

		this->buffer[gPos.y].erase(gPos.x, 1);

		this->endTransaction();
		return true;
	}
	else if (gPos.y < this->buffer.size() - 1) {
		this->startTransaction();

		this->buffer[cursor.end.y] += this->buffer[cursor.end.y + 1];
		this->buffer.erase(this->buffer.begin() + cursor.end.y + 1);

		this->endTransaction();
		return true;
	}
	return false;
}
bool Editor::del() {
	this->startTransaction();

	for (size_t i = 0;i < this->cursors.size();i++) {
		this->del(i);
	}

	this->endTransaction();

	return true;
}

bool Editor::delWord() {
	this->startTransaction();

	for (size_t i = 0;i < this->cursors.size();i++) {
		if (this->cursors[i].isSelection()) {
			this->emptySelection(i);
			continue;
		}

		if (!this->del()) continue;

		while (getCharType(this->cursors[i].start.getNext(this->buffer)) == CharType::Alphabet) {
			if (!this->del()) break;
		}
	}

	this->endTransaction();
	return true;
}

void Editor::enter(size_t idx) {
	this->startTransaction();

	Cursor& cursor = this->cursors[idx];
	if (cursor.isSelection()) {
		this->emptySelection(idx);
	}
	IVec2 gPos = this->getGhostEnd(idx);

	std::string before = this->buffer[gPos.y].substr(0, gPos.x);
	std::string after = this->buffer[gPos.y].substr(gPos.x);

	this->buffer[gPos.y] = before;
	this->buffer.insert(this->buffer.begin() + gPos.y + 1, after);

	// realign cursors after that line
	for (Cursor& scurs : this->cursors) {
		if (scurs.start.y > gPos.y) {
			scurs.start.y++;
		}
		if (scurs.end.y > gPos.y) {
			scurs.end.y++;
		}
	}

	// realign cursors on same line
	for (Cursor& scurs : this->cursors) {
		if (scurs.start.y == gPos.y && scurs.start.x >= gPos.x) {
			scurs.start.x -= gPos.x;
			scurs.start.y++;
		}
		if (scurs.end.y == gPos.y && scurs.end.x >= gPos.x) {
			scurs.end.x -= gPos.x;
			scurs.end.y++;
		}
	}

	this->endTransaction();
}
void Editor::enter() {
	this->startTransaction();

	for (size_t i = 0;i < this->cursors.size();i++) {
		this->enter(i);
	}

	this->endTransaction();
}

void Editor::enterAndIndent(size_t idx) {
	this->startTransaction();

	Cursor& cursor = this->cursors[idx];
	if (cursor.isSelection()) {
		this->emptySelection(idx);
	}
	IVec2 gPos = this->getGhostEnd(idx);

	int indentSize = this->getIndentOf(gPos.y);
	bool shouldAddIndent = this->shouldAddIndent(gPos.y, gPos.x);
	bool shouldDropIntoNewLine = this->shouldDropIntoNewLine(gPos.y, gPos.x);

	this->enter(idx);
	repeat(indentSize) this->insertBefore(' ', idx);

	if (shouldDropIntoNewLine) {
		repeat(2) this->insertBefore(' ', idx);
		this->enter(idx);

		repeat(indentSize) this->insertBefore(' ', idx);

		cursor.start.y--;
		cursor.end.y--;
		cursor.start.x = indentSize + 2;
		cursor.end.x = indentSize + 2;
	}

	this->endTransaction();
}
void Editor::enterAndIndent() {
	this->startTransaction();

	for (size_t i = 0;i < this->cursors.size();i++) {
		this->enterAndIndent(i);
	}

	this->endTransaction();
}

bool Editor::home() {
	TODO();
	/*if (this->cursor.end.x == 0) {
		return false;
	}
	this->cursor.end.x = 0;
	this->cursor.start.x = 0;*/
	return true;
}

bool Editor::end(size_t idx) {
	Cursor& cursor = this->cursors[idx];
	if (cursor.end.x == this->buffer[cursor.end.y].size()) {
		return false;
	}
	cursor.end.x = this->buffer[cursor.end.y].size();
	cursor.start.x = this->buffer[cursor.end.y].size();
	return true;
}
bool Editor::end() {
	for (size_t i = 0; i < this->cursors.size(); i++) {
		this->end(i);
	}

	return true;
}

int Editor::getIndentOf(size_t lineNo) {
	int indentSize = 0;
	while (indentSize < this->buffer[lineNo].size() && this->buffer[lineNo][indentSize] == ' ') indentSize++;
	return indentSize;
}

static std::unordered_set<char> indentOpeners = { '(','[','{' };
static std::unordered_set<char> indentClosers = { ')',']','}' };

// TODO this might differ from exact behaviour of vscode, but is good enough for now
bool Editor::shouldAddIndent(size_t lineNo, size_t curPosX) {
	for (int i = (int)(curPosX - 1); i >= 0; i--) {
		if (indentOpeners.find(this->buffer[lineNo][i]) != indentOpeners.end()) {
			return true;
		}
		else if (indentClosers.find(this->buffer[lineNo][i]) != indentClosers.end()) {
			return false;
		}
	}
	return false;
}

bool Editor::shouldDropIntoNewLine(size_t lineNo, size_t curPosX) {
	return curPosX < this->buffer[lineNo].size() && indentClosers.find(this->buffer[lineNo][curPosX]) != indentClosers.end();
}

void Editor::loadGrammar(Grammar grammar) {
	this->grammar = grammar;
}

void Editor::debug() {
	std::cout << "--------------------------\n";
	for (const std::string& l : buffer) std::cout << l.size() << " : " << l << "\n";
	std::cout << "--------------------------\n";
}
