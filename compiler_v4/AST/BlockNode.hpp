#pragma once

#include "ASTNode.hpp"
#include <memory>
#include <vector>

struct BlockNode : ASTNode
{
	std::vector<std::unique_ptr<ASTNode>> statements;

	BlockNode() : ASTNode(NodeType::Block) {}
};
