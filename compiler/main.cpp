#include "../parser_v3/Token.hpp"
#include "../parser_v3/Node.hpp"
#include "../parser_v3/State.hpp"
#include "Compiler.hpp"

#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

Node* parser(std::vector<Token>& tokens, std::vector<int>& vars);

static void clear(Node* node)
{
	if (!node)
		return;
	clear(node->left);
	clear(node->right);
	delete node;
}

int main()
{
	Node* tree = nullptr;
	try
	{
		std::string expression;
		std::cout << "Enter expression: ";
		std::getline(std::cin, expression);

		std::vector<int> variables;
		std::cout << "Enter variables (space-separated integers, Ctrl+D when done):\n";
		int value;
		while (std::cin >> value)
			variables.push_back(value);

		std::stringstream line(expression);
		std::vector<std::string> v      = lexer(line);
		std::vector<Token>       tokens = tokenizer(v);

		tree = parser(tokens, variables);

		VM                       vm;
		std::vector<Instruction> program;

		compile(tree, program, vm);
		std::cout << "\nValue: " << execute(program, vm) << "\n";

		clear(tree);
	}
	catch (const std::exception& e)
	{
		clear(tree);
		std::cerr << "Error: " << e.what() << "\n";
		return 1;
	}
	return 0;
}
