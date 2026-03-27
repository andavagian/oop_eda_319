#include "State.hpp"

#include <cctype>
#include <stdexcept>

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
			std::string var;
			while (i < source.size())
			{
				unsigned char c = static_cast<unsigned char>(source[i]);
				if (!std::isalnum(c) && source[i] != '_')
					break;
				var += source[i++];
			}
			out.push_back(var);
			continue;
		}
		if (source[i] == '+' || source[i] == '-' || source[i] == '*' || source[i] == '/' || source[i] == '(' || source[i] == ')')
		{
			out.push_back(std::string(1, source[i]));
			++i;
			continue;
		}
		throw std::runtime_error(std::string("Unexpected token: ") + source[i]);
	}

	return out;
}
