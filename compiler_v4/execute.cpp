#include "execute.hpp"

#include <stdexcept>

int execute(VM& vm)
{
	size_t ip = 0;
	while (ip < vm.program.size())
	{
		const Instruction& instr = vm.program[ip];
		OpCode op = static_cast<OpCode>(instr.op);

		switch (op)
		{
		case OpCode::LOAD_NUM:
			vm.regs[instr.dest] = vm.constants[instr.right];
			break;
		case OpCode::LOAD_VAR:
			vm.regs[instr.dest] = vm.memory[instr.right];
			break;
		case OpCode::STORE_VAR:
			vm.memory[instr.dest] = vm.regs[instr.left];
			break;
		case OpCode::ADD:
			vm.regs[instr.dest] = vm.regs[instr.left] + vm.regs[instr.right];
			break;
		case OpCode::SUB:
			vm.regs[instr.dest] = vm.regs[instr.left] - vm.regs[instr.right];
			break;
		case OpCode::MUL:
			vm.regs[instr.dest] = vm.regs[instr.left] * vm.regs[instr.right];
			break;
		case OpCode::DIV:
			if (vm.regs[instr.right] == 0)
				throw std::runtime_error("Division by zero");
			vm.regs[instr.dest] = vm.regs[instr.left] / vm.regs[instr.right];
			break;
		case OpCode::CMP:
			vm.cmpFlag = vm.regs[instr.left] - vm.regs[instr.right];
			break;
		case OpCode::JMP:
			ip = instr.dest;
			continue;
		case OpCode::JE:
			if (vm.cmpFlag == 0) { ip = instr.dest; continue; }
			break;
		case OpCode::JNE:
			if (vm.cmpFlag != 0) { ip = instr.dest; continue; }
			break;
		case OpCode::JG:
			if (vm.cmpFlag > 0)  { ip = instr.dest; continue; }
			break;
		case OpCode::JL:
			if (vm.cmpFlag < 0)  { ip = instr.dest; continue; }
			break;
		case OpCode::JGE:
			if (vm.cmpFlag >= 0) { ip = instr.dest; continue; }
			break;
		case OpCode::JLE:
			if (vm.cmpFlag <= 0) { ip = instr.dest; continue; }
			break;
		case OpCode::HALT:
			return vm.regs[vm.retReg];
		}
		++ip;
	}
	return vm.regs[vm.retReg];
}
