#pragma once

#include "ASTNode.hpp"
#include <memory>
#include <string>
#include <vector>

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
	std::string              op;
	std::unique_ptr<ASTNode> left;
	std::unique_ptr<ASTNode> right;
	explicit BinOpNode(std::string o) : ASTNode(NodeType::Op), op(std::move(o)) {}
};

struct CompNode : ASTNode
{
	std::string              op;
	std::unique_ptr<ASTNode> left;
	std::unique_ptr<ASTNode> right;
	explicit CompNode(std::string o) : ASTNode(NodeType::Comp), op(std::move(o)) {}
};

// Unary minus and logical not
struct UnaryNode : ASTNode
{
	std::string              op;
	std::unique_ptr<ASTNode> operand;
	explicit UnaryNode(std::string o) : ASTNode(NodeType::Not), op(std::move(o)) {}
};

// Function / built-in call expression
struct CallNode : ASTNode
{
	std::string                           name;
	std::vector<std::unique_ptr<ASTNode>> args;
	explicit CallNode(std::string n) : ASTNode(NodeType::Call), name(std::move(n)) {}
};
