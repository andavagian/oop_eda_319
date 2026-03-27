#include "State.hpp"

#include <map>
#include <stdexcept>

namespace
{
struct ParserState
{
	std::vector<Token>& tokens;
	size_t pos;
	std::map<std::string, size_t> bindings;
	std::vector<int>& vars;
};

Node* parse_expr(ParserState& st);

void clear(Node* node)
{
	if (!node)
		return;
	clear(node->left);
	clear(node->right);
	delete node;
}

bool at_end(const ParserState& st)
{
	return st.pos >= st.tokens.size();
}

bool accept(ParserState& st, TokenType t, Operator op = Operator::Add)
{
	if (at_end(st))
		return false;
	const Token& tok = st.tokens[st.pos];
	if (tok.type != t)
		return false;
	if (t == TokenType::Operator && tok.op != op)
		return false;
	++st.pos;
	return true;
}

Node* make_var(ParserState& st, const std::string& name)
{
	auto it = st.bindings.find(name);
	if (it == st.bindings.end())
	{
		size_t next = st.bindings.size();
		if (next >= st.vars.size())
			throw std::runtime_error("Undefined variable: " + name);
		st.bindings[name] = next;
		it = st.bindings.find(name);
	}
	return new Node(name, &st.vars[it->second]);
}

Node* parse_factor(ParserState& st)
{
	if (at_end(st))
		throw std::runtime_error("Unexpected end of expression");

	const Token& tok = st.tokens[st.pos];
	if (tok.type == TokenType::Number)
	{
		++st.pos;
		return new Node(tok.value);
	}
	if (tok.type == TokenType::Variable)
	{
		++st.pos;
		return make_var(st, tok.text);
	}
	if (accept(st, TokenType::LParen))
	{
		Node* inside = parse_expr(st);
		if (!accept(st, TokenType::RParen))
		{
			clear(inside);
			throw std::runtime_error("Missing closing parenthesis");
		}
		return inside;
	}
	throw std::runtime_error("Unexpected token: " + tok.text);
}

Node* parse_term(ParserState& st)
{
	Node* left = parse_factor(st);
	while (!at_end(st))
	{
		Operator op;
		if (accept(st, TokenType::Operator, Operator::Mult))
			op = Operator::Mult;
		else if (accept(st, TokenType::Operator, Operator::Div))
			op = Operator::Div;
		else
			break;

		Node* right = nullptr;
		try
		{
			right = parse_factor(st);
		}
		catch (...)
		{
			clear(left);
			throw;
		}
		left = new Node(op, left, right);
	}
	return left;
}

Node* parse_expr(ParserState& st)
{
	Node* left = parse_term(st);
	while (!at_end(st))
	{
		Operator op;
		if (accept(st, TokenType::Operator, Operator::Add))
			op = Operator::Add;
		else if (accept(st, TokenType::Operator, Operator::Sub))
			op = Operator::Sub;
		else
			break;

		Node* right = nullptr;
		try
		{
			right = parse_term(st);
		}
		catch (...)
		{
			clear(left);
			throw;
		}
		left = new Node(op, left, right);
	}
	return left;
}
} // namespace

Node* parser(std::vector<Token>& tokens, std::vector<int>& vars)
{
	ParserState st{tokens, 0, std::map<std::string, size_t>(), vars};
	Node* root = parse_expr(st);
	if (st.pos != tokens.size())
	{
		clear(root);
		throw std::runtime_error("Unexpected trailing token: " + tokens[st.pos].text);
	}
	return root;
}
