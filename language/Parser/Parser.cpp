#include "Parser/Parser.hpp"

#include "AST/AssignNode.hpp"
#include "AST/ExprNode.hpp"
#include "AST/FuncNode.hpp"
#include "AST/IfNode.hpp"
#include "AST/ReturnNode.hpp"
#include "AST/WhileNode.hpp"

#include <stdexcept>

namespace
{

struct Ctx
{
	const Tokens& toks;
	size_t&       pos;
	SymbolTable&  sym;

	bool at_end() const { return pos >= toks.size(); }

	const Token& peek() const { return toks[pos]; }

	bool check(NodeType t) const { return !at_end() && toks[pos].type == t; }

	Token consume() { return toks[pos++]; }

	Token expect(NodeType t, const char* msg)
	{
		if (at_end() || toks[pos].type != t)
			throw std::runtime_error(msg);
		return toks[pos++];
	}
};

// ── forward declarations ───────────────────────────────────────────────────

static std::unique_ptr<ASTNode>   parseStatement(Ctx& ctx);
static std::unique_ptr<BlockNode> parseBlock    (Ctx& ctx);
static std::unique_ptr<ASTNode>   parseLogical  (Ctx& ctx);
static std::unique_ptr<ASTNode>   parseComp     (Ctx& ctx);
static std::unique_ptr<ASTNode>   parseExpr     (Ctx& ctx);

// ── expression grammar ─────────────────────────────────────────────────────

// parseFactor: number | '(' expr ')' | '-' factor | '!' factor | call | var
static std::unique_ptr<ASTNode> parseFactor(Ctx& ctx)
{
	if (ctx.at_end())
		throw std::runtime_error("Unexpected end of input in expression");

	const Token& tok = ctx.peek();

	// Unary minus
	if (tok.type == NodeType::Op && tok.text == "-")
	{
		ctx.pos++;
		auto operand = parseFactor(ctx);
		auto node = std::make_unique<UnaryNode>("-");
		node->operand = std::move(operand);
		return node;
	}

	// Logical not
	if (tok.type == NodeType::Not)
	{
		ctx.pos++;
		auto operand = parseFactor(ctx);
		auto node = std::make_unique<UnaryNode>("!");
		node->operand = std::move(operand);
		return node;
	}

	// Number literal
	if (tok.type == NodeType::Num)
	{
		ctx.pos++;
		return std::make_unique<NumNode>(tok.value);
	}

	// Variable or function call
	if (tok.type == NodeType::Var)
	{
		ctx.pos++;
		// Function call: identifier followed by '('
		if (!ctx.at_end() && ctx.peek().type == NodeType::OpBr)
		{
			ctx.pos++;   // consume '('
			auto node = std::make_unique<CallNode>(tok.text);
			while (!ctx.at_end() && !ctx.check(NodeType::ClBr))
			{
				node->args.push_back(parseLogical(ctx));
				if (!ctx.at_end() && ctx.peek().type == NodeType::Comma)
					ctx.pos++;
			}
			ctx.expect(NodeType::ClBr, "Expected ')' after arguments");
			return node;
		}
		// Regular variable
		int addr = ctx.sym.lookup(tok.text);
		return std::make_unique<VarNode>(tok.text, addr);
	}

	// Parenthesised expression
	if (tok.type == NodeType::OpBr)
	{
		ctx.pos++;
		auto inner = parseLogical(ctx);
		ctx.expect(NodeType::ClBr, "Expected ')'");
		return inner;
	}

	throw std::runtime_error("Unexpected token in expression: " + tok.text);
}

// parseTerm: factor ( ('*' | '/' | '%') factor )*
static std::unique_ptr<ASTNode> parseTerm(Ctx& ctx)
{
	auto left = parseFactor(ctx);
	while (!ctx.at_end() && ctx.peek().type == NodeType::Op &&
	       (ctx.peek().text == "*" || ctx.peek().text == "/" || ctx.peek().text == "%"))
	{
		std::string op  = ctx.consume().text;
		auto        rhs = parseFactor(ctx);
		auto        node = std::make_unique<BinOpNode>(op);
		node->left  = std::move(left);
		node->right = std::move(rhs);
		left = std::move(node);
	}
	return left;
}

// parseExpr: term ( ('+' | '-') term )*
static std::unique_ptr<ASTNode> parseExpr(Ctx& ctx)
{
	auto left = parseTerm(ctx);
	while (!ctx.at_end() && ctx.peek().type == NodeType::Op &&
	       (ctx.peek().text == "+" || ctx.peek().text == "-"))
	{
		std::string op  = ctx.consume().text;
		auto        rhs = parseTerm(ctx);
		auto        node = std::make_unique<BinOpNode>(op);
		node->left  = std::move(left);
		node->right = std::move(rhs);
		left = std::move(node);
	}
	return left;
}

// parseComp: expr ( comp_op expr )?
static std::unique_ptr<ASTNode> parseComp(Ctx& ctx)
{
	auto left = parseExpr(ctx);
	if (!ctx.at_end() && ctx.peek().type == NodeType::Comp)
	{
		std::string op  = ctx.consume().text;
		auto        rhs = parseExpr(ctx);
		auto        node = std::make_unique<CompNode>(op);
		node->left  = std::move(left);
		node->right = std::move(rhs);
		return node;
	}
	return left;
}

// parseAnd: comp ( '&&' comp )*
static std::unique_ptr<ASTNode> parseAnd(Ctx& ctx)
{
	auto left = parseComp(ctx);
	while (!ctx.at_end() && ctx.peek().type == NodeType::Op &&
	       ctx.peek().text == "&&")
	{
		ctx.pos++;
		auto rhs  = parseComp(ctx);
		auto node = std::make_unique<BinOpNode>("&&");
		node->left  = std::move(left);
		node->right = std::move(rhs);
		left = std::move(node);
	}
	return left;
}

// parseLogical: and ( '||' and )*
static std::unique_ptr<ASTNode> parseLogical(Ctx& ctx)
{
	auto left = parseAnd(ctx);
	while (!ctx.at_end() && ctx.peek().type == NodeType::Op &&
	       ctx.peek().text == "||")
	{
		ctx.pos++;
		auto rhs  = parseAnd(ctx);
		auto node = std::make_unique<BinOpNode>("||");
		node->left  = std::move(left);
		node->right = std::move(rhs);
		left = std::move(node);
	}
	return left;
}

// ── statement parsers ──────────────────────────────────────────────────────

static std::unique_ptr<BlockNode> parseBlock(Ctx& ctx)
{
	ctx.expect(NodeType::OpBody, "Expected '{'");
	ctx.sym.enterScope();

	auto block = std::make_unique<BlockNode>();
	while (!ctx.at_end() && !ctx.check(NodeType::ClBody))
		block->statements.push_back(parseStatement(ctx));

	ctx.expect(NodeType::ClBody, "Expected '}'");
	ctx.sym.exitScope();
	return block;
}

static std::unique_ptr<ASTNode> parseIf(Ctx& ctx)
{
	ctx.pos++;   // consume 'if'
	ctx.expect(NodeType::OpBr, "Expected '(' after 'if'");

	auto node = std::make_unique<IfNode>();
	node->condition = parseLogical(ctx);
	ctx.expect(NodeType::ClBr, "Expected ')' after condition");
	node->trueBranch = parseBlock(ctx);

	if (!ctx.at_end() && ctx.peek().type == NodeType::Else)
	{
		ctx.pos++;
		if (!ctx.at_end() && ctx.peek().type == NodeType::If)
			node->falseBranch = parseIf(ctx);
		else
			node->falseBranch = parseBlock(ctx);
	}
	return node;
}

static std::unique_ptr<ASTNode> parseWhile(Ctx& ctx)
{
	ctx.pos++;   // consume 'while'
	ctx.expect(NodeType::OpBr, "Expected '(' after 'while'");

	auto node = std::make_unique<WhileNode>();
	node->condition = parseLogical(ctx);
	ctx.expect(NodeType::ClBr, "Expected ')' after condition");
	node->body = parseBlock(ctx);
	return node;
}

static std::unique_ptr<ASTNode> parseVarDecl(Ctx& ctx)
{
	ctx.pos++;   // consume type keyword
	if (ctx.at_end() || ctx.peek().type != NodeType::Var)
		throw std::runtime_error("Expected variable name after type");

	std::string name = ctx.consume().text;
	int         addr = ctx.sym.declare(name);

	auto node = std::make_unique<AssignNode>(addr, name);
	if (!ctx.at_end() && ctx.peek().type == NodeType::Assign)
	{
		ctx.pos++;
		node->rhs = parseLogical(ctx);
	}
	else
	{
		node->rhs = std::make_unique<NumNode>(0);
	}
	ctx.expect(NodeType::Semi, "Expected ';' after variable declaration");
	return node;
}

static std::unique_ptr<ASTNode> parseAssign(Ctx& ctx)
{
	std::string name = ctx.consume().text;
	int         addr = ctx.sym.lookup(name);
	ctx.expect(NodeType::Assign, "Expected '=' in assignment");

	auto node = std::make_unique<AssignNode>(addr, name);
	node->rhs = parseLogical(ctx);
	ctx.expect(NodeType::Semi, "Expected ';' after assignment");
	return node;
}

static std::unique_ptr<ASTNode> parseReturn(Ctx& ctx)
{
	ctx.pos++;   // consume 'return'
	auto node = std::make_unique<ReturnNode>();
	if (!ctx.at_end() && !ctx.check(NodeType::Semi))
		node->expr = parseLogical(ctx);
	ctx.expect(NodeType::Semi, "Expected ';' after return");
	return node;
}

static std::unique_ptr<ASTNode> parseStatement(Ctx& ctx)
{
	if (ctx.at_end())
		throw std::runtime_error("Unexpected end of input");

	switch (ctx.peek().type)
	{
	case NodeType::If:     return parseIf(ctx);
	case NodeType::While:  return parseWhile(ctx);
	case NodeType::Type:   return parseVarDecl(ctx);
	case NodeType::Ret:    return parseReturn(ctx);
	case NodeType::OpBody: return parseBlock(ctx);
	case NodeType::Var:
		// Assignment: identifier '=' …
		if (ctx.pos + 1 < ctx.toks.size() &&
		    ctx.toks[ctx.pos + 1].type == NodeType::Assign)
			return parseAssign(ctx);
		// Expression statement (e.g. function call)
		{
			auto e = parseLogical(ctx);
			ctx.expect(NodeType::Semi, "Expected ';'");
			return e;
		}
	default:
		throw std::runtime_error("Unexpected token: " + ctx.peek().text);
	}
}

}   // anonymous namespace

