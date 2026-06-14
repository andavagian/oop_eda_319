#include "VirtualMachine/Debugger.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

// ---- disassembler ----
std::string Debugger::disasm(uint32_t w) {
    uint8_t op  = (w >> 24) & 0xFF;
    int     rd  = (w >> 21) & 0x07;
    int     rs1 = (w >> 18) & 0x07;
    int     rs2 = (w >> 15) & 0x07;
    int32_t imm = (int32_t)(w & 0x7FFF);
    if (imm & 0x4000) imm |= (int32_t)0xFFFF8000;   // sign-extend 15-bit

    std::ostringstream o;
    auto R = [](int n){ return "R" + std::to_string(n); };
    switch (op) {
    case 1:  o << "ADD  " << R(rd) << ", " << R(rs1) << ", " << R(rs2); break;
    case 2:  o << "SUB  " << R(rd) << ", " << R(rs1) << ", " << R(rs2); break;
    case 3:  o << "MUL  " << R(rd) << ", " << R(rs1) << ", " << R(rs2); break;
    case 4:  o << "DIV  " << R(rd) << ", " << R(rs1) << ", " << R(rs2); break;
    case 5:  o << "MOD  " << R(rd) << ", " << R(rs1) << ", " << R(rs2); break;
    case 6:  o << "NEG  " << R(rd) << ", " << R(rs1); break;
    case 7:  o << "NOT  " << R(rd) << ", " << R(rs1); break;
    case 8:  o << "LI   " << R(rd) << ", " << imm; break;
    case 9:  o << "MOV  " << R(rd) << ", " << R(rs1); break;
    case 10: o << "LOAD " << R(rd) << ", [" << imm << "]"; break;
    case 11: o << "STORE" << R(rd) << ", [" << imm << "]"; break;
    case 12: o << "JMP  " << imm; break;
    case 13: o << "BEQZ " << R(rd) << ", " << imm; break;
    case 14: o << "BNEZ " << R(rd) << ", " << imm; break;
    case 15: {
        static const char* C[] = {"==","!=","<",">","<=",">="};
        const char* c = (imm >= 0 && imm < 6) ? C[imm] : "?";
        o << "CMP  " << R(rd) << ", " << R(rs1) << " " << c << " " << R(rs2);
        break;
    }
    case 16: o << "CALL @" << imm << " -> " << R(rd); break;
    case 17: o << "RET"; break;
    case 18: o << "PUSH " << R(rd); break;
    case 19: o << "POP  " << R(rd); break;
    case 20: o << "OUT  " << R(rd); break;
    case 21: o << "AND  " << R(rd) << ", " << R(rs1) << ", " << R(rs2); break;
    case 22: o << "OR   " << R(rd) << ", " << R(rs1) << ", " << R(rs2); break;
    case 255: o << "HALT"; break;
    default: o << "??? (op " << (int)op << ")"; break;
    }
    return o.str();
}

void Debugger::loadBinary(const std::vector<uint32_t>& binary) {
    binary_ = binary;
    cpu_.loadBinary(binary_);
}

