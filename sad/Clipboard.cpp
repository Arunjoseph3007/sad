#include "Clipboard.h"
#include <iostream>

void ClipboardManager::onCopy(const std::string& text, ClipboardManageState state) {
	this->lastCopied = text;
	this->state = state;
}

std::optional<ClipboardManageState> ClipboardManager::onPaste(const std::string& text) {
	if (text == this->lastCopied) {
		std::cout << "[CLIPBOARD MGR]: initiating smart paste\n";
		return this->state;
	}

	this->lastCopied = "";
	return std::nullopt;
}