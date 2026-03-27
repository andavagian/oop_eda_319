#pragma once

#include "../parser_v3/NodeType.hpp"
#include "../parser_v3/Node.hpp"

#include <vector>
#include <stdexcept>

struct Instruction
{
	Operator	op;
	int			dest;
	int			left;
	int			right;
};

struct VM
{
	int regs[100] = {0};
	int next = 0;
};

int		compile(Node* node, std::vector<Instruction>& prog, VM& vm);
int		execute(const std::vector<Instruction>& prog, VM& vm);
