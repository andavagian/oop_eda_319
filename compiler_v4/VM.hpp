#pragma once

#include <cstdint>
#include <vector>

enum class OpCode : uint8_t
{
	LOAD_NUM,
	LOAD_VAR,
	STORE_VAR,
	ADD, SUB, MUL, DIV,
	CMP,
	JMP,
	JE,
	JNE,
	JG,
	JL,
	JGE,
	JLE,
	HALT
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
	std::vector<int>         regs;
	std::vector<int>         memory;
	std::vector<int>         constants;
	std::vector<Instruction> program;
	int                      next    = 0;
	int                      cmpFlag = 0;
	int                      retReg  = 0;

	VM() : regs(100, 0), memory(256, 0) {}

	int allocReg()    { return next++; }
	int addConst(int v)
	{
		constants.push_back(v);
		return static_cast<int>(constants.size()) - 1;
	}
};
