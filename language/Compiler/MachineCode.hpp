#pragma once
#include "Compiler/Assembler.hpp"
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

// A single compiled translation unit, before linking. Intra-unit jumps are
// already resolved to LOCAL instruction indices; CALL targets are left as
// relocations to be patched by the linker against the merged symbol table.
struct ObjectCode {
    std::vector<uint32_t>                    code;     // encoded instructions
    std::vector<std::pair<std::string, int>> symbols;  // FUNC_BEGIN name -> local index
    std::vector<std::pair<int, std::string>> relocs;   // CALL instr index -> callee name
};

class MachineCode {
public:
    // Single-file path: CALL targets resolved locally (unchanged behavior).
    std::vector<uint32_t> generate(const std::vector<AssemblyInstruction>& asmCode);
    // Linkable path: CALLs left unresolved (recorded as relocs), functions exported.
    ObjectCode generateObject(const std::vector<AssemblyInstruction>& asmCode);
    void saveToFile(const std::vector<uint32_t>& binary, const std::string& filename) const;

private:
    static int  regNum(const std::string& reg);
    static bool isPseudo(const std::string& op);
};
