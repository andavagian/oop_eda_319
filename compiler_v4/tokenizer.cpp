#include "tokenizer.hpp"

#include <cctype>

std::vector<Token> tokenize(const std::vector<std::string>& pieces)
{
	std::vector<Token> tokens;
	for (const std::string& p : pieces)
	{
		if (p == "if")     { tokens.emplace_back(NodeType::If,      p); continue; }
		if (p == "else")   { tokens.emplace_back(NodeType::Else,    p); continue; }
		if (p == "while")  { tokens.emplace_back(NodeType::While,   p); continue; }
		if (p == "return") { tokens.emplace_back(NodeType::Ret,     p); continue; }
		if (p == "Func")   { tokens.emplace_back(NodeType::Func,    p); continue; }
		if (p == "int" || p == "void")
		                   { tokens.emplace_back(NodeType::Type,    p); continue; }
		if (p == "{")      { tokens.emplace_back(NodeType::OpBody,  p); continue; }
		if (p == "}")      { tokens.emplace_back(NodeType::ClBody,  p); continue; }
		if (p == ";")      { tokens.emplace_back(NodeType::Semi,    p); continue; }
		if (p == "=")      { tokens.emplace_back(NodeType::Assign,  p); continue; }
		if (p == "(")      { tokens.emplace_back(NodeType::OpBr,    p); continue; }
		if (p == ")")      { tokens.emplace_back(NodeType::ClBr,    p); continue; }
		if (p == ",")      { tokens.emplace_back(NodeType::Comma,   p); continue; }
		if (p == "!")      { tokens.emplace_back(NodeType::Not,     p); continue; }
		if (p == "+" || p == "-" || p == "*" || p == "/" || p == "%" ||
		    p == "&&"     || p == "||")
		                   { tokens.emplace_back(NodeType::Op,      p); continue; }
		if (p == "==" || p == "!=" || p == ">" || p == "<" ||
		    p == ">=" || p == "<=")
		                   { tokens.emplace_back(NodeType::Comp,    p); continue; }
		if (!p.empty() && std::isdigit(static_cast<unsigned char>(p[0])))
		{
			tokens.emplace_back(NodeType::Num, p, std::stoi(p));
			continue;
		}
		tokens.emplace_back(NodeType::Var, p);
	}
	return tokens;
}
