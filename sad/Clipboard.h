#pragma once
#include <string>
#include <optional>
#include "Common.h"

class ClipboardManageState {
public:
	TextBuffer multiCursorText;

	ClipboardManageState() :multiCursorText() {}
	ClipboardManageState(const TextBuffer& mct) :multiCursorText(mct) {}
};

class ClipboardManager {
private:
	//TODO: we can probably replace this with just the hash of content
	std::string lastCopied;
	ClipboardManageState state;

public:
	ClipboardManager() : lastCopied(""), state(ClipboardManageState()) {}

	void onCopy(const std::string& text, ClipboardManageState state);
	std::optional<ClipboardManageState> onPaste(const std::string& text);
};