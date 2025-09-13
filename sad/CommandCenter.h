#pragma once
#include <vector>
#include <unordered_map>
#include <functional>
#include <string>
#include "Editor.h"

typedef std::vector<std::string> CommandArgs;
typedef std::function<bool(Editor&, const CommandArgs&)> CommandFunc;

// this macro can be used to simplify command decalration using lambda syntax
// CMD_DECL{ return e.up(); }
// instead of
// [](Editor& e, const CommandArgs& args)
#define CMD_DECL [](Editor& e, const CommandArgs& args)

class Command {
public:
	Command(CommandFunc func, size_t t) : commandAction(func), argsCount(t) {};

	CommandFunc commandAction;
	size_t argsCount;

	bool dispatch(Editor& editor, const CommandArgs& args) const;
};

class CommandCenter {
private:
	std::unordered_map<std::string, Command> commands;

public:
	void addCommand(const std::string& commandName, CommandFunc func, size_t argCount = 0);

	bool dispatch(Editor& editor, const char* commandText) const;
};