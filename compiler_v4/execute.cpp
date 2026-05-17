#include "execute.hpp"

#include <iostream>
#include <stdexcept>

int execute(VM& vm)
{
	size_t ip = 0;

	while (ip < vm.program.size())
	{
		const Instruction& I  = vm.program[ip];
		const int          rb = vm.regBase;
		const int          mb = vm.memBase;

		// Frame-relative register and memory accessors
		auto R = [&](int rel) -> int& { return vm.regs  [rb + rel]; };
		auto M = [&](int rel) -> int& { return vm.memory[mb + rel]; };

		switch (I.op)
		{
		// ── load / store ──────────────────────────────────────────────────
		case OpCode::LOAD_NUM:
			R(I.dest) = vm.constants[I.src2];
			break;
		case OpCode::LOAD_VAR:
			R(I.dest) = M(I.src2);
			break;
		case OpCode::STORE_VAR:
			M(I.dest) = R(I.src1);
			break;

		// ── arithmetic ────────────────────────────────────────────────────
		case OpCode::ADD:
			R(I.dest) = R(I.src1) + R(I.src2);
			break;
		case OpCode::SUB:
			R(I.dest) = R(I.src1) - R(I.src2);
			break;
		case OpCode::MUL:
			R(I.dest) = R(I.src1) * R(I.src2);
			break;
		case OpCode::DIV:
			if (R(I.src2) == 0)
				throw std::runtime_error("Division by zero");
			R(I.dest) = R(I.src1) / R(I.src2);
			break;
		case OpCode::MOD:
			if (R(I.src2) == 0)
				throw std::runtime_error("Modulo by zero");
			R(I.dest) = R(I.src1) % R(I.src2);
			break;
		case OpCode::NEG:
			R(I.dest) = -R(I.src1);
			break;

		// ── logical ───────────────────────────────────────────────────────
		case OpCode::NOT_BOOL:
			R(I.dest) = (R(I.src1) == 0) ? 1 : 0;
			break;
		case OpCode::AND_BOOL:
			R(I.dest) = (R(I.src1) != 0 && R(I.src2) != 0) ? 1 : 0;
			break;
		case OpCode::OR_BOOL:
			R(I.dest) = (R(I.src1) != 0 || R(I.src2) != 0) ? 1 : 0;
			break;

		// ── comparison ────────────────────────────────────────────────────
		case OpCode::CMP:
			vm.cmpFlag = R(I.src1) - R(I.src2);
			break;

		// ── unconditional / conditional jumps ─────────────────────────────
		case OpCode::JMP:
			ip = (size_t)I.src1;
			continue;
		case OpCode::JE:
			if (vm.cmpFlag == 0) { ip = (size_t)I.src1; continue; }
			break;
		case OpCode::JNE:
			if (vm.cmpFlag != 0) { ip = (size_t)I.src1; continue; }
			break;
		case OpCode::JG:
			if (vm.cmpFlag >  0) { ip = (size_t)I.src1; continue; }
			break;
		case OpCode::JL:
			if (vm.cmpFlag <  0) { ip = (size_t)I.src1; continue; }
			break;
		case OpCode::JGE:
			if (vm.cmpFlag >= 0) { ip = (size_t)I.src1; continue; }
			break;
		case OpCode::JLE:
			if (vm.cmpFlag <= 0) { ip = (size_t)I.src1; continue; }
			break;

		// ── function calling convention ───────────────────────────────────
		case OpCode::PUSH_ARG:
			vm.argStack.push_back(R(I.dest));
			break;
		case OpCode::LOAD_ARG:
			if (vm.argStack.empty())
				throw std::runtime_error("Argument stack underflow");
			R(I.dest) = vm.argStack.back();
			vm.argStack.pop_back();
			break;

		case OpCode::CALL: {
			if (vm.regBase + REGS_PER_FRAME >= (int)vm.regs.size())
				throw std::runtime_error("Stack overflow");
			// Save caller's frame; retDest is absolute in caller's frame
			vm.callStack.push_back({
				(int)(ip + 1),
				vm.regBase,
				vm.memBase,
				vm.regBase + I.dest   // absolute register for the return value
			});
			vm.regBase += REGS_PER_FRAME;
			vm.memBase += MEM_PER_FRAME;
			ip = (size_t)I.src1;
			continue;
		}

		case OpCode::RET: {
			int retVal = R(I.dest);   // callee's relative register
			CallFrame frame = vm.callStack.back();
			vm.callStack.pop_back();
			vm.regBase = frame.prevRegBase;
			vm.memBase = frame.prevMemBase;
			vm.regs[frame.retDest] = retVal;   // write into caller's frame
			ip = (size_t)frame.returnPC;
			continue;
		}

		// ── I/O ───────────────────────────────────────────────────────────
		case OpCode::PRINT:
			std::cout << R(I.dest) << "\n";
			break;

		// ── termination ───────────────────────────────────────────────────
		case OpCode::HALT:
			return vm.regs[vm.regBase + I.dest];
		}
		++ip;
	}
	return 0;
}
