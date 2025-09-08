#include "Grammar.h"
#include "Timer.h"
#include <string>
#include <iostream>

Grammar simpleJsGrammar() {
	Grammar js = {};

	js.patterns.emplace_back("comment", std::regex(R"(\/\/.*)"));
	js.patterns.emplace_back("comment", std::regex(R"(\/\*)"), std::regex(R"(\*\/)"));

	js.patterns.emplace_back("control", std::regex(R"(\b(break|case|catch|continue|default|do|else|finally|for|if|return|switch|throw|try|while|with)\b)"));
	js.patterns.emplace_back("declaration", std::regex(R"(\b(var|let|const|function|class)\b)"));
	js.patterns.emplace_back("context", std::regex(R"(\b(this|super|new|delete|typeof|void|yield|await|import|export)\b)"));
	js.patterns.emplace_back("literal", std::regex(R"(\b(true|false|null)\b)"));

	js.patterns.emplace_back("string", std::regex(R"(\")"), std::regex(R"(\")"));
	js.patterns.emplace_back("string", std::regex(R"(')"), std::regex(R"(')"));
	js.patterns.emplace_back("string", std::regex(R"(`)"), std::regex(R"(`)"));

	js.patterns.emplace_back("numeric", std::regex(R"(\b(?:0[bB][01]+|0[oO][0-7]+|0[xX][\dA-Fa-f]+|\d+(\.\d+)?([eE][+-]?\d+)?|\.\d+([eE][+-]?\d+)?)\b)"));
	js.patterns.emplace_back("operator", std::regex(R"((\+\+|--|===|==|!==|!=|<=|>=|<|>|\+=|-=|\*=|\/=|%=|\*\*|&&|\|\||!|=|\+|-|\*|\/|%|\*\*=|&=|\|=|\^=|<<=|>>=|>>>=|&|\||\^|~|<<|>>|>>>|\?|:|=>))"));
	js.patterns.emplace_back("variable", std::regex(R"(\b[a-zA-Z_$][a-zA-Z0-9_$]*\b)"));
	js.patterns.emplace_back("punctuation", std::regex(R"([.,;()[\]{}])"));

	return js;
}

std::vector<GrammarMatch> Grammar::parseString(const std::string& input) const {
	TIMEIT();

	std::vector<GrammarMatch> result;

	for (size_t i = 0;i < input.size();) {
		const char c = input[i];

		// ignore white space
		if (c == ' ' || c == '\r' || c == '\t' || c == '\n') {
			GrammarMatch m = { .start = i, .end = i + 1,.matchedClass = "whitespace" };
			result.push_back(m);
			i++;
			continue;
		}

		bool found = false;
		for (const GrammarRule& rule : this->patterns) {
			if (rule.isRegionBased) {
				std::cmatch startM, endM;
				std::regex_search(input.data() + i, startM, rule.match, std::regex_constants::match_continuous);

				if (startM.size() > 0) {
					found = true;
					size_t start = i, end = input.size() - 1;
					i += startM[0].length();

					std::regex_search(input.data() + i, endM, rule.endMatch);

					if (endM.size() > 0) end = endM[0].second - input.data();

					GrammarMatch m = { .start = start, .end = end,.matchedClass = rule.name };
					result.push_back(m);

					i = end;
					break;
				}
			}
			else {
				std::cmatch match;
				std::regex_search(input.data() + i, match, rule.match, std::regex_constants::match_continuous);

				// match found
				if (match.size() > 0) {
					found = true;
					GrammarMatch m = { .start = i, .end = i + match[0].length(),.matchedClass = rule.name };
					result.push_back(m);
					i += match[0].length();
					break;
				}
			}
		}
		if (!found) {
			i++;
		}
	}

	return result;
}

// TODO this function is kind off cluttered try and sort it out
std::vector<GrammarMatch> Grammar::parseTextBuffer(const TextBuffer& tb) const {
	TIMEIT();
	std::vector<GrammarMatch> result;

	for (size_t lineNo = 0; lineNo < tb.size(); lineNo++) {
		for (size_t i = 0; i < tb[lineNo].size();) {
			const char c = tb[lineNo][i];

			// ignore white space
			if (c == ' ' || c == '\r' || c == '\t') {
				GrammarMatch m = { .start = i, .end = i + 1,.matchedClass = "whitespace", .line = lineNo };
				result.push_back(m);
				i++;
				continue;
			}

			bool found = false;
			for (const GrammarRule& rule : this->patterns) {
				// region based matching
				if (rule.isRegionBased) {
					std::cmatch startM, endM;
					std::regex_search(tb[lineNo].data() + i, startM, rule.match, std::regex_constants::match_continuous);

					if (startM.size() > 0) {
						found = true;
						size_t start = i, end = tb[lineNo].size() - 1;
						i += startM[0].length();

						std::regex_search(tb[lineNo].data() + i, endM, rule.endMatch);
						if (endM.size() > 0) {
							end = endM[0].second - tb[lineNo].data();
							GrammarMatch m = { .start = start, .end = end,.matchedClass = rule.name, .line = lineNo };
							result.push_back(m);

							i = end;
						}
						else {
							GrammarMatch firstl = { .start = start, .end = end,.matchedClass = rule.name, .line = lineNo };
							result.push_back(firstl);
							lineNo++;
							while (!std::regex_search(tb[lineNo].data(), endM, rule.endMatch)) {
								GrammarMatch midl = { .start = 0,.end = tb[lineNo].size() - 1,.matchedClass = rule.name, .line = lineNo };
								result.push_back(midl);
								lineNo++;
							}
							GrammarMatch lastl = { .start = 0,.end = (size_t)(endM[0].second - tb[lineNo].data()), .matchedClass = rule.name, .line = lineNo };
							result.push_back(lastl);
							i = lastl.end;
						}
						break;
					}
				}
				// Token based matching
				else {
					std::cmatch match;
					std::regex_search(tb[lineNo].data() + i, match, rule.match, std::regex_constants::match_continuous);

					// match found
					if (match.size() > 0) {
						found = true;
						GrammarMatch m = { .start = i, .end = i + match[0].length(),.matchedClass = rule.name, .line = lineNo };
						result.push_back(m);
						i += match[0].length();
						break;
					}
				}
			}

			if (!found) {
				i++;
			}
		}
	}

	return result;
}

std::ostream& operator<<(std::ostream& os, const GrammarMatch& obj) {
	os << "GrammarMatch(line: " << obj.line << ", start: " << obj.start << ", end: " << obj.end << ", class: " << obj.matchedClass << ")";
	return os;
}