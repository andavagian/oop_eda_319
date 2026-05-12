#pragma once

#include "ASTNode.hpp"
#include <memory>
#include <string>

struct NumNode : ASTNode
{
	int value;
	explicit NumNode(int v) : ASTNode(NodeType::Num), value(v) {}
};

struct VarNode : ASTNode
{
	std::string name;
	int         addr;
	VarNode(std::string n, int a) : ASTNode(NodeType::Var), name(std::move(n)), addr(a) {}
};

struct BinOpNode : ASTNode
{
	std::string              op; // "+", "-", "*", "/"
	std::unique_ptr<ASTNode> left;
	std::unique_ptr<ASTNode> right;
	explicit BinOpNode(std::string o) : ASTNode(NodeType::Op), op(std::move(o)) {}
};

struct CompNode : ASTNode
{
	std::string              op; // "==", "!=", ">", "<", ">=", "<="
	std::unique_ptr<ASTNode> left;
	std::unique_ptr<ASTNode> right;
	explicit CompNode(std::string o) : ASTNode(NodeType::Comp), op(std::move(o)) {}
};
