#include "VirtualMachine/CPU.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>

void CPUSimulator::push(uint32_t value) {
    cpu.memory[cpu.SP] = value & 0xFF;
    if (cpu.SP > 0) cpu.SP--;
}

uint32_t CPUSimulator::pop() {
    if (cpu.SP < 4095) cpu.SP++;
    return cpu.memory[cpu.SP];
}

void CPUSimulator::execute(uint32_t instr) {
    uint8_t  opcode = (instr >> 24) & 0xFF;
    uint8_t  rd     = (instr >> 21) & 0x07;
    uint8_t  rs1    = (instr >> 18) & 0x07;
    uint8_t  rs2    = (instr >> 15) & 0x07;
    int32_t  imm    = (int32_t)(instr & 0x7FFF);
    if (imm & 0x4000) imm |= (int32_t)0xFFFF8000; // sign-extend 15-bit → 32-bit

    switch (opcode) {
    case 1:   // ADD
        cpu.R[rd] = cpu.R[rs1] + cpu.R[rs2];
        cpu.PC++;
        break;
    case 2:   // SUB
        cpu.R[rd] = cpu.R[rs1] - cpu.R[rs2];
        cpu.PC++;
        break;
    case 3:   // MUL
        cpu.R[rd] = cpu.R[rs1] * cpu.R[rs2];
        cpu.PC++;
        break;
    case 4:   // DIV
        if (cpu.R[rs2] == 0) throw std::runtime_error("Division by zero");
        cpu.R[rd] = cpu.R[rs1] / cpu.R[rs2];
        cpu.PC++;
        break;
    case 5:   // MOD
        if (cpu.R[rs2] == 0) throw std::runtime_error("Modulo by zero");
        cpu.R[rd] = cpu.R[rs1] % cpu.R[rs2];
        cpu.PC++;
        break;
    case 6:   // NEG
        cpu.R[rd] = (uint32_t)(-(int32_t)cpu.R[rs1]);
        cpu.PC++;
        break;
    case 7:   // NOT
        cpu.R[rd] = (cpu.R[rs1] == 0) ? 1u : 0u;
        cpu.PC++;
        break;
    case 8:   // LI  (sign-extended immediate)
        cpu.R[rd] = (uint32_t)imm;
        cpu.PC++;
        break;
    case 9:   // MOV
        cpu.R[rd] = cpu.R[rs1];
        cpu.PC++;
        break;
    case 12:  // JMP
        cpu.PC = (uint32_t)imm;
        break;
    case 13:  // BEQZ
        if (cpu.R[rd] == 0) cpu.PC = (uint32_t)imm;
        else                 cpu.PC++;
        break;
    case 14:  // BNEZ
        if (cpu.R[rd] != 0) cpu.PC = (uint32_t)imm;
        else                 cpu.PC++;
        break;
    case 15: { // CMP — comparison type encoded in imm: ==0 !=1 <2 >3 <=4 >=5
        int32_t a = (int32_t)cpu.R[rs1];
        int32_t b = (int32_t)cpu.R[rs2];
        bool result;
        switch (imm) {
        case 0:  result = (a == b); break;
        case 1:  result = (a != b); break;
        case 2:  result = (a <  b); break;
        case 3:  result = (a >  b); break;
        case 4:  result = (a <= b); break;
        case 5:  result = (a >= b); break;
        default: result = false;    break;
        }
        cpu.R[rd] = result ? 1u : 0u;
        cpu.zero_flag     = (a == b);
        cpu.negative_flag = (a <  b);
        cpu.PC++;
        break;
    }
    case 16:  // CALL
        cpu.RA = cpu.PC + 1;
        cpu.PC = (uint32_t)imm;
        break;
    case 17:  // RET
        cpu.PC = cpu.RA;
        break;
    case 18:  // PUSH
        push(cpu.R[rd]);
        cpu.PC++;
        break;
    case 19:  // POP
        cpu.R[rd] = pop();
        cpu.PC++;
        break;
    case 20:  // OUT
        std::cout << (int32_t)cpu.R[rd] << "\n";
        cpu.PC++;
        break;
    case 21:  // AND
        cpu.R[rd] = cpu.R[rs1] & cpu.R[rs2];
        cpu.PC++;
        break;
    case 22:  // OR
        cpu.R[rd] = cpu.R[rs1] | cpu.R[rs2];
        cpu.PC++;
        break;
    case 255: // HALT
        cpu.running = false;
        break;
    default:
        cpu.PC++;   // unknown opcode: advance and continue
        break;
    }
}

void CPUSimulator::loadBinary(const std::vector<uint32_t>& binary) {
    program = binary;
    cpu = CPU();
}

void CPUSimulator::loadProgram(const std::string& filename) {
    std::ifstream in(filename, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot open program file: " + filename);
    program.clear();
    uint32_t word;
    while (in.read(reinterpret_cast<char*>(&word), sizeof(word)))
        program.push_back(word);
    cpu = CPU();
}

void CPUSimulator::run(int maxInstructions) {
    for (int i = 0; i < maxInstructions && cpu.running; i++) {
        if (cpu.PC >= (uint32_t)program.size()) break;
        execute(program[cpu.PC]);
    }
}
