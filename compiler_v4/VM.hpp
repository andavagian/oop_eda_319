#pragma once

#include <cstdint>
#include <vector>

enum class OpCode : uint8_t
{
	LOAD_NUM,  // regs[dest] = constants[right]
	LOAD_VAR,  // regs[dest] = memory[right]
	STORE_VAR, // memory[dest] = regs[left]
	ADD, SUB, MUL, DIV,
	CMP,       // cmpFlag = regs[left] - regs[right]
	JMP,       // ip = dest (unconditional)
	JE,        // ip = dest if cmpFlag == 0
	JNE,       // ip = dest if cmpFlag != 0
	JG,        // ip = dest if cmpFlag >  0
	JL,        // ip = dest if cmpFlag <  0
	JGE,       // ip = dest if cmpFlag >= 0
	JLE,       // ip = dest if cmpFlag <= 0
	HALT       // stop execution
};

struct Instruction
{
	uint8_t op;
	uint8_t dest;
	uint8_t left;
	uint8_t right;
};

struct VM
{
	std::vector<int>         regs;      // 100 general-purpose registers
	std::vector<int>         memory;    // variable storage, indexed by SymbolTable addr
	std::vector<int>         constants; // constant pool
	std::vector<Instruction> program;
	int                      next    = 0; // next free register
	int                      cmpFlag = 0;
	int                      retReg  = 0; // register holding the return value

	VM() : regs(100, 0), memory(256, 0) {}

	int allocReg()    { return next++; }
	int addConst(int v)
	{
		constants.push_back(v);
		return static_cast<int>(constants.size()) - 1;
	}
};
