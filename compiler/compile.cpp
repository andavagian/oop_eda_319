#include "../common/Token.hpp"
#include "../common/Node.hpp"
#include "../common/State.hpp"
#include "Compiler.hpp"

// Recursively walks the AST and emits instructions into prog.
// Leaf nodes (Num, Var) are loaded directly into a fresh register.
// Operator nodes emit a binary instruction after compiling both children.
// Returns the register index that holds the result of this subtree.
int compile(Node* node, std::vector<Instruction>& prog, VM& vm)
{
	if (node->type == NodeType::Num)
	{
		int r = vm.next++;
		vm.regs[r] = node->value;
		return r;
	}

	if (node->type == NodeType::Var)
	{
		int r = vm.next++;
		vm.regs[r] = *(node->ptr);
		return r;
	}

	int l    = compile(node->left,  prog, vm);
	int r    = compile(node->right, prog, vm);
	int dest = vm.next++;

	prog.push_back({node->op, dest, l, r});
	return dest;
}
