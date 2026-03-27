#pragma once

#include <iostream>
#include <exception>
#include <map>

enum NodeType
{
	ADD,
	SUB,
	DIV,
	MULTI,
	VAL,
	VAR
};

struct Node
{
	int			value;
	NodeType	type;
	Node*		left;
	Node*		right;
	std::string	var;

	Node(int v, NodeType _type, Node* l, Node* r, const std::string& var_name)
		: value(v), type(_type), left(l), right(r), var(var_name) {}
	Node(int v, NodeType _type, Node* l, Node* r)
		: value(v), type(_type), left(l), right(r) {}
	Node()
		: value(0), type(VAL), left(nullptr), right(nullptr) {}
	Node(int v)
		: value(v), type(VAL), left(nullptr), right(nullptr) {}
	Node(const std::string _var)
		: value(0), type(VAR), left(nullptr), right(nullptr), var(_var) {}
	~Node() {}
};

// Parser
Node*	parse_expr(char* s);
void	parse_vars(char* var, std::map<std::string, int>& vars);

// Evaluator
int		eval_tree(const Node* tree, const std::map<std::string, int>& vars);

// Cleanup
void	destroy_tree(Node* n);
