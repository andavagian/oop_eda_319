#include <cctype>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

struct Parser
{
	const std::string& expr;
	size_t pos;
	const std::map<std::string, int>& vars;

	Parser(const std::string& s, const std::map<std::string, int>& v)
		: expr(s), pos(0), vars(v) {}

	int parse()
	{
		int result = parse_expr();
		if (pos != expr.size())
			throw std::runtime_error(std::string("Unexpected token: ") + expr[pos]);
		return result;
	}

	int parse_expr()
	{
		int lhs = parse_term();
		while (pos < expr.size() && (expr[pos] == '+' || expr[pos] == '-'))
		{
			char op = expr[pos++];
			int rhs = parse_term();
			lhs = (op == '+') ? lhs + rhs : lhs - rhs;
		}
		return lhs;
	}

	int parse_term()
	{
		int lhs = parse_factor();
		while (pos < expr.size() && (expr[pos] == '*' || expr[pos] == '/'))
		{
			char op = expr[pos++];
			int rhs = parse_factor();
			if (op == '*')
				lhs *= rhs;
			else
			{
				if (rhs == 0)
					throw std::runtime_error("Division by zero");
				lhs /= rhs;
			}
		}
		return lhs;
	}

	int parse_factor()
	{
		if (pos >= expr.size())
			throw std::runtime_error("Unexpected end of input");

		if (expr[pos] == '(')
		{
			++pos;
			int val = parse_expr();
			if (pos >= expr.size() || expr[pos] != ')')
				throw std::runtime_error("Missing closing parenthesis");
			++pos;
			return val;
		}

		if (std::isdigit(static_cast<unsigned char>(expr[pos])))
		{
			int value = 0;
			while (pos < expr.size() && std::isdigit(static_cast<unsigned char>(expr[pos])))
				value = value * 10 + (expr[pos++] - '0');
			return value;
		}

		if (std::isalpha(static_cast<unsigned char>(expr[pos])) || expr[pos] == '_')
		{
			std::string name;
			while (pos < expr.size())
			{
				unsigned char c = static_cast<unsigned char>(expr[pos]);
				if (!std::isalnum(c) && expr[pos] != '_')
					break;
				name += expr[pos++];
			}
			std::map<std::string, int>::const_iterator it = vars.find(name);
			if (it == vars.end())
				throw std::runtime_error("Undefined variable: " + name);
			return it->second;
		}

		throw std::runtime_error(std::string("Unexpected token: ") + expr[pos]);
	}
};

static void parse_var(const char* arg, std::map<std::string, int>& vars)
{
	std::string item(arg);
	size_t eq = item.find('=');
	if (eq == std::string::npos || eq == 0 || eq + 1 >= item.size())
		throw std::runtime_error("Invalid variable assignment: " + item);
	std::string name = item.substr(0, eq);
	for (size_t i = 0; i < name.size(); ++i)
	{
		unsigned char c = static_cast<unsigned char>(name[i]);
		if (i == 0)
		{
			if (!std::isalpha(c) && name[i] != '_')
				throw std::runtime_error("Invalid variable name: " + name);
		}
		else if (!std::isalnum(c) && name[i] != '_')
			throw std::runtime_error("Invalid variable name: " + name);
	}
	vars[name] = std::stoi(item.substr(eq + 1));
}

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		std::cout << "Usage: ./expr \"expression\" [var=value ...]\n";
		return 1;
	}

	try
	{
		std::map<std::string, int> vars;
		for (int i = 2; i < argc; ++i)
			parse_var(argv[i], vars);

		Parser p(argv[1], vars);
		std::cout << p.parse() << std::endl;
		return 0;
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
}