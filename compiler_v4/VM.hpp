#pragma once

#include <string>
#include <unordered_map>
#include <vector>

static constexpr int REGS_PER_FRAME = 256;
static constexpr int MEM_PER_FRAME  = 256;
static constexpr int MAX_CALL_DEPTH = 64;

enum class OpCode
{
	LOAD_NUM,
	LOAD_VAR,
	STORE_VAR,
	ADD, SUB, MUL, DIV, MOD,
	NEG,
	NOT_BOOL, AND_BOOL, OR_BOOL,
	CMP,
	JMP, JE, JNE, JG, JL, JGE, JLE,
	PUSH_ARG, LOAD_ARG,
	CALL, RET,
	PRINT,
	HALT
};

struct Instruction
{
	OpCode op;
	int    dest;
	int    src1;
	int    src2;
};

struct CallFrame
{
	int returnPC;
	int prevRegBase;
	int prevMemBase;
	int retDest;     // absolute register in caller frame to receive return value
};

struct VM
{
	std::vector<int>                     regs;
	std::vector<int>                     memory;
	std::vector<int>                     constants;
	std::vector<Instruction>             program;

	int regBase = 0;
	int memBase = 0;
	int next    = 0;   // relative register allocator (reset per function at compile time)
	int cmpFlag = 0;

	std::vector<CallFrame>               callStack;
	std::vector<int>                     argStack;
	std::unordered_map<std::string, int> funcPCs;

	VM()
		: regs  (REGS_PER_FRAME * MAX_CALL_DEPTH, 0)
		, memory(MEM_PER_FRAME  * MAX_CALL_DEPTH, 0)
	{}

	int allocReg()  { return next++; }
	int addConst(int v)
	{
		constants.push_back(v);
		return static_cast<int>(constants.size()) - 1;
	}
};
