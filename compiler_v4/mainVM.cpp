#include "BlockNode.hpp"
#include "FuncNode.hpp"
#include "VM.hpp"
#include "compile.hpp"
#include "execute.hpp"
#include "parser.hpp"
#include "tokenizer.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

std::vector<std::string> lexer(std::stringstream& input);

struct Program
{
	std::vector<std::unique_ptr<FuncNode>> funcs;
	std::unique_ptr<BlockNode>             main;
};

static Program buildProgram(const Tokens& tokens)
{
	Program prog;

	size_t pos = 0;

	// Each function gets a fresh SymbolTable so addresses start from 0 per frame
	// Main code uses its own SymbolTable
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
		auto pieces  = lexer(ss);
		auto tokens  = tokenize(pieces);
		auto prog    = buildProgram(tokens);

		VM vm;
		compile(prog.funcs, prog.main.get(), vm);
		int result = execute(vm);
		std::cout << result << "\n";
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << "\n";
		return 1;
	}
	return 0;
}
