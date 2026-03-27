#include "Node.hpp"

int main(int argc, char** argv)
{
	if (argc == 1)
	{
		std::cout << "Usage: ./expr \"expression\" [var=value ...]\n";
		std::cout << "  e.g: ./expr \"(5+7)/2\"\n";
		std::cout << "  e.g: ./expr \"(a-4)*9\" a=7\n";
		return 1;
	}

	Node* tree = parse_expr(argv[1]);
	if (!tree)
		return 1;

	std::map<std::string, int> vars;
	for (int i = 2; i < argc; i++)
		parse_vars(argv[i], vars);

	try
	{
		std::cout << eval_tree(tree, vars) << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << "\n";
		destroy_tree(tree);
		return 1;
	}

	destroy_tree(tree);
	return 0;
}
