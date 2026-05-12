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
		std::cout << pad << "Var(" << n->name << " @mem[" << n->addr << "])\n";
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
	case NodeType::Comp:
	{
		auto* n = static_cast<const CompNode*>(node);
		std::cout << pad << "Comp(" << n->op << ")\n";
		printAST(n->left.get(),  depth + 1);
		printAST(n->right.get(), depth + 1);
		break;
	}
	case NodeType::Assign:
	{
		auto* n = static_cast<const AssignNode*>(node);
		std::cout << pad << "Assign(" << n->name << " @mem[" << n->addr << "])\n";
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

static const char* opName(uint8_t op)
{
	switch (static_cast<OpCode>(op))
	{
	case OpCode::LOAD_NUM:  return "LOAD_NUM";
	case OpCode::LOAD_VAR:  return "LOAD_VAR";
	case OpCode::STORE_VAR: return "STORE_VAR";
	case OpCode::ADD:       return "ADD";
	case OpCode::SUB:       return "SUB";
	case OpCode::MUL:       return "MUL";
	case OpCode::DIV:       return "DIV";
	case OpCode::CMP:       return "CMP";
	case OpCode::JMP:       return "JMP";
	case OpCode::JE:        return "JE";
	case OpCode::JNE:       return "JNE";
	case OpCode::JG:        return "JG";
	case OpCode::JL:        return "JL";
	case OpCode::JGE:       return "JGE";
	case OpCode::JLE:       return "JLE";
	case OpCode::HALT:      return "HALT";
	}
	return "???";
}

static std::unique_ptr<ASTNode> buildAST(const Tokens& tokens)
{
	SymbolTable sym;
	size_t pos = 0;

	if (!tokens.empty() && tokens[0].type == NodeType::Func)
		return parseFunc(tokens, pos, sym);

	sym.enterScope();
	auto block = std::make_unique<BlockNode>();
	while (pos < tokens.size())
		block->statements.push_back(parseStatement(tokens, pos, sym));
	sym.exitScope();
	return block;
}

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
		auto ast    = buildAST(tokens);

		std::cout << "=== AST ===\n";
		printAST(ast.get());

		VM vm;
		compile(ast.get(), vm);

		std::cout << "\n=== Bytecode (" << vm.program.size() << " instructions) ===\n";
		for (size_t i = 0; i < vm.program.size(); ++i)
		{
			const Instruction& instr = vm.program[i];
			std::cout << i << "\t" << opName(instr.op)
			          << "\tdest=" << (int)instr.dest
			          << "  left=" << (int)instr.left
			          << "  right=" << (int)instr.right;
			if (static_cast<OpCode>(instr.op) == OpCode::LOAD_NUM)
				std::cout << "  (const=" << vm.constants[instr.right] << ")";
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
