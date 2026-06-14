#include "Compiler/MachineCode.hpp"
#include <fstream>
#include <stdexcept>
#include <unordered_map>

static const std::unordered_map<std::string, uint8_t> OPCODES = {
    {"ADD",  1},  {"SUB",   2}, {"MUL",  3}, {"DIV",  4}, {"MOD",  5},
    {"NEG",  6},  {"NOT",   7}, {"LI",   8}, {"MOV",  9},
    {"LOAD", 10}, {"STORE", 11},
    {"JMP",  12}, {"BEQZ", 13}, {"BNEZ", 14}, {"CMP", 15},
    {"CALL", 16}, {"RET",  17}, {"PUSH", 18}, {"POP", 19},
    {"OUT",  20}, {"AND",  21}, {"OR",   22}, {"HALT", 255}
};

int MachineCode::regNum(const std::string& reg) {
    if (reg.size() < 2 || reg[0] != 'R') return 0;
    try { return std::stoi(reg.substr(1)) & 7; }
    catch (...) { return 0; }
}

bool MachineCode::isPseudo(const std::string& op) {
    return (op == "LABEL" || op == "FUNC_BEGIN");
}

std::vector<uint32_t> MachineCode::generate(const std::vector<AssemblyInstruction>& asmCode) {
    // Pass 1: build label → instruction-index map (pseudo-instructions have no index)
    std::unordered_map<std::string, int> labelMap;
    int idx = 0;
    for (const auto& instr : asmCode) {
        if (isPseudo(instr.op)) {
            // LABEL and FUNC_BEGIN mark the address of the next real instruction
            labelMap[instr.rd] = idx;
        } else {
            idx++;
        }
    }

    // Pass 2: encode each real instruction
    std::vector<uint32_t> binary;
    for (const auto& instr : asmCode) {
        if (isPseudo(instr.op)) continue;

        static const std::unordered_map<std::string, int> COMP_TYPES = {
            {"==", 0}, {"!=", 1}, {"<", 2}, {">", 3}, {"<=", 4}, {">=", 5}
        };

        uint8_t opcode = 0;
        if (instr.op.size() > 4 && instr.op.substr(0, 4) == "CMP_") {
            opcode = 15;
        } else {
            auto it = OPCODES.find(instr.op);
            if (it == OPCODES.end()) continue;   // unknown op: skip silently
            opcode = it->second;
        }

        int rd  = regNum(instr.rd);
        int rs1 = regNum(instr.rs1);
        int rs2 = regNum(instr.rs2);
        int imm = 0;

        if (instr.op.size() > 4 && instr.op.substr(0, 4) == "CMP_") {
            // Encode comparison type in immediate: ==0 !=1 <2 >3 <=4 >=5
            auto ct = COMP_TYPES.find(instr.op.substr(4));
            imm = (ct != COMP_TYPES.end()) ? ct->second : 0;
        } else if (instr.op == "LI") {
            // rs1 field holds the literal value as a string
            try { imm = std::stoi(instr.rs1); } catch (...) { imm = 0; }
        } else if (instr.op == "JMP") {
            // rs1 field holds the target label
            auto lab = labelMap.find(instr.rs1);
            if (lab != labelMap.end()) imm = lab->second;
        } else if (instr.op == "BEQZ" || instr.op == "BNEZ") {
            // rd = condition register (already decoded above), rs1 = label
            auto lab = labelMap.find(instr.rs1);
            if (lab != labelMap.end()) imm = lab->second;
        } else if (instr.op == "CALL") {
            // rd field holds the function name; rs2 holds the return-value register
            rd = regNum(instr.rs2);
            auto lab = labelMap.find(instr.rd);
            if (lab != labelMap.end()) imm = lab->second;
        }

        uint32_t word = ((uint32_t)opcode        << 24)
                      | ((uint32_t)(rd  & 7)      << 21)
                      | ((uint32_t)(rs1 & 7)      << 18)
                      | ((uint32_t)(rs2 & 7)      << 15)
                      | ((uint32_t)(imm & 0x7FFF));
        binary.push_back(word);
    }
    return binary;
}

ObjectCode MachineCode::generateObject(const std::vector<AssemblyInstruction>& asmCode) {
    static const std::unordered_map<std::string, int> COMP_TYPES = {
        {"==", 0}, {"!=", 1}, {"<", 2}, {">", 3}, {"<=", 4}, {">=", 5}
    };

    // Pass 1: label -> local index; remember which labels are FUNC_BEGIN (exported).
    std::unordered_map<std::string, int> labelMap;
    ObjectCode obj;
    int idx = 0;
    for (const auto& instr : asmCode) {
        if (isPseudo(instr.op)) {
            labelMap[instr.rd] = idx;
            if (instr.op == "FUNC_BEGIN")
                obj.symbols.emplace_back(instr.rd, idx);   // export the function symbol
        } else {
            idx++;
        }
    }

    // Pass 2: encode. CALL targets are NOT resolved here — recorded as relocations.
    for (const auto& instr : asmCode) {
        if (isPseudo(instr.op)) continue;

        uint8_t opcode = 0;
        bool isCmp = (instr.op.size() > 4 && instr.op.substr(0, 4) == "CMP_");
        if (isCmp) {
            opcode = 15;
        } else {
            auto it = OPCODES.find(instr.op);
            if (it == OPCODES.end()) continue;
            opcode = it->second;
        }

        int rd  = regNum(instr.rd);
        int rs1 = regNum(instr.rs1);
        int rs2 = regNum(instr.rs2);
        int imm = 0;

        if (isCmp) {
            auto ct = COMP_TYPES.find(instr.op.substr(4));
            imm = (ct != COMP_TYPES.end()) ? ct->second : 0;
        } else if (instr.op == "LI") {
            try { imm = std::stoi(instr.rs1); } catch (...) { imm = 0; }
        } else if (instr.op == "JMP") {
            auto lab = labelMap.find(instr.rs1);
            if (lab != labelMap.end()) imm = lab->second;   // intra-unit: local index
        } else if (instr.op == "BEQZ" || instr.op == "BNEZ") {
            auto lab = labelMap.find(instr.rs1);
            if (lab != labelMap.end()) imm = lab->second;
        } else if (instr.op == "CALL") {
            rd = regNum(instr.rs2);                          // return-value register
            obj.relocs.emplace_back((int)obj.code.size(), instr.rd);  // patched at link time
            imm = 0;
        }

        uint32_t word = ((uint32_t)opcode   << 24)
                      | ((uint32_t)(rd  & 7) << 21)
                      | ((uint32_t)(rs1 & 7) << 18)
                      | ((uint32_t)(rs2 & 7) << 15)
                      | ((uint32_t)(imm & 0x7FFF));
        obj.code.push_back(word);
    }
    return obj;
}

void MachineCode::saveToFile(const std::vector<uint32_t>& binary,
                              const std::string& filename) const {
    std::ofstream out(filename, std::ios::binary);
    if (!out) throw std::runtime_error("Cannot open output file: " + filename);
    out.write(reinterpret_cast<const char*>(binary.data()),
              binary.size() * sizeof(uint32_t));
}
