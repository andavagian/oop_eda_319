#pragma once
#include "Compiler/Assembler.hpp"
#include <cstdint>
#include <string>
#include <vector>

class MachineCode {
public:
    std::vector<uint32_t> generate(const std::vector<AssemblyInstruction>& asmCode);
    void saveToFile(const std::vector<uint32_t>& binary, const std::string& filename) const;

private:
    static int  regNum(const std::string& reg);
    static bool isPseudo(const std::string& op);
};
