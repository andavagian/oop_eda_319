#include "compile.hpp"

#include "AssignNode.hpp"
#include "BlockNode.hpp"
#include "ExprNode.hpp"
#include "FuncNode.hpp"
#include "IfNode.hpp"
#include "ReturnNode.hpp"
#include "WhileNode.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

// ── compilation context ────────────────────────────────────────────────────

struct CompileCtx
{
	VM&  vm;
	bool inFunction = false;
	std::unordered_map<std::string, std::vector<int>> callFixups;
};

// ── forward declarations ───────────────────────────────────────────────────

static int  compileNode     (const ASTNode*  node, CompileCtx& ctx);
static void compileBlock    (const BlockNode* n,   CompileCtx& ctx);
static int  compileCondition(const ASTNode*  cond, CompileCtx& ctx);

// ── expression compilers ───────────────────────────────────────────────────

static int compileNum(const NumNode* n, CompileCtx& ctx)
{
	int r  = ctx.vm.allocReg();
	int ci = ctx.vm.addConst(n->value);
	ctx.vm.program.push_back({OpCode::LOAD_NUM, r, 0, ci});
	return r;
}

static int compileVar(const VarNode* n, CompileCtx& ctx)
{
	int r = ctx.vm.allocReg();
	ctx.vm.program.push_back({OpCode::LOAD_VAR, r, 0, n->addr});
	return r;
}

static int compileBinOp(const BinOpNode* n, CompileCtx& ctx)
{
	const std::string& op = n->op;

	// Short-circuit &&
	if (op == "&&")
	{
		int dest = ctx.vm.allocReg();
		int zReg = ctx.vm.allocReg();
		int zIdx = ctx.vm.addConst(0);
		ctx.vm.program.push_back({OpCode::LOAD_NUM, zReg, 0, zIdx});

		int lReg = compileNode(n->left.get(), ctx);
		ctx.vm.program.push_back({OpCode::CMP, 0, lReg, zReg});
		int jFalse1 = (int)ctx.vm.program.size();
		ctx.vm.program.push_back({OpCode::JE, 0, 0, 0});

		int rReg = compileNode(n->right.get(), ctx);
		ctx.vm.program.push_back({OpCode::CMP, 0, rReg, zReg});
		int jFalse2 = (int)ctx.vm.program.size();
		ctx.vm.program.push_back({OpCode::JE, 0, 0, 0});

		int oneIdx = ctx.vm.addConst(1);
		ctx.vm.program.push_back({OpCode::LOAD_NUM, dest, 0, oneIdx});
		int jEnd = (int)ctx.vm.program.size();
		ctx.vm.program.push_back({OpCode::JMP, 0, 0, 0});

		int falsePC = (int)ctx.vm.program.size();
		ctx.vm.program[jFalse1].src1 = falsePC;
		ctx.vm.program[jFalse2].src1 = falsePC;
		ctx.vm.program.push_back({OpCode::LOAD_NUM, dest, 0, zIdx});
		ctx.vm.program[jEnd].src1 = (int)ctx.vm.program.size();
		return dest;
	}

	// Short-circuit ||
	if (op == "||")
	{
		int dest = ctx.vm.allocReg();
		int zReg = ctx.vm.allocReg();
		int zIdx = ctx.vm.addConst(0);
		ctx.vm.program.push_back({OpCode::LOAD_NUM, zReg, 0, zIdx});

		int lReg = compileNode(n->left.get(), ctx);
		ctx.vm.program.push_back({OpCode::CMP, 0, lReg, zReg});
		int jTrue1 = (int)ctx.vm.program.size();
		ctx.vm.program.push_back({OpCode::JNE, 0, 0, 0});

		int rReg = compileNode(n->right.get(), ctx);
		ctx.vm.program.push_back({OpCode::CMP, 0, rReg, zReg});
		int jTrue2 = (int)ctx.vm.program.size();
		ctx.vm.program.push_back({OpCode::JNE, 0, 0, 0});

		ctx.vm.program.push_back({OpCode::LOAD_NUM, dest, 0, zIdx});
		int jEnd = (int)ctx.vm.program.size();
		ctx.vm.program.push_back({OpCode::JMP, 0, 0, 0});

		int truePC  = (int)ctx.vm.program.size();
		ctx.vm.program[jTrue1].src1 = truePC;
		ctx.vm.program[jTrue2].src1 = truePC;
		int oneIdx = ctx.vm.addConst(1);
		ctx.vm.program.push_back({OpCode::LOAD_NUM, dest, 0, oneIdx});
		ctx.vm.program[jEnd].src1 = (int)ctx.vm.program.size();
		return dest;
	}

	int lReg = compileNode(n->left.get(),  ctx);
	int rReg = compileNode(n->right.get(), ctx);
	int dest = ctx.vm.allocReg();

	OpCode opc;
	if      (op == "+") opc = OpCode::ADD;
	else if (op == "-") opc = OpCode::SUB;
	else if (op == "*") opc = OpCode::MUL;
	else if (op == "/") opc = OpCode::DIV;
	else if (op == "%") opc = OpCode::MOD;
	else throw std::runtime_error("Unknown binary operator: " + op);

	ctx.vm.program.push_back({opc, dest, lReg, rReg});
	return dest;
}

