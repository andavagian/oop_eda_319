#include "Linker/Linker.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

static const uint32_t IMM_MASK = 0x7FFF;

static uint8_t opcodeOf(uint32_t w) { return (uint8_t)((w >> 24) & 0xFF); }
static uint32_t setImm(uint32_t w, int imm) {
    return (w & ~IMM_MASK) | ((uint32_t)imm & IMM_MASK);
}

void writeObject(const std::string& path, const ObjectFile& obj) {
    std::ofstream out(path);
    if (!out) throw std::runtime_error("Cannot write object file: " + path);
    out << "VOBJ1\n";
    out << "ENTRY " << (obj.entry ? 1 : 0) << "\n";
    out << "CODE " << obj.code.size() << "\n";
    for (uint32_t w : obj.code) out << w << "\n";
    out << "SYM " << obj.symbols.size() << "\n";
    for (const auto& s : obj.symbols) out << s.first << " " << s.second << "\n";
    out << "RELOC " << obj.relocs.size() << "\n";
    for (const auto& r : obj.relocs) out << r.first << " " << r.second << "\n";
}

ObjectFile readObject(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("Cannot read object file: " + path);
    ObjectFile obj;
    std::string magic;
    in >> magic;
    if (magic != "VOBJ1") throw std::runtime_error("Not a .vobj file: " + path);

    std::string tag;
    while (in >> tag) {
        if (tag == "ENTRY") {
            int e = 0; in >> e; obj.entry = (e != 0);
        } else if (tag == "CODE") {
            size_t n = 0; in >> n;
            obj.code.reserve(n);
            for (size_t i = 0; i < n; i++) { uint32_t w; in >> w; obj.code.push_back(w); }
        } else if (tag == "SYM") {
            size_t n = 0; in >> n;
            for (size_t i = 0; i < n; i++) { std::string nm; int idx; in >> nm >> idx; obj.symbols.emplace_back(nm, idx); }
        } else if (tag == "RELOC") {
            size_t n = 0; in >> n;
            for (size_t i = 0; i < n; i++) { int idx; std::string nm; in >> idx >> nm; obj.relocs.emplace_back(idx, nm); }
        } else {
            throw std::runtime_error("Malformed .vobj (unexpected '" + tag + "'): " + path);
        }
    }
    return obj;
}

std::vector<uint32_t> linkObjects(std::vector<ObjectFile> units) {
    // 1. Exactly one entry unit, placed first.
    std::vector<ObjectFile> ordered;
    int entryCount = 0;
    for (auto& u : units) if (u.entry) { entryCount++; }
    if (entryCount == 0)
        throw std::runtime_error("Link error: no entry object (one file must contain top-level/main code)");
    if (entryCount > 1)
        throw std::runtime_error("Link error: multiple entry objects (only one file may contain top-level/main code)");
    for (auto& u : units) if (u.entry)  ordered.push_back(std::move(u));
    for (auto& u : units) if (!u.entry) ordered.push_back(std::move(u));

    // 2. Assign base offsets.
    std::vector<int> base(ordered.size());
    int off = 0;
    for (size_t i = 0; i < ordered.size(); i++) { base[i] = off; off += (int)ordered[i].code.size(); }

    // 3. Build global symbol table (error on duplicates).
    std::unordered_map<std::string, int> globals;
    for (size_t i = 0; i < ordered.size(); i++)
        for (const auto& s : ordered[i].symbols) {
            int addr = base[i] + s.second;
            if (!globals.emplace(s.first, addr).second)
                throw std::runtime_error("Link error: duplicate symbol '" + s.first + "'");
        }

    // 4. Concatenate, rebasing intra-unit jump/branch targets by each unit's base.
    std::vector<uint32_t> image;
    image.reserve(off);
    for (size_t i = 0; i < ordered.size(); i++) {
        for (uint32_t w : ordered[i].code) {
            uint8_t op = opcodeOf(w);
            if (op == 12 || op == 13 || op == 14) {   // JMP, BEQZ, BNEZ
                int target = (int)(w & IMM_MASK) + base[i];
                w = setImm(w, target);
            }
            image.push_back(w);
        }
    }

    // 5. Patch CALL relocations against the global symbol table.
    for (size_t i = 0; i < ordered.size(); i++)
        for (const auto& r : ordered[i].relocs) {
            auto it = globals.find(r.second);
            if (it == globals.end())
                throw std::runtime_error("Link error: undefined symbol '" + r.second + "'");
            int pos = base[i] + r.first;
            image[pos] = setImm(image[pos], it->second);
        }

    return image;
}
