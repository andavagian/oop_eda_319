#include "compile.hpp"

#include "AssignNode.hpp"
#include "BlockNode.hpp"
#include "ExprNode.hpp"
#include "FuncNode.hpp"
#include "IfNode.hpp"
#include "ReturnNode.hpp"
#include "WhileNode.hpp"

#include <stdexcept>

static int compileNode(const ASTNode* node, VM& vm);

static int compileNum(const NumNode* n, VM& vm)
{
	int r  = vm.allocReg();
	int ci = vm.addConst(n->value);
	vm.program.push_back({(uint8_t)OpCode::LOAD_NUM, (uint8_t)r, 0, (uint8_t)ci});
	return r;
}

static int compileVar(const VarNode* n, VM& vm)
{
	int r = vm.allocReg();
	vm.program.push_back({(uint8_t)OpCode::LOAD_VAR, (uint8_t)r, 0, (uint8_t)n->addr});
	return r;
}

static int compileBinOp(const BinOpNode* n, VM& vm)
{
	int lReg = compileNode(n->left.get(),  vm);
	int rReg = compileNode(n->right.get(), vm);
	int dest = vm.allocReg();

	OpCode op;
	if      (n->op == "+") op = OpCode::ADD;
	else if (n->op == "-") op = OpCode::SUB;
	else if (n->op == "*") op = OpCode::MUL;
	else if (n->op == "/") op = OpCode::DIV;
	else throw std::runtime_error("Unknown binary operator: " + n->op);

	vm.program.push_back({(uint8_t)op, (uint8_t)dest, (uint8_t)lReg, (uint8_t)rReg});
	return dest;
}

static int compileComp(const CompNode* n, VM& vm)
{
	int lReg = compileNode(n->left.get(),  vm);
	int rReg = compileNode(n->right.get(), vm);
	vm.program.push_back({(uint8_t)OpCode::CMP, 0, (uint8_t)lReg, (uint8_t)rReg});

	OpCode jmp;
	const std::string& op = n->op;
	if      (op == "==") jmp = OpCode::JNE;
	else if (op == "!=") jmp = OpCode::JE;
	else if (op == ">")  jmp = OpCode::JLE;
	else if (op == "<")  jmp = OpCode::JGE;
	else if (op == ">=") jmp = OpCode::JL;
	else if (op == "<=") jmp = OpCode::JG;
	else throw std::runtime_error("Unknown comparison operator: " + op);

	int jumpIdx = (int)vm.program.size();
	vm.program.push_back({(uint8_t)jmp, 0, 0, 0});
	return jumpIdx;
}

static int compileCondition(const ASTNode* cond, VM& vm)
{
	if (cond->kind == NodeType::Comp)
		return compileComp(static_cast<const CompNode*>(cond), vm);

	int r    = compileNode(cond, vm);
	int zReg = vm.allocReg();
	int zIdx = vm.addConst(0);
	vm.program.push_back({(uint8_t)OpCode::LOAD_NUM, (uint8_t)zReg, 0, (uint8_t)zIdx});
	vm.program.push_back({(uint8_t)OpCode::CMP, 0, (uint8_t)r, (uint8_t)zReg});
	int jumpIdx = (int)vm.program.size();
	vm.program.push_back({(uint8_t)OpCode::JE, 0, 0, 0});
	return jumpIdx;
}

static void compileAssign(const AssignNode* n, VM& vm)
{
	int r = compileNode(n->rhs.get(), vm);
	vm.program.push_back({(uint8_t)OpCode::STORE_VAR, (uint8_t)n->addr, (uint8_t)r, 0});
}

static void compileBlock(const BlockNode* n, VM& vm)
{
	for (const auto& stmt : n->statements)
		compileNode(stmt.get(), vm);
}

static void compileIf(const IfNode* n, VM& vm)
{
	int jumpIdx = compileCondition(n->condition.get(), vm);
	compileNode(n->trueBranch.get(), vm);

	if (n->falseBranch)
	{
		int skipIdx = (int)vm.program.size();
		vm.program.push_back({(uint8_t)OpCode::JMP, 0, 0, 0});

		vm.program[jumpIdx].dest = (uint8_t)vm.program.size();

		compileNode(n->falseBranch.get(), vm);

		vm.program[skipIdx].dest = (uint8_t)vm.program.size();
	}
	else
	{
		vm.program[jumpIdx].dest = (uint8_t)vm.program.size();
	}
}

static void compileWhile(const WhileNode* n, VM& vm)
{
	int loopStart = (int)vm.program.size();
	int jumpIdx   = compileCondition(n->condition.get(), vm);
	compileNode(n->body.get(), vm);
	vm.program.push_back({(uint8_t)OpCode::JMP, (uint8_t)loopStart, 0, 0});
	vm.program[jumpIdx].dest = (uint8_t)vm.program.size();
}

static void compileReturn(const ReturnNode* n, VM& vm)
{
	if (n->expr)
		vm.retReg = compileNode(n->expr.get(), vm);
	vm.program.push_back({(uint8_t)OpCode::HALT, 0, 0, 0});
}

static int compileNode(const ASTNode* node, VM& vm)
{
	switch (node->kind)
	{
	case NodeType::Num:
		return compileNum(static_cast<const NumNode*>(node), vm);
	case NodeType::Var:
		return compileVar(static_cast<const VarNode*>(node), vm);
	case NodeType::Op:
		return compileBinOp(static_cast<const BinOpNode*>(node), vm);
	case NodeType::Assign:
		compileAssign(static_cast<const AssignNode*>(node), vm);
		return -1;
	case NodeType::Block:
		compileBlock(static_cast<const BlockNode*>(node), vm);
		return -1;
	case NodeType::If:
		compileIf(static_cast<const IfNode*>(node), vm);
		return -1;
	case NodeType::While:
		compileWhile(static_cast<const WhileNode*>(node), vm);
		return -1;
	case NodeType::Ret:
		compileReturn(static_cast<const ReturnNode*>(node), vm);
		return -1;
	case NodeType::Func:
		compileNode(static_cast<const FuncNode*>(node)->body.get(), vm);
		return -1;
	default:
		throw std::runtime_error("compileNode: unhandled node kind");
	}
}

void compile(const ASTNode* root, VM& vm)
{
	compileNode(root, vm);
}
