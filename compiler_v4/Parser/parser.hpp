#pragma once

#include "AST/ASTNode.hpp"
#include "AST/BlockNode.hpp"
#include "AST/FuncNode.hpp"
#include "SymbolTable/SymbolTable.hpp"
#include "Tokenizer/Token.hpp"
#include <memory>
#include <vector>

using Tokens = std::vector<Token>;

std::unique_ptr<ASTNode>    parseStatement(const Tokens& toks, size_t& pos, SymbolTable& sym);
std::unique_ptr<BlockNode>  parseBlock    (const Tokens& toks, size_t& pos, SymbolTable& sym);
std::unique_ptr<FuncNode>   parseFunc     (const Tokens& toks, size_t& pos, SymbolTable& sym);
