#include "Compiler/Assembler.hpp"
#include <cctype>

bool Assembler::isNumeric(const std::string& s) const {
    if (s.empty()) return false;
    size_t start = (s[0] == '-') ? 1 : 0;
    if (start >= s.size()) return false;
    for (size_t i = start; i < s.size(); i++)
        if (!std::isdigit((unsigned char)s[i])) return false;
    return true;
}

std::string Assembler::allocReg(const std::string& name) {
    if (name.empty()) return "";
    auto it = regMap_.find(name);
    if (it != regMap_.end()) return it->second;
    std::string reg = "R" + std::to_string(regIndex_ % 8);
    regIndex_++;
    regMap_[name] = reg;
    return reg;
}

std::string Assembler::loadOperand(const std::string& name) {
    if (name.empty()) return "";
    if (isNumeric(name)) {
        std::string reg = "R" + std::to_string(regIndex_ % 8);
        regIndex_++;
        emit("LI", reg, name, "");
        return reg;
    }
    return allocReg(name);
}

void Assembler::emit(const std::string& op, const std::string& rd,
                     const std::string& rs1, const std::string& rs2) {
    code_.push_back({op, rd, rs1, rs2});
}

void Assembler::processInstr(const IRInstruction& ir) {
    const std::string& op = ir.op;

    if (op == "ADD" || op == "SUB" || op == "MUL" || op == "DIV" ||
        op == "MOD" || op == "AND" || op == "OR") {
        std::string rs1 = loadOperand(ir.arg1);
        std::string rs2 = loadOperand(ir.arg2);
        std::string rd  = allocReg(ir.result);
        emit(op, rd, rs1, rs2);
    } else if (op == "NEG" || op == "NOT") {
        std::string rs1 = loadOperand(ir.arg1);
        std::string rd  = allocReg(ir.result);
        emit(op, rd, rs1, "");
    } else if (op.size() > 4 && op.substr(0, 4) == "CMP_") {
        // Pass the full "CMP_<cond>" name through; MachineCode encodes the type in imm.
        std::string rs1 = loadOperand(ir.arg1);
        std::string rs2 = loadOperand(ir.arg2);
        std::string rd  = allocReg(ir.result);
        emit(op, rd, rs1, rs2);
    } else if (op == "ASSIGN") {
        std::string rd = allocReg(ir.result);
        if (isNumeric(ir.arg1))
            emit("LI",  rd, ir.arg1, "");
        else
            emit("MOV", rd, allocReg(ir.arg1), "");
    } else if (op == "LABEL") {
        emit("LABEL", ir.arg1, "", "");
    } else if (op == "JMP") {
        emit("JMP", "", ir.arg2, "");
    } else if (op == "BEQZ") {
        emit("BEQZ", allocReg(ir.arg1), ir.arg2, "");
    } else if (op == "PRINT") {
        emit("OUT", loadOperand(ir.arg1), "", "");
    } else if (op == "PARAM") {
        emit("PUSH", loadOperand(ir.arg1), "", "");
    } else if (op == "CALL") {
        std::string rd = allocReg(ir.result);
        emit("CALL", ir.arg1, ir.arg2, rd);
        emit("MOV",  rd, "R0", "");   // return value is in R0 by convention
    } else if (op == "RET") {
        std::string reg = loadOperand(ir.arg1);
        emit("MOV", "R0", reg, "");   // place return value in R0 before returning
        emit("RET", "R0", "", "");
    } else if (op == "FUNC_BEGIN") {
        emit("FUNC_BEGIN", ir.arg1, "", "");
    } else if (op == "FUNC_END") {
        emit("RET", "R0", "", "");
    } else if (op == "LOAD_PARAM") {
        std::string rd = allocReg(ir.result);
        emit("POP", rd, "", "");
    } else if (op == "HALT") {
        emit("HALT", "", "", "");
    }
}

std::vector<AssemblyInstruction> Assembler::generate(const std::vector<IRInstruction>& ir) {
    code_.clear();
    regMap_.clear();
    regIndex_ = 0;
    for (const auto& instr : ir)
        processInstr(instr);
    return code_;
}
