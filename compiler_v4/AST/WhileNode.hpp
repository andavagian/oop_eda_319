#pragma once

#include "ASTNode.hpp"
#include <memory>

struct WhileNode : ASTNode
{
	std::unique_ptr<ASTNode> condition;
	std::unique_ptr<ASTNode> body;

	WhileNode() : ASTNode(NodeType::While) {}
};
