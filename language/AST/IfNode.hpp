#pragma once

#include "AST/ASTNode.hpp"
#include <memory>

struct IfNode : ASTNode
{
	std::unique_ptr<ASTNode> condition;
	std::unique_ptr<ASTNode> trueBranch;
	std::unique_ptr<ASTNode> falseBranch;

	IfNode() : ASTNode(NodeType::If) {}
};
