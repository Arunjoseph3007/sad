#pragma once
#include <string>
#include <regex>
#include <vector>

class GrammarRules {
public:
	std::string name;
	std::regex match;
};


struct GrammarMatch {
public:
	size_t start, end;
	std::string matchedClass;
};

std::ostream& operator<<(std::ostream& os, const GrammarMatch& obj);

struct Grammar {
public:
	//std::string scopeName;
	//std::vector<std::regex> fileTypes;
	//std::regex foldingStartMarker, foldingStopMarker;
	std::vector<GrammarRules> patterns;

	std::vector<GrammarMatch>	parseString(const std::string& input) const;
};

Grammar simpleJsGrammar();