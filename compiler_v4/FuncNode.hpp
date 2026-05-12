#pragma once

#include "ASTNode.hpp"
#include <memory>
#include <string>
#include <utility>
#include <vector>

struct FuncNode : ASTNode
{
	std::string                              name;
	std::string                              returnType;
	std::vector<std::pair<std::string,std::string>> params; // {type, name}
	std::unique_ptr<ASTNode>                 body;

	FuncNode() : ASTNode(NodeType::Func) {}
};
