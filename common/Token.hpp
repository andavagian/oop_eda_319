#pragma once

#include "NodeType.hpp"

#include <string>

enum class TokenType
{
	Number,
	Variable,
	Operator,
	LParen,
	RParen
};

struct Token
{
	TokenType type;
	int value;
	Operator op;
	std::string text;
};
