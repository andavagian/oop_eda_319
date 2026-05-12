#pragma once

#include "../parser_v3/NodeType.hpp"
#include <string>

struct Token
{
	NodeType    type;
	std::string text;
	int         value; // populated for Num tokens

	Token(NodeType t, std::string txt, int v = 0)
		: type(t), text(std::move(txt)), value(v) {}
};
