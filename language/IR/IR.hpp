#pragma once
#include "AST/ASTNode.hpp"
#include "AST/BlockNode.hpp"
#include "AST/FuncNode.hpp"
#include "SymbolTable/SymbolTable.hpp"
#include <memory>
#include <string>
#include <vector>

struct IRInstruction {
    std::string op, arg1, arg2, result;
};

class IR {
public:
    explicit IR(SymbolTable& sym);
    std::vector<IRInstruction> generate(
        const std::vector<std::unique_ptr<FuncNode>>& funcs,
        const BlockNode* mainBlock);

private:
    SymbolTable& sym_;
    int tempCount_  = 0;
    int labelCount_ = 0;
    std::vector<IRInstruction> code_;

    std::string newTemp();
    std::string newLabel();
    void emit(const std::string& op, const std::string& arg1,
              const std::string& arg2, const std::string& result);
    std::string genExpr(const ASTNode* node);
    void genStatement(const ASTNode* node);
    void genBlock(const BlockNode* block);
    void genFunc(const FuncNode* func);
};