static int compileUnary(const UnaryNode* n, CompileCtx& ctx)
{
	int r    = compileNode(n->operand.get(), ctx);
	int dest = ctx.vm.allocReg();
	if (n->op == "-")
		ctx.vm.program.push_back({OpCode::NEG,      dest, r, 0});
	else if (n->op == "!")
		ctx.vm.program.push_back({OpCode::NOT_BOOL, dest, r, 0});
	else
		throw std::runtime_error("Unknown unary operator: " + n->op);
	return dest;
}

// Compile a CompNode for use as a condition (returns index of the conditional
// jump instruction so the caller can backpatch its target).
static int compileComp(const CompNode* n, CompileCtx& ctx)
{
	int lReg = compileNode(n->left.get(),  ctx);
	int rReg = compileNode(n->right.get(), ctx);
	ctx.vm.program.push_back({OpCode::CMP, 0, lReg, rReg});

	const std::string& op = n->op;
	OpCode jmp;
	if      (op == "==") jmp = OpCode::JNE;
	else if (op == "!=") jmp = OpCode::JE;
	else if (op == ">")  jmp = OpCode::JLE;
	else if (op == "<")  jmp = OpCode::JGE;
	else if (op == ">=") jmp = OpCode::JL;
	else if (op == "<=") jmp = OpCode::JG;
	else throw std::runtime_error("Unknown comparison operator: " + op);

	int jumpIdx = (int)ctx.vm.program.size();
	ctx.vm.program.push_back({jmp, 0, 0, 0});
	return jumpIdx;
}

// Compile a CompNode as a value expression (produces 0 or 1 in a register).
static int compileCompValue(const CompNode* n, CompileCtx& ctx)
{
	int lReg = compileNode(n->left.get(),  ctx);
	int rReg = compileNode(n->right.get(), ctx);
	ctx.vm.program.push_back({OpCode::CMP, 0, lReg, rReg});

	int dest = ctx.vm.allocReg();
	const std::string& op = n->op;
	OpCode skipTrue;   // jump over "true" block when condition is FALSE
	if      (op == "==") skipTrue = OpCode::JNE;
	else if (op == "!=") skipTrue = OpCode::JE;
	else if (op == ">")  skipTrue = OpCode::JLE;
	else if (op == "<")  skipTrue = OpCode::JGE;
	else if (op == ">=") skipTrue = OpCode::JL;
	else if (op == "<=") skipTrue = OpCode::JG;
	else throw std::runtime_error("Unknown comparison operator: " + op);

	int jFalse  = (int)ctx.vm.program.size();
	ctx.vm.program.push_back({skipTrue, 0, 0, 0});
	int oneIdx  = ctx.vm.addConst(1);
	ctx.vm.program.push_back({OpCode::LOAD_NUM, dest, 0, oneIdx});
	int jEnd    = (int)ctx.vm.program.size();
	ctx.vm.program.push_back({OpCode::JMP, 0, 0, 0});
	int falsePC = (int)ctx.vm.program.size();
	ctx.vm.program[jFalse].src1 = falsePC;
	int zeroIdx = ctx.vm.addConst(0);
	ctx.vm.program.push_back({OpCode::LOAD_NUM, dest, 0, zeroIdx});
	ctx.vm.program[jEnd].src1 = (int)ctx.vm.program.size();
	return dest;
}