// ── public API ─────────────────────────────────────────────────────────────

std::unique_ptr<ASTNode> parseStatement(const Tokens& toks, size_t& pos, SymbolTable& sym)
{
	Ctx ctx{toks, pos, sym};
	return parseStatement(ctx);
}

std::unique_ptr<BlockNode> parseBlock(const Tokens& toks, size_t& pos, SymbolTable& sym)
{
	Ctx ctx{toks, pos, sym};
	return parseBlock(ctx);
}

std::unique_ptr<FuncNode> parseFunc(const Tokens& toks, size_t& pos, SymbolTable& sym)
{
	Ctx ctx{toks, pos, sym};
	ctx.expect(NodeType::Func, "Expected 'Func'");

	auto node = std::make_unique<FuncNode>();
	node->returnType = ctx.expect(NodeType::Type, "Expected return type").text;
	node->name       = ctx.expect(NodeType::Var,  "Expected function name").text;
	ctx.expect(NodeType::OpBr, "Expected '('");

	sym.enterScope();
	while (!ctx.at_end() && !ctx.check(NodeType::ClBr))
	{
		ctx.expect(NodeType::Type, "Expected parameter type");
		std::string pn   = ctx.expect(NodeType::Var, "Expected parameter name").text;
		int         addr = sym.declare(pn);
		node->params.push_back({"int", pn});
		node->paramAddrs.push_back(addr);
		if (!ctx.at_end() && ctx.peek().type == NodeType::Comma)
			ctx.pos++;
	}
	ctx.expect(NodeType::ClBr, "Expected ')'");

	node->body = parseBlock(ctx);
	sym.exitScope();
	return node;
}
