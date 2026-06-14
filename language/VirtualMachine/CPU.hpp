#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct CPU {
    uint32_t R[8]  = {};
    uint32_t PC    = 0;
    uint32_t SP    = 0;
    uint32_t RA    = 0;
    std::vector<uint8_t> memory;
    bool zero_flag     = false;
    bool negative_flag = false;
    bool running       = true;

    CPU() : memory(4096, 0) { SP = 4095; }
};

class CPUSimulator {
public:
    void loadBinary(const std::vector<uint32_t>& binary);
    void loadProgram(const std::string& filename);
    void run(int maxInstructions = 10000);

    // Stepping / inspection API (used by the debugger).
    bool        step();                              // execute one instruction at PC
    bool        running() const { return cpu.running && cpu.PC < program.size(); }
    const CPU&  state() const { return cpu; }
    std::size_t programSize() const { return program.size(); }
    uint32_t    instrAt(uint32_t pc) const { return pc < program.size() ? program[pc] : 0u; }

private:
    CPU cpu;
    std::vector<uint32_t> program;

    void     execute(uint32_t instr);
    void     push(uint32_t value);
    uint32_t pop();
};
