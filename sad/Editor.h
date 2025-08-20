#pragma once
#include <string>
#include <vector>
#include "Cursor.h"

#define EVENT_LOGGING
#ifdef EVENT_LOGGING
#define ELOG() printf(__VA__ARGS__)
#else
#define ELOG() ()
#endif

class Editor {
public:
    Cursor cursor;
    TextBuffer buffer;

    Editor();

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

    std::string getSelectionString() const;
    void emptySelection();

    void insertBefore(const char c);
    void insertBefore(const std::string& text);
    void insertAfter(const char c);
    void insertAfter(const std::string& text);
    void charInsertBefore(int ch, bool shift);

    bool backspace();
    bool eDelete();

    void enter();

    bool home();
    bool end();

    void debug();
};
