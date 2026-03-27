#include "Node.hpp"

void destroy_tree(Node* n)
{
	if (!n)
		return;
	destroy_tree(n->left);
	destroy_tree(n->right);
	delete n;
}

static void unexpected(char c)
{
	if (c)
		std::cout << "Unexpected token '" << c << "'\n";
	else
		std::cout << "Unexpected end of input\n";
}

static int accept(char** s, char c)
{
	if (**s == c)
	{
		(*s)++;
		return 1;
	}
	return 0;
}

static int expect(char** s, char c)
{
	if (accept(s, c))
		return 1;
	unexpected(**s);
	return 0;
}

static Node* parse_expr_r(char** s);
static Node* parse_term(char** s);
static Node* parse_factor(char** s);

// factor = number | variable | '(' expr ')'
static Node* parse_factor(char** s)
{
	if (isdigit((unsigned char)**s))
	{
		int value = 0;
		while (isdigit((unsigned char)**s))
		{
			value = value * 10 + (**s - '0');
			(*s)++;
		}
		return new Node(value);
	}
	if (isalpha((unsigned char)**s) || **s == '_')
	{
		std::string var_name;
		while (isalnum((unsigned char)**s) || **s == '_')
		{
			var_name += **s;
			(*s)++;
		}
		return new Node(var_name);
	}
	if (accept(s, '('))
	{
		Node* e = parse_expr_r(s);
		if (!e)
			return nullptr;
		if (!expect(s, ')'))
		{
			destroy_tree(e);
			return nullptr;
		}
		return e;
	}
	unexpected(**s);
	return nullptr;
}

// term = factor (('*' | '/') factor)*
static Node* parse_term(char** s)
{
	Node* left = parse_factor(s);
	if (!left)
		return nullptr;

	while (1)
	{
		if (accept(s, '*'))
		{
			Node* right = parse_factor(s);
			if (!right)
			{
				destroy_tree(left);
				return nullptr;
			}
			left = new Node(0, MULTI, left, right);
		}
		else if (accept(s, '/'))
		{
			Node* right = parse_factor(s);
			if (!right)
			{
				destroy_tree(left);
				return nullptr;
			}
			left = new Node(0, DIV, left, right);
		}
		else
			break;
	}
	return left;
}

// expr = term (('+' | '-') term)*
static Node* parse_expr_r(char** s)
{
	Node* left = parse_term(s);
	if (!left)
		return nullptr;

	while (1)
	{
		if (accept(s, '+'))
		{
			Node* right = parse_term(s);
			if (!right)
			{
				destroy_tree(left);
				return nullptr;
			}
			left = new Node(0, ADD, left, right);
		}
		else if (accept(s, '-'))
		{
			Node* right = parse_term(s);
			if (!right)
			{
				destroy_tree(left);
				return nullptr;
			}
			left = new Node(0, SUB, left, right);
		}
		else
			break;
	}
	return left;
}

Node* parse_expr(char* s)
{
	char* p = s;
	Node* ret = parse_expr_r(&p);
	if (!ret)
		return nullptr;
	if (*p)
	{
		unexpected(*p);
		destroy_tree(ret);
		return nullptr;
	}
	return ret;
}

int eval_tree(const Node* tree, const std::map<std::string, int>& vars)
{
	if (!tree)
		throw std::runtime_error("Null node in tree");

	switch (tree->type)
	{
	case ADD:
		return eval_tree(tree->left, vars) + eval_tree(tree->right, vars);
	case SUB:
		return eval_tree(tree->left, vars) - eval_tree(tree->right, vars);
	case MULTI:
		return eval_tree(tree->left, vars) * eval_tree(tree->right, vars);
	case DIV:
	{
		int right = eval_tree(tree->right, vars);
		if (right == 0)
			throw std::runtime_error("Division by zero");
		return eval_tree(tree->left, vars) / right;
	}
	case VAL:
		return tree->value;
	case VAR:
	{
		auto it = vars.find(tree->var);
		if (it == vars.end())
			throw std::runtime_error("Undefined variable: " + tree->var);
		return it->second;
	}
	}
	return 0;
}

void parse_vars(char* var, std::map<std::string, int>& vars)
{
	std::string name;
	int i = 0;

	if (var[i] != '\0' && (isalpha(var[i]) || var[i] == '_'))
	{
		while (var[i] != '\0' && var[i] != '=')
			name += var[i++];
	}
	else
		return unexpected(var[i]);

	if (var[i] == '\0')
		return unexpected(var[i]);
	i++;

	int value = 0;
	if (isdigit(var[i]))
	{
		while (isdigit(var[i]))
		{
			value = value * 10 + (var[i] - '0');
			i++;
		}
	}
	else
		return unexpected(var[i]);

	vars[name] = value;
}
