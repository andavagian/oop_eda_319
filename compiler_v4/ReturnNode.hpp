#pragma once

#include "ASTNode.hpp"
#include <memory>

struct ReturnNode : ASTNode
{
	std::unique_ptr<ASTNode> expr; // nullable (bare return)

	ReturnNode() : ASTNode(NodeType::Ret) {}
};
