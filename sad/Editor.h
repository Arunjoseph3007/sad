#pragma once
#include <string>
#include "Cursor.h"
#include "Edit.h"
#include "Grammar.h"

const int MAX_UNDO_HISTORY_SIZE = 100;

class Editor {
public:
	Editor();

	void debug();
	std::string getText() const;

	/*
		Undo system
		LORD FORGIVE ME FOR I HAVE SINNED, THIS IS VERY INNEFFICEINT

		This uses basic vector diffing to manage edits.
		This is very inefficeint because we might be storing entire string
		even if the diff is only a character.

		We copy and store buffer into oldBuffer on startTransaction
		and diff and push into history in endTransaction
		We are using reference counting to detect nested transactions

		If user is implementing a trasaction make sure that,
		all the edits must be between startTransaction() and endTransaction()
	*/
	// TODO make private
public:
	int transactionRefCount = 0;
	History undoHistory, redoHistory;
	TextBuffer oldBuffer;
	std::vector<Cursor> oldCursors;
public:
	bool startTransaction();
	bool endTransaction();
	bool undo();
	bool redo();

	/*
		Cursor system
		I PERSONALY BELIEVE WE NAILED THIS, FAMOUS LAST WORDS

		We have 2 class IVec2(Anchor) and Cursor, but not Selection
		Anchor: know how to navigate given a buffer, methods like up, down, left & right
		Cursor: is a combination of 2 Anchors (start & end)

		When they are the same they behave as cursor
		When different they behave as Selections
	*/
public:
	IVec2 getCursorEnd(size_t idx);
	IVec2 getGhostEnd(size_t idx);
	IVec2 getCursorStart(size_t idx);
	IVec2 getGhostStart(size_t idx);
	void syncCusrorEnd(size_t idx);
	void syncCusrorStart(size_t idx);

	void collapseOverlappingCursosr() { hack("Empty implmentation for collapseOverlappingCursosr"); }

	bool up();
	bool down(size_t idx);
	bool down();
	bool left(size_t idx);
	bool left();
	bool right(size_t idx);
	bool right();

	void leftWord();
	void rightWord();

	void selectUp();
	void selectDown();
	void selectLeft();
	void selectRight();

	bool home();
	bool end(size_t idx);
	bool end();

	std::string getSelectionString(size_t idx) const;
	void emptySelection(size_t idx);

	/*
		Editing system

		How to work with multiple cursors?
		Each function has to realign and manipulate the cursors such that it makes sense before returning.
		This means that as long as you are using functions you wont have to worry about managing cursors.

		It is only when you directly manipulate buffer or cursors that you are responsible for
		realigning not only the current cursor but also other cursors so that it is all good.
		When introducing keep in mind that it should manage its own cursors.
	*/
public:
	std::vector<Cursor> cursors;
	TextBuffer buffer;
private:
	void insertLine(size_t idx, const std::string& line);
public:
	void insertBefore(const char c, size_t idx);
	void insertBefore(const char c);
	void insertBefore(const TextBuffer& segments, size_t idx);
	void insertBefore(const std::string& text);

	void insertAfter(const char c, size_t idx);
	void insertAfter(const char c);
	void charInsertBefore(int ch, bool shift);

	bool backspace(size_t idx);
	bool backspace();
	bool backspaceWord();
	bool del(size_t idx);
	bool del();
	bool delWord();

	void enter(size_t idx);
	void enter();
	void enterAndIndent(size_t idx);
	void enterAndIndent();

	int getIndentOf(size_t lineNo);
	bool shouldAddIndent(size_t lineNo, size_t curPosX);
	bool shouldDropIntoNewLine(size_t lineNo, size_t curPosX);

	/*
		Syntax highlighting stuff
		this really should be here but we are using it just to move fast
	*/
private:
	Grammar grammar;

public:
	std::vector<GrammarMatch> tokens;

	void loadGrammar(const Grammar& grammar);
	void tokenize();
};
