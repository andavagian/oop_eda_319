#include "AssignNode.hpp"
#include "BlockNode.hpp"
#include "ExprNode.hpp"
#include "FuncNode.hpp"
#include "IfNode.hpp"
#include "ReturnNode.hpp"
#include "WhileNode.hpp"
#include "VM.hpp"
#include "compile.hpp"
#include "parser.hpp"
#include "tokenizer.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

std::vector<std::string> lexer(std::stringstream& input);

// ── AST printer ────────────────────────────────────────────────────────────

static void printAST(const ASTNode* node, int depth = 0)
{
	std::string pad(depth * 2, ' ');
	if (!node) { std::cout << pad << "(null)\n"; return; }

	switch (node->kind)
	{
	case NodeType::Num:
		std::cout << pad << "Num(" << static_cast<const NumNode*>(node)->value << ")\n";
		break;
	case NodeType::Var:
	{
		auto* n = static_cast<const VarNode*>(node);
		std::cout << pad << "Var(" << n->name << " @" << n->addr << ")\n";
		break;
	}
	case NodeType::Op:
	{
		auto* n = static_cast<const BinOpNode*>(node);
		std::cout << pad << "BinOp(" << n->op << ")\n";
		printAST(n->left.get(),  depth + 1);
		printAST(n->right.get(), depth + 1);
		break;
	}
	case NodeType::Not:
	{
		auto* n = static_cast<const UnaryNode*>(node);
		std::cout << pad << "Unary(" << n->op << ")\n";
		printAST(n->operand.get(), depth + 1);
		break;
	}
	case NodeType::Comp:
	{
		auto* n = static_cast<const CompNode*>(node);
		std::cout << pad << "Comp(" << n->op << ")\n";
		printAST(n->left.get(),  depth + 1);
		printAST(n->right.get(), depth + 1);
		break;
	}
	case NodeType::Call:
	{
		auto* n = static_cast<const CallNode*>(node);
		std::cout << pad << "Call(" << n->name << ")\n";
		for (const auto& a : n->args)
			printAST(a.get(), depth + 1);
		break;
	}
	case NodeType::Assign:
	{
		auto* n = static_cast<const AssignNode*>(node);
		std::cout << pad << "Assign(" << n->name << " @" << n->addr << ")\n";
		printAST(n->rhs.get(), depth + 1);
		break;
	}
	case NodeType::Block:
	{
		auto* n = static_cast<const BlockNode*>(node);
		std::cout << pad << "Block\n";
		for (const auto& s : n->statements)
			printAST(s.get(), depth + 1);
		break;
	}
	case NodeType::If:
	{
		auto* n = static_cast<const IfNode*>(node);
		std::cout << pad << "If\n";
		std::cout << pad << "  cond:\n";
		printAST(n->condition.get(),  depth + 2);
		std::cout << pad << "  then:\n";
		printAST(n->trueBranch.get(), depth + 2);
		if (n->falseBranch)
		{
			std::cout << pad << "  else:\n";
			printAST(n->falseBranch.get(), depth + 2);
		}
		break;
	}
	case NodeType::While:
	{
		auto* n = static_cast<const WhileNode*>(node);
		std::cout << pad << "While\n";
		std::cout << pad << "  cond:\n";
		printAST(n->condition.get(), depth + 2);
		std::cout << pad << "  body:\n";
		printAST(n->body.get(),      depth + 2);
		break;
	}
	case NodeType::Ret:
	{
		auto* n = static_cast<const ReturnNode*>(node);
		std::cout << pad << "Return\n";
		if (n->expr) printAST(n->expr.get(), depth + 1);
		break;
	}
	case NodeType::Func:
	{
		auto* n = static_cast<const FuncNode*>(node);
		std::cout << pad << "Func " << n->returnType << " " << n->name << "(";
		for (size_t i = 0; i < n->params.size(); ++i)
		{
			if (i > 0) std::cout << ", ";
			std::cout << n->params[i].first << " " << n->params[i].second;
		}
		std::cout << ")\n";
		printAST(n->body.get(), depth + 1);
		break;
	}
	default:
		std::cout << pad << "<?>\n";
	}
}

