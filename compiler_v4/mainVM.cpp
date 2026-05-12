#include "BlockNode.hpp"
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

		VM vm;
		compile(ast.get(), vm);
		std::cout << execute(vm) << "\n";
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << "\n";
		return 1;
	}
	return 0;
}
