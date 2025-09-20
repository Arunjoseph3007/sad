#pragma once
#include <string>
#include "Cursor.h"
#include "Edit.h"
#include "Grammar.h"

const int MAX_UNDO_HISTORY_SIZE = 100;

class Editor {
public:
	std::vector<Cursor> cursors;
	TextBuffer buffer;

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
	IVec2 getCursorEnd(int idx);
	IVec2 getGhostEnd(int idx);
	IVec2 getCursorStart(int idx);
	IVec2 getGhostStart(int idx);
	void syncCusrorEnd(int idx);
	void syncCusrorStart(int idx);

	void collapseOverlappingCursosr() {/*TODO*/ }

	bool up();
	bool down(size_t idx);
	bool down();
	bool left();
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

	int getIndentOf(int lineNo);
	bool shouldAddIndent(int lineNo, int curPosX);
	bool shouldDropIntoNewLine(int lineNo, int curPosX);

	std::string getSelectionString(int idx) const;
	void emptySelection(int idx);

	/*
		Editing system
	*/
public:
	void insertBefore(const char c, size_t idx);
	void insertBefore(const char c);
	void insertBefore(const std::string& text);
	void insertAfter(const char c, size_t idx);
	void insertAfter(const char c);
	void insertAfter(const std::string& text);
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

	/*
		Syntax highlighting stuff
		this really should be here but we are using it just to move fast
	*/
private:
	Grammar grammar;

public:
	std::vector<GrammarMatch> tokens;

	void loadGrammar(Grammar grammar);
};
