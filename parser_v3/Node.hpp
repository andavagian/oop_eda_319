#pragma once

#include "NodeType.hpp"

#include <string>

struct Node
{
	NodeType type;
	Operator op;
	int value;
	int* ptr;
	Node* left;
	Node* right;
	std::string name;

	Node(int v)
		: type(NodeType::Num), op(Operator::Add), value(v), ptr(nullptr), left(nullptr), right(nullptr) {}

	Node(const std::string& var_name, int* var_ptr)
		: type(NodeType::Var), op(Operator::Add), value(0), ptr(var_ptr), left(nullptr), right(nullptr), name(var_name) {}

	Node(Operator node_op, Node* l, Node* r)
		: type(NodeType::Op), op(node_op), value(0), ptr(nullptr), left(l), right(r) {}
};
