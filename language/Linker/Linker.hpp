#pragma once
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

// A relocatable object unit on disk (".vobj"). Produced from one source file.
struct ObjectFile {
    std::vector<uint32_t>                    code;
    std::vector<std::pair<std::string, int>> symbols;  // exported FUNC_BEGIN name -> local index
    std::vector<std::pair<int, std::string>> relocs;   // CALL instr index -> callee name
    bool                                     entry = false;  // has top-level / main code
};

// Text-format (".vobj") serialization.
void       writeObject(const std::string& path, const ObjectFile& obj);
ObjectFile readObject(const std::string& path);

// Merge units into one executable image:
//  - the single entry unit is placed first (address 0),
//  - every unit's intra-unit jumps (JMP/BEQZ/BNEZ) are rebased by its base offset,
//  - CALL relocations are patched against the merged global symbol table.
// Throws std::runtime_error on no/multiple entry units, duplicate or undefined symbols.
std::vector<uint32_t> linkObjects(std::vector<ObjectFile> units);
