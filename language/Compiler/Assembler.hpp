#pragma once
#include "IR/IR.hpp"
#include <string>
#include <unordered_map>
#include <vector>

struct AssemblyInstruction {
    std::string op, rd, rs1, rs2;
};

class Assembler {
public:
    std::vector<AssemblyInstruction> generate(const std::vector<IRInstruction>& ir);

private:
    std::unordered_map<std::string, std::string> regMap_;
    int regIndex_ = 0;
    std::vector<AssemblyInstruction> code_;

    std::string allocReg(const std::string& name);
    std::string loadOperand(const std::string& name);
    void emit(const std::string& op, const std::string& rd,
              const std::string& rs1, const std::string& rs2);
    bool isNumeric(const std::string& s) const;
    void processInstr(const IRInstruction& ir);
};
