#pragma once

#include "../common/NodeType.hpp"

struct ASTNode
{
	NodeType kind;
	explicit ASTNode(NodeType k) : kind(k) {}
	virtual ~ASTNode() = default;
};
