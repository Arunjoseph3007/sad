#pragma once
#include <string>
#include <regex>
#include <vector>

/*
* We support 2 types of rules, TokenBased and RegionBased
* 
* TokenBased: use match regex to match entire token like keywords, operator, puctuations, etc
* RegionBased: use startMatch to mark start of a region and searches for endMatch to define end
*/
class GrammarRules {
public:
	std::string name;
	std::regex match;

	bool isRegionBased = {};
	std::regex startMatch = {};
	std::regex endMatch = {};
};


struct GrammarMatch {
public:
	size_t start, end;
	std::string matchedClass;
};

std::ostream& operator<<(std::ostream& os, const GrammarMatch& obj);

struct Grammar {
public:
	std::vector<GrammarRules> patterns;

	std::vector<GrammarMatch>	parseString(const std::string& input) const;
};

Grammar simpleJsGrammar();