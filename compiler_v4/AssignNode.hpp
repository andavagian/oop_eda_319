#pragma once

#include "ASTNode.hpp"
#include <memory>
#include <string>

struct AssignNode : ASTNode
{
	int                      addr;
	std::string              name;
	std::unique_ptr<ASTNode> rhs;

	AssignNode(int a, std::string n)
		: ASTNode(NodeType::Assign), addr(a), name(std::move(n)) {}
};