// Compile an arbitrary condition expression; returns the index of the
// conditional jump that skips the body when condition is false.
static int compileCondition(const ASTNode* cond, CompileCtx& ctx)
{
	if (cond->kind == NodeType::Comp)
		return compileComp(static_cast<const CompNode*>(cond), ctx);

	// Generic: evaluate to value, then compare to zero
	int r    = compileNode(cond, ctx);
	int zReg = ctx.vm.allocReg();
	int zIdx = ctx.vm.addConst(0);
	ctx.vm.program.push_back({OpCode::LOAD_NUM, zReg, 0, zIdx});
	ctx.vm.program.push_back({OpCode::CMP,      0,    r, zReg});
	int jumpIdx = (int)ctx.vm.program.size();
	ctx.vm.program.push_back({OpCode::JE, 0, 0, 0});
	return jumpIdx;
}

static int compileCall(const CallNode* n, CompileCtx& ctx)
{
	// ── built-in: print ──────────────────────────────────────────────────
	if (n->name == "print" || n->name == "println")
	{
		if (n->args.empty())
			throw std::runtime_error(n->name + "() expects at least 1 argument");
		for (const auto& arg : n->args)
		{
			int r = compileNode(arg.get(), ctx);
			ctx.vm.program.push_back({OpCode::PRINT, r, 0, 0});
		}
		return ctx.vm.allocReg();   // dummy; print has no useful return value
	}

	// ── user-defined function call ────────────────────────────────────────
	// Push arguments in REVERSE so callee pops left-to-right
	for (int i = (int)n->args.size() - 1; i >= 0; --i)
	{
		int r = compileNode(n->args[i].get(), ctx);
		ctx.vm.program.push_back({OpCode::PUSH_ARG, r, 0, 0});
	}

	int retDest = ctx.vm.allocReg();

	if (ctx.vm.funcPCs.count(n->name))
	{
		int pc = ctx.vm.funcPCs.at(n->name);
		ctx.vm.program.push_back({OpCode::CALL, retDest, pc, 0});
	}
	else
	{
		// Forward reference — fill in PC after all functions compiled
		int instrIdx = (int)ctx.vm.program.size();
		ctx.callFixups[n->name].push_back(instrIdx);
		ctx.vm.program.push_back({OpCode::CALL, retDest, -1, 0});
	}
	return retDest;
}

// ── statement compilers ────────────────────────────────────────────────────

static void compileAssign(const AssignNode* n, CompileCtx& ctx)
{
	int r = compileNode(n->rhs.get(), ctx);
	ctx.vm.program.push_back({OpCode::STORE_VAR, n->addr, r, 0});
}

static void compileBlock(const BlockNode* n, CompileCtx& ctx)
{
	for (const auto& stmt : n->statements)
		compileNode(stmt.get(), ctx);
}

static void compileIf(const IfNode* n, CompileCtx& ctx)
{
	int jumpIdx = compileCondition(n->condition.get(), ctx);
	compileNode(n->trueBranch.get(), ctx);

	if (n->falseBranch)
	{
		int skipIdx = (int)ctx.vm.program.size();
		ctx.vm.program.push_back({OpCode::JMP, 0, 0, 0});
		ctx.vm.program[jumpIdx].src1 = (int)ctx.vm.program.size();
		compileNode(n->falseBranch.get(), ctx);
		ctx.vm.program[skipIdx].src1 = (int)ctx.vm.program.size();
	}
	else
	{
		ctx.vm.program[jumpIdx].src1 = (int)ctx.vm.program.size();
	}
}

static void compileWhile(const WhileNode* n, CompileCtx& ctx)
{
	int loopStart = (int)ctx.vm.program.size();
	int jumpIdx   = compileCondition(n->condition.get(), ctx);
	compileNode(n->body.get(), ctx);
	ctx.vm.program.push_back({OpCode::JMP, 0, loopStart, 0});
	ctx.vm.program[jumpIdx].src1 = (int)ctx.vm.program.size();
}

static void compileReturn(const ReturnNode* n, CompileCtx& ctx)
{
	if (ctx.inFunction)
	{
		int r;
		if (n->expr)
		{
			r = compileNode(n->expr.get(), ctx);
		}
		else
		{
			r = ctx.vm.allocReg();
			int zi = ctx.vm.addConst(0);
			ctx.vm.program.push_back({OpCode::LOAD_NUM, r, 0, zi});
		}
		ctx.vm.program.push_back({OpCode::RET, r, 0, 0});
	}
	else
	{
		int r;
		if (n->expr)
		{
			r = compileNode(n->expr.get(), ctx);
		}
		else
		{
			r = ctx.vm.allocReg();
			int zi = ctx.vm.addConst(0);
			ctx.vm.program.push_back({OpCode::LOAD_NUM, r, 0, zi});
		}
		ctx.vm.program.push_back({OpCode::HALT, r, 0, 0});
	}
}

