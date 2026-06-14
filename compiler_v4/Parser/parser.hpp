#pragma once

#include "ASTNode.hpp"
#include "BlockNode.hpp"
#include "FuncNode.hpp"
#include "SymbolTable.hpp"
#include "Token.hpp"
#include <memory>
#include <vector>

using Tokens = std::vector<Token>;

std::unique_ptr<ASTNode>    parseStatement(const Tokens& toks, size_t& pos, SymbolTable& sym);
std::unique_ptr<BlockNode>  parseBlock    (const Tokens& toks, size_t& pos, SymbolTable& sym);
std::unique_ptr<FuncNode>   parseFunc     (const Tokens& toks, size_t& pos, SymbolTable& sym);
