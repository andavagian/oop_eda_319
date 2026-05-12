#include "State.hpp"

#include <cctype>
#include <stdexcept>

static Operator to_op(char c)
{
	if (c == '+')
		return Operator::Add;
	if (c == '-')
		return Operator::Sub;
	if (c == '*')
		return Operator::Mult;
	if (c == '/')
		return Operator::Div;
	throw std::runtime_error("Unknown operator");
}

std::vector<Token> tokenizer(const std::vector<std::string>& pieces)
{
	std::vector<Token> tokens;
	for (const std::string& piece : pieces)
	{
		if (piece == "(")
		{
			tokens.push_back({TokenType::LParen, 0, Operator::Add, piece});
			continue;
		}
		if (piece == ")")
		{
			tokens.push_back({TokenType::RParen, 0, Operator::Add, piece});
			continue;
		}
		if (piece.size() == 1 && (piece[0] == '+' || piece[0] == '-' || piece[0] == '*' || piece[0] == '/'))
		{
			tokens.push_back({TokenType::Operator, 0, to_op(piece[0]), piece});
			continue;
		}
		if (!piece.empty() && std::isdigit(static_cast<unsigned char>(piece[0])))
		{
			tokens.push_back({TokenType::Number, std::stoi(piece), Operator::Add, piece});
			continue;
		}
		tokens.push_back({TokenType::Variable, 0, Operator::Add, piece});
	}
	return tokens;
}
