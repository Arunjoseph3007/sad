#include "Grammar.h"
#include "Timer.h"
#include <string>
#include <iostream>

Grammar simpleJsGrammar() {
	Grammar js = {};
	js.patterns.push_back({ "commnets", std::regex(R"(\/\/.*|\/\*[\s\S]*?\*\/)") });
	js.patterns.push_back({ "control", std::regex(R"(\b(break|case|catch|continue|default|do|else|finally|for|if|return|switch|throw|try|while|with)\b)") });
	js.patterns.push_back({ "declaration", std::regex(R"(\b(var|let|const|function|class)\b)") });
	js.patterns.push_back({ "context", std::regex(R"(\b(this|super|new|delete|typeof|void|yield|await|import|export)\b)") });
	js.patterns.push_back({ "literal", std::regex(R"(\b(true|false|null)\b)") });
	js.patterns.push_back({ "string", std::regex(R"(([\"'`])((?:(?!\1)(?:\\.|[^\\]))*)(\1))") });
	js.patterns.push_back({ "numeric", std::regex(R"(\b(?:0[bB][01]+|0[oO][0-7]+|0[xX][\dA-Fa-f]+|\d+(\.\d+)?([eE][+-]?\d+)?|\.\d+([eE][+-]?\d+)?)\b)") });
	js.patterns.push_back({ "operator", std::regex(R"((\+\+|--|===|==|!==|!=|<=|>=|<|>|\+=|-=|\*=|\/=|%=|\*\*|&&|\|\||!|=|\+|-|\*|\/|%|\*\*=|&=|\|=|\^=|<<=|>>=|>>>=|&|\||\^|~|<<|>>|>>>|\?|:|=>))") });
	js.patterns.push_back({ "variable", std::regex(R"(\b[a-zA-Z_$][a-zA-Z0-9_$]*\b)") });
	js.patterns.push_back({ "punctuation", std::regex(R"([.,;()[\]{}])") });

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
		for (const GrammarRules& rule : this->patterns) {
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
		if (!found) {
			i++;
		}
	}

	return result;
}

std::ostream& operator<<(std::ostream& os, const GrammarMatch& obj) {
	os << "GrammarMatch( start: " << obj.start << ", end: " << obj.end << ", class: " << obj.matchedClass << ")";
	return os;
}