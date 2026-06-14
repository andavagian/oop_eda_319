#pragma once

#include "../common/NodeType.hpp"
#include <string>

struct Token
{
	NodeType    type;
	std::string text;
	int         value;

	Token(NodeType t, std::string txt, int v = 0)
		: type(t), text(std::move(txt)), value(v) {}
};