void Debugger::loadProgram(const std::string& filename) {
    std::ifstream in(filename, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot open program file: " + filename);
    binary_.clear();
    uint32_t word;
    while (in.read(reinterpret_cast<char*>(&word), sizeof(word)))
        binary_.push_back(word);
    cpu_.loadBinary(binary_);
}

void Debugger::reset() { cpu_.loadBinary(binary_); }

void Debugger::showStatus(const char* why) const {
    uint32_t pc = cpu_.state().PC;
    std::cout << why << "  PC=" << pc;
    if (pc < cpu_.programSize())
        std::cout << ":  " << disasm(cpu_.instrAt(pc));
    std::cout << "\n";
}

void Debugger::doContinue() {
    int guard = 0;
    do {
        if (!cpu_.running()) break;
        cpu_.step();
        if (++guard > 1000000) { std::cout << "[instruction cap reached]\n"; break; }
    } while (cpu_.running() && !breakpoints_.count(cpu_.state().PC));

    if (!cpu_.running()) std::cout << "[program halted]\n";
    else                 showStatus("[breakpoint]");
}

void Debugger::doStep(int n) {
    for (int i = 0; i < n; i++) {
        if (!cpu_.running()) { std::cout << "[program halted]\n"; return; }
        cpu_.step();
    }
    if (cpu_.running()) showStatus("[step]");
    else                std::cout << "[program halted]\n";
}

void Debugger::printRegs() const {
    const CPU& c = cpu_.state();
    for (int i = 0; i < 8; i++) {
        std::cout << "R" << i << "=" << (int32_t)c.R[i] << "  ";
        if (i == 3) std::cout << "\n";
    }
    std::cout << "\nPC=" << c.PC << "  SP=" << c.SP << "  RA=" << c.RA
              << "  zero=" << c.zero_flag << "  neg=" << c.negative_flag << "\n";
}

void Debugger::printMem(uint32_t addr, uint32_t count) const {
    const CPU& c = cpu_.state();
    for (uint32_t i = 0; i < count; i++) {
        uint32_t a = addr + i;
        if (a >= c.memory.size()) break;
        if (i % 8 == 0) std::cout << "[" << a << "] ";
        std::cout << (int)c.memory[a] << " ";
        if (i % 8 == 7) std::cout << "\n";
    }
    std::cout << "\n";
}

void Debugger::printDisasm(int n) const {
    uint32_t pc = cpu_.state().PC;
    for (int i = 0; i < n; i++) {
        uint32_t a = pc + i;
        if (a >= cpu_.programSize()) break;
        std::cout << (a == pc ? "-> " : "   ") << a << ":  "
                  << disasm(cpu_.instrAt(a)) << "\n";
    }
}

static void help() {
    std::cout <<
      "commands:\n"
      "  s [n]        step one (or n) instruction(s)\n"
      "  c            continue to next breakpoint or HALT\n"
      "  r            restart from the beginning\n"
      "  b <addr>     set breakpoint at instruction index; 'b' alone lists them\n"
      "  d <addr>     delete breakpoint\n"
      "  regs         show registers and flags\n"
      "  mem <a> [n]  show n memory bytes from address a (default 16)\n"
      "  dis [n]      disassemble n instructions from PC (default 6)\n"
      "  help         this help\n"
      "  q            quit\n";
}

void Debugger::repl() {
    std::cout << "langc debugger — " << cpu_.programSize()
              << " instructions loaded. Type 'help'.\n";
    showStatus("[start]");
    std::string line;
    while (true) {
        std::cout << "(dbg) " << std::flush;
        if (!std::getline(std::cin, line)) break;
        std::istringstream ss(line);
        std::string cmd; ss >> cmd;
        if (cmd.empty()) continue;

        if (cmd == "q" || cmd == "quit") {
            break;
        } else if (cmd == "s" || cmd == "step") {
            int n = 1; ss >> n; if (n < 1) n = 1;
            doStep(n);
        } else if (cmd == "c" || cmd == "continue") {
            doContinue();
        } else if (cmd == "r" || cmd == "run") {
            reset(); std::cout << "[reset]\n"; doContinue();
        } else if (cmd == "b" || cmd == "break") {
            uint32_t a;
            if (ss >> a) { breakpoints_.insert(a); std::cout << "breakpoint at " << a << "\n"; }
            else {
                std::cout << "breakpoints:";
                for (uint32_t bp : breakpoints_) std::cout << " " << bp;
                std::cout << (breakpoints_.empty() ? " (none)" : "") << "\n";
            }
        } else if (cmd == "d" || cmd == "delete") {
            uint32_t a; if (ss >> a) { breakpoints_.erase(a); std::cout << "deleted " << a << "\n"; }
        } else if (cmd == "regs" || cmd == "info") {
            printRegs();
        } else if (cmd == "mem") {
            uint32_t a = 0, n = 16; ss >> a; ss >> n;
            printMem(a, n);
        } else if (cmd == "dis") {
            int n = 6; ss >> n; if (n < 1) n = 1;
            printDisasm(n);
        } else if (cmd == "help") {
            help();
        } else {
            std::cout << "unknown command '" << cmd << "' — type 'help'\n";
        }
    }
}
