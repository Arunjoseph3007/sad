#include "CommandCenter.h"
#include "Common.h"
#include <algorithm>
#include <iostream>

bool Command::dispatch(Editor& editor, const CommandArgs& args) const {
	return this->commandAction(editor, args);
}

void CommandCenter::addCommand(const std::string& commandName, CommandFunc func, size_t argCount) {
	Command c(func, argCount);
	this->commands.insert({ commandName, c });
}

static int toInt(const std::string& str) {
	int result = 0;

	for (char ch : str) {
		if (!isdigit(ch)) return -1;

		int no = ch - '0';
		result = result * 10 + no;
	}

	return result;
}

bool CommandCenter::dispatch(Editor& editor, const char* commandText) const {
	CommandArgs args = splitString(commandText, ' ');

	if (args.size() == 0) {
		std::cout << "Invalid command\n";
		return false;
	}

	int repeat = toInt(args[0]);
	if (repeat > 0) args.erase(args.begin());
	else repeat = 1;

	if (args.size() == 0) {
		std::cout << "No command found after repeat predicate\n";
		return false;
	}

	if (this->commands.find(args[0]) == this->commands.end()) {
		std::cout << "No command named: " << args[0] << " registered\n";
		return false;
	}

	std::string command = args[0];
	args.erase(args.begin());

	Command c = this->commands.at(command);

	if (args.size() != c.argsCount) {
		printf("Expected %zu arguments, got %zu instead\n", c.argsCount, args.size());
		return false;
	}

	bool resp = true;
	// dispatch repeatedly
	for (int i = 0;i < repeat;i++) if (!c.dispatch(editor, args)) resp = false;

	return resp;
}
