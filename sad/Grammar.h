#pragma once
#include <string>
#include <regex>
#include <vector>
#include "Common.h"

/*
* We support 2 types of rules, TokenBased and RegionBased
*
* TokenBased: use match regex to match entire token like keywords, operator, puctuations, etc
* RegionBased: use match to mark start of a region and searches for endMatch to define end
*/
class GrammarRule {
public:
	std::string name;
	std::regex match;

	bool isRegionBased = {};
	std::regex endMatch = {};

	// used to creatin TokenBased rule
	GrammarRule(std::string name, std::regex match) :
		name(name),
		isRegionBased(false),
		match(match) {
	};

	// used to creatin RegionBased rule
	GrammarRule(std::string name, std::regex startMatch, std::regex endMatch) :
		name(name),
		isRegionBased(true),
		match(startMatch),
		endMatch(endMatch) {
	};
};

struct GrammarMatch {
public:
	size_t start, end;
	std::string matchedClass;
	size_t line = 0;
};

std::ostream& operator<<(std::ostream& os, const GrammarMatch& obj);

struct Grammar {
public:
	std::vector<GrammarRule> patterns;

	std::vector<GrammarMatch> parseString(const std::string& input) const;
	std::vector<GrammarMatch> parseTextBuffer(const TextBuffer& tb) const;
};

Grammar simpleJsGrammar();