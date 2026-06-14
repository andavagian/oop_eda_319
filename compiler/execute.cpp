#include "../common/Token.hpp"
#include "../common/Node.hpp"
#include "../common/State.hpp"
#include "Compiler.hpp"

// Executes all instructions in prog sequentially against the VM register file.
// Returns the value in the last instruction's destination register,
// or 0 if the program is empty (e.g. a bare literal with no operations).
int execute(const std::vector<Instruction>& prog, VM& vm)
{
	for (const auto& i : prog)
	{
		int a = vm.regs[i.left];
		int b = vm.regs[i.right];

		switch (i.op)
		{
		case Operator::Add:
			vm.regs[i.dest] = a + b;
			break;
		case Operator::Sub:
			vm.regs[i.dest] = a - b;
			break;
		case Operator::Mult:
			vm.regs[i.dest] = a * b;
			break;
		case Operator::Div:
			if (b == 0)
				throw std::runtime_error("Division by zero");
			vm.regs[i.dest] = a / b;
			break;
		}
	}

	return prog.empty() ? vm.regs[0] : vm.regs[prog.back().dest];
}
