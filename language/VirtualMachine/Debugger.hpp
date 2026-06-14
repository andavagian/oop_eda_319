#pragma once
#include "VirtualMachine/CPU.hpp"
#include <cstdint>
#include <set>
#include <string>
#include <vector>

// A small interactive debugger over the CPU simulator: breakpoints, single
// stepping, continue, register/memory inspection and disassembly.
class Debugger {
public:
    void loadBinary(const std::vector<uint32_t>& binary);
    void loadProgram(const std::string& filename);
    void repl();                              // command loop on stdin

    static std::string disasm(uint32_t word); // decode one instruction word

private:
    CPUSimulator          cpu_;
    std::vector<uint32_t> binary_;            // kept for "run from start"
    std::set<uint32_t>    breakpoints_;

    void reset();
    void doContinue();                        // run until breakpoint or HALT
    void doStep(int n);
    void printRegs() const;
    void printMem(uint32_t addr, uint32_t count) const;
    void printDisasm(int n) const;
    void showStatus(const char* why) const;   // print PC + current instruction
};
