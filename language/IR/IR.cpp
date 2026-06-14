#include "IR/IR.hpp"
#include "AST/AssignNode.hpp"
#include "AST/ExprNode.hpp"
#include "AST/IfNode.hpp"
#include "AST/ReturnNode.hpp"
#include "AST/WhileNode.hpp"

IR::IR(SymbolTable& sym) : sym_(sym) { (void)sym_; }

std::string IR::newTemp() {
    return "t" + std::to_string(tempCount_++);
}

std::string IR::newLabel() {
    return "L" + std::to_string(labelCount_++);
}

void IR::emit(const std::string& op, const std::string& arg1,
              const std::string& arg2, const std::string& result) {
    code_.push_back({op, arg1, arg2, result});
}

std::string IR::genExpr(const ASTNode* node) {
    switch (node->kind) {
    case NodeType::Num: {
        auto* n = static_cast<const NumNode*>(node);
        return std::to_string(n->value);
    }
    case NodeType::Var: {
        auto* n = static_cast<const VarNode*>(node);
        return n->name;
    }
    case NodeType::Op: {
        auto* n = static_cast<const BinOpNode*>(node);
        std::string l = genExpr(n->left.get());
        std::string r = genExpr(n->right.get());
        std::string irOp;
        if      (n->op == "+")  irOp = "ADD";
        else if (n->op == "-")  irOp = "SUB";
        else if (n->op == "*")  irOp = "MUL";
        else if (n->op == "/")  irOp = "DIV";
        else if (n->op == "%")  irOp = "MOD";
        else if (n->op == "&&") irOp = "AND";
        else if (n->op == "||") irOp = "OR";
        else                     irOp = n->op;
        std::string t = newTemp();
        emit(irOp, l, r, t);
        return t;
    }
    case NodeType::Not: {
        auto* n = static_cast<const UnaryNode*>(node);
        std::string operand = genExpr(n->operand.get());
        std::string t = newTemp();
        emit(n->op == "-" ? "NEG" : "NOT", operand, "", t);
        return t;
    }
    case NodeType::Comp: {
        auto* n = static_cast<const CompNode*>(node);
        std::string l = genExpr(n->left.get());
        std::string r = genExpr(n->right.get());
        std::string t = newTemp();
        emit("CMP_" + n->op, l, r, t);
        return t;
    }
    case NodeType::Assign: {
        auto* n = static_cast<const AssignNode*>(node);
        std::string rhs = genExpr(n->rhs.get());
        emit("ASSIGN", rhs, "", n->name);
        return n->name;
    }
    case NodeType::Call: {
        auto* n = static_cast<const CallNode*>(node);
        if (n->name == "print" || n->name == "println") {
            std::string arg = n->args.empty() ? "" : genExpr(n->args[0].get());
            emit("PRINT", arg, "", "");
            return "";
        }
        for (int i = (int)n->args.size() - 1; i >= 0; --i)
            emit("PARAM", genExpr(n->args[i].get()), "", "");
        std::string t = newTemp();
        emit("CALL", n->name, std::to_string(n->args.size()), t);
        return t;
    }
    default:
        return "";
    }
}

void IR::genStatement(const ASTNode* node) {
    switch (node->kind) {
    case NodeType::Block:
        genBlock(static_cast<const BlockNode*>(node));
        break;
    case NodeType::If: {
        auto* n = static_cast<const IfNode*>(node);
        std::string cond = genExpr(n->condition.get());
        if (n->falseBranch) {
            std::string lElse = newLabel();
            std::string lEnd  = newLabel();
            emit("BEQZ",  cond,  lElse, "");
            genStatement(n->trueBranch.get());
            emit("JMP",   "",    lEnd,  "");
            emit("LABEL", lElse, "",    "");
            genStatement(n->falseBranch.get());
            emit("LABEL", lEnd,  "",    "");
        } else {
            std::string lEnd = newLabel();
            emit("BEQZ",  cond, lEnd, "");
            genStatement(n->trueBranch.get());
            emit("LABEL", lEnd, "",   "");
        }
        break;
    }
    case NodeType::While: {
        auto* n = static_cast<const WhileNode*>(node);
        std::string lStart = newLabel();
        std::string lEnd   = newLabel();
        emit("LABEL", lStart, "", "");
        std::string cond = genExpr(n->condition.get());
        emit("BEQZ",  cond,   lEnd,   "");
        genStatement(n->body.get());
        emit("JMP",   "",     lStart, "");
        emit("LABEL", lEnd,   "",     "");
        break;
    }
    case NodeType::Ret: {
        auto* n = static_cast<const ReturnNode*>(node);
        std::string val = n->expr ? genExpr(n->expr.get()) : "0";
        emit("RET", val, "", "");
        break;
    }
    default:
        genExpr(node);
        break;
    }
}

void IR::genBlock(const BlockNode* block) {
    for (const auto& stmt : block->statements)
        genStatement(stmt.get());
}

void IR::genFunc(const FuncNode* func) {
    emit("FUNC_BEGIN", func->name, "", "");
    for (const auto& param : func->params)
        emit("LOAD_PARAM", param.second, "", param.second);
    genStatement(func->body.get());
    emit("FUNC_END", "", "", "");
}

std::vector<IRInstruction> IR::generate(
    const std::vector<std::unique_ptr<FuncNode>>& funcs,
    const BlockNode* mainBlock)
{
    code_.clear();
    tempCount_  = 0;
    labelCount_ = 0;

    // If functions are present, jump over them so main code runs first.
    std::string mainEntry = newLabel();
    if (!funcs.empty())
        emit("JMP", "", mainEntry, "");

    for (const auto& func : funcs)
        genFunc(func.get());

    if (!funcs.empty())
        emit("LABEL", mainEntry, "", "");

    if (mainBlock)
        genBlock(mainBlock);

    emit("HALT", "", "", "");
    return code_;
}
