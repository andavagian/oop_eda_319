#pragma once

#include "AST/ASTNode.hpp"
#include <memory>
#include <vector>

struct BlockNode : ASTNode
{
	std::vector<std::unique_ptr<ASTNode>> statements;

	BlockNode() : ASTNode(NodeType::Block) {}
};