// ── disassembler ───────────────────────────────────────────────────────────

static const char* opName(OpCode op)
{
	switch (op)
	{
	case OpCode::LOAD_NUM:  return "LOAD_NUM";
	case OpCode::LOAD_VAR:  return "LOAD_VAR";
	case OpCode::STORE_VAR: return "STORE_VAR";
	case OpCode::ADD:       return "ADD";
	case OpCode::SUB:       return "SUB";
	case OpCode::MUL:       return "MUL";
	case OpCode::DIV:       return "DIV";
	case OpCode::MOD:       return "MOD";
	case OpCode::NEG:       return "NEG";
	case OpCode::NOT_BOOL:  return "NOT_BOOL";
	case OpCode::AND_BOOL:  return "AND_BOOL";
	case OpCode::OR_BOOL:   return "OR_BOOL";
	case OpCode::CMP:       return "CMP";
	case OpCode::JMP:       return "JMP";
	case OpCode::JE:        return "JE";
	case OpCode::JNE:       return "JNE";
	case OpCode::JG:        return "JG";
	case OpCode::JL:        return "JL";
	case OpCode::JGE:       return "JGE";
	case OpCode::JLE:       return "JLE";
	case OpCode::PUSH_ARG:  return "PUSH_ARG";
	case OpCode::LOAD_ARG:  return "LOAD_ARG";
	case OpCode::CALL:      return "CALL";
	case OpCode::RET:       return "RET";
	case OpCode::PRINT:     return "PRINT";
	case OpCode::HALT:      return "HALT";
	}
	return "???";
}

// ── program builder (same logic as mainVM) ─────────────────────────────────

struct Program
{
	std::vector<std::unique_ptr<FuncNode>> funcs;
	std::unique_ptr<BlockNode>             main;
};

static Program buildProgram(const Tokens& tokens)
{
	Program prog;
	size_t  pos = 0;

	SymbolTable mainSym;
	mainSym.enterScope();
	auto mainBlock = std::make_unique<BlockNode>();

	while (pos < tokens.size())
	{
		if (tokens[pos].type == NodeType::Func)
		{
			SymbolTable funcSym;
			prog.funcs.push_back(parseFunc(tokens, pos, funcSym));
		}
		else
		{
			mainBlock->statements.push_back(parseStatement(tokens, pos, mainSym));
		}
	}

	mainSym.exitScope();
	prog.main = std::move(mainBlock);
	return prog;
}

// ── main ───────────────────────────────────────────────────────────────────

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		std::cerr << "Usage: " << argv[0] << " <source_file>\n";
		return 1;
	}

	std::ifstream file(argv[1]);
	if (!file)
	{
		std::cerr << "Cannot open: " << argv[1] << "\n";
		return 1;
	}

	std::string src((std::istreambuf_iterator<char>(file)),
	                 std::istreambuf_iterator<char>());
	try
	{
		std::stringstream ss(src);
		auto pieces = lexer(ss);
		auto tokens = tokenize(pieces);
		auto prog   = buildProgram(tokens);

		std::cout << "=== AST ===\n";
		for (const auto& fn : prog.funcs)
			printAST(fn.get());
		printAST(prog.main.get());

		VM vm;
		compile(prog.funcs, prog.main.get(), vm);

		// Print function addresses
		if (!vm.funcPCs.empty())
		{
			std::cout << "\n=== Functions ===\n";
			for (const auto& [name, pc] : vm.funcPCs)
				std::cout << name << " → PC " << pc << "\n";
		}

		std::cout << "\n=== Bytecode (" << vm.program.size() << " instructions) ===\n";
		for (size_t i = 0; i < vm.program.size(); ++i)
		{
			const Instruction& instr = vm.program[i];
			std::cout << i << "\t" << opName(instr.op)
			          << "\tdest=" << instr.dest
			          << "  src1=" << instr.src1
			          << "  src2=" << instr.src2;
			if (instr.op == OpCode::LOAD_NUM && instr.src2 < (int)vm.constants.size())
				std::cout << "  (const=" << vm.constants[instr.src2] << ")";
			std::cout << "\n";
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << "\n";
		return 1;
	}
	return 0;
}
