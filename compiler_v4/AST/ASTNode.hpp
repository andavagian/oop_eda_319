#pragma once

#include "../parser_v3/NodeType.hpp"

struct ASTNode
{
	NodeType kind;
	explicit ASTNode(NodeType k) : kind(k) {}
	virtual ~ASTNode() = default;
};
