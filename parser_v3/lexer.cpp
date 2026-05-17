#include "State.hpp"

#include <cctype>
#include <stdexcept>
#include <string>

std::vector<std::string> lexer(std::stringstream& input)
{
	std::string source = input.str();
	std::vector<std::string> out;

	for (size_t i = 0; i < source.size();)
	{
		unsigned char ch = static_cast<unsigned char>(source[i]);
		if (std::isspace(ch))
		{
			++i;
			continue;
		}
		if (std::isdigit(ch))
		{
			std::string num;
			while (i < source.size() && std::isdigit(static_cast<unsigned char>(source[i])))
				num += source[i++];
			out.push_back(num);
			continue;
		}
		if (std::isalpha(ch) || source[i] == '_')
		{
			std::string word;
			while (i < source.size())
			{
				unsigned char c = static_cast<unsigned char>(source[i]);
				if (!std::isalnum(c) && source[i] != '_')
					break;
				word += source[i++];
			}
			out.push_back(word);
			continue;
		}
		// two-character operators — must check before single-char
		if (i + 1 < source.size())
		{
			char c0 = source[i], c1 = source[i + 1];
			if ((c0 == '=' && c1 == '=') || (c0 == '!' && c1 == '=') ||
			    (c0 == '>' && c1 == '=') || (c0 == '<' && c1 == '=') ||
			    (c0 == '&' && c1 == '&') || (c0 == '|' && c1 == '|'))
			{
				out.push_back(std::string{c0, c1});
				i += 2;
				continue;
			}
		}
		// single-character operators and punctuation
		static const std::string singles = "+-*/%(){};<>=!,";
		if (singles.find(source[i]) != std::string::npos)
		{
			out.push_back(std::string(1, source[i]));
			++i;
			continue;
		}
		throw std::runtime_error(std::string("Unexpected character: ") + source[i]);
	}

	return out;
}
