#pragma once
#include <string>
#include <deque>
#include "Cursor.h"
#include "Edit.h"

typedef std::deque<Edit> History;

const int MAX_UNDO_HISTORY_SIZE = 100;

class Editor {
public:
	Cursor cursor;
	TextBuffer buffer;

	Editor();
	void debug();

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
private:
	int transactionRefCount = 0;
	History undoHistory, redoHistory;
	TextBuffer oldBuffer;
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
	IVec2 getCursorEnd();
	IVec2 getGhostEnd();
	IVec2 getCursorStart();
	IVec2 getGhostStart();
	void syncCusrorEnd();
	void syncCusrorStart();

	bool up();
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
	bool end();

	std::string getSelectionString() const;
	void emptySelection();

	/*
		Editing system
	*/
public:
	void insertBefore(const char c);
	void insertBefore(const std::string& text);
	void insertAfter(const char c);
	void insertAfter(const std::string& text);
	void charInsertBefore(int ch, bool shift);

	bool backspace();
	bool eDelete();

	void enter();
};