// ── dispatch ───────────────────────────────────────────────────────────────

static int compileNode(const ASTNode* node, CompileCtx& ctx)
{
	switch (node->kind)
	{
	case NodeType::Num:
		return compileNum(static_cast<const NumNode*>(node), ctx);
	case NodeType::Var:
		return compileVar(static_cast<const VarNode*>(node), ctx);
	case NodeType::Op:
		return compileBinOp(static_cast<const BinOpNode*>(node), ctx);
	case NodeType::Not:
		return compileUnary(static_cast<const UnaryNode*>(node), ctx);
	case NodeType::Comp:
		return compileCompValue(static_cast<const CompNode*>(node), ctx);
	case NodeType::Call:
		return compileCall(static_cast<const CallNode*>(node), ctx);
	case NodeType::Assign:
		compileAssign(static_cast<const AssignNode*>(node), ctx);
		return -1;
	case NodeType::Block:
		compileBlock(static_cast<const BlockNode*>(node), ctx);
		return -1;
	case NodeType::If:
		compileIf(static_cast<const IfNode*>(node), ctx);
		return -1;
	case NodeType::While:
		compileWhile(static_cast<const WhileNode*>(node), ctx);
		return -1;
	case NodeType::Ret:
		compileReturn(static_cast<const ReturnNode*>(node), ctx);
		return -1;
	default:
		throw std::runtime_error("compileNode: unhandled node kind");
	}
}

// ── function definition ────────────────────────────────────────────────────

static void compileFuncDef(const FuncNode* fn, CompileCtx& ctx)
{
	ctx.vm.funcPCs[fn->name] = (int)ctx.vm.program.size();
	ctx.vm.next = 0;   // fresh register window for this function

	// Prologue: pop arguments from argStack (caller pushed them in reverse order)
	for (int addr : fn->paramAddrs)
	{
		int t = ctx.vm.allocReg();
		ctx.vm.program.push_back({OpCode::LOAD_ARG,  t,    0, 0});
		ctx.vm.program.push_back({OpCode::STORE_VAR, addr, t, 0});
	}

	ctx.inFunction = true;
	compileBlock(static_cast<const BlockNode*>(fn->body.get()), ctx);
	ctx.inFunction = false;

	// Implicit return 0 if execution falls off the end
	int zr = ctx.vm.allocReg();
	int zi = ctx.vm.addConst(0);
	ctx.vm.program.push_back({OpCode::LOAD_NUM, zr, 0, zi});
	ctx.vm.program.push_back({OpCode::RET,      zr, 0, 0});
}

// ── entry point ────────────────────────────────────────────────────────────

void compile(
	const std::vector<std::unique_ptr<FuncNode>>& funcs,
	const BlockNode*                              mainBlock,
	VM&                                           vm)
{
	CompileCtx ctx{vm, false, {}};

	// 1. Jump over all function bodies; patch target after functions are compiled
	int jmpOverFuncs = (int)vm.program.size();
	vm.program.push_back({OpCode::JMP, 0, 0, 0});

	// 2. Compile each function (records PC in vm.funcPCs)
	for (const auto& fn : funcs)
		compileFuncDef(fn.get(), ctx);

	// 3. Patch the initial jump to land at main
	vm.program[jmpOverFuncs].src1 = (int)vm.program.size();

	// 4. Compile main block
	vm.next = 0;
	ctx.inFunction = false;
	compileBlock(mainBlock, ctx);

	// 5. Fallthrough HALT (reached if no explicit top-level return)
	int zr = vm.allocReg();
	int zi = vm.addConst(0);
	vm.program.push_back({OpCode::LOAD_NUM, zr, 0, zi});
	vm.program.push_back({OpCode::HALT,     zr, 0, 0});

	// 6. Resolve forward-reference CALL instructions
	for (auto& [name, indices] : ctx.callFixups)
	{
		if (!vm.funcPCs.count(name))
			throw std::runtime_error("Undefined function: " + name);
		int pc = vm.funcPCs.at(name);
		for (int idx : indices)
			vm.program[idx].src1 = pc;
	}
}
