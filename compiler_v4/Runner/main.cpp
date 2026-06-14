#include "AssignNode.hpp"
#include "BlockNode.hpp"
#include "ExprNode.hpp"
#include "FuncNode.hpp"
#include "IfNode.hpp"
#include "ReturnNode.hpp"
#include "WhileNode.hpp"
#include "parser.hpp"
#include "tokenizer.hpp"
#include "IR.hpp"
#include "Assembler.hpp"
#include "MachineCode.hpp"
#include "CPU.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

std::vector<std::string> lexer(std::stringstream& input);

struct Program {
    std::vector<std::unique_ptr<FuncNode>> funcs;
    std::unique_ptr<BlockNode>             main;
};

static Program buildProgram(const Tokens& tokens) {
    Program     prog;
    size_t      pos = 0;
    SymbolTable mainSym;
    mainSym.enterScope();
    auto mainBlock = std::make_unique<BlockNode>();

    while (pos < tokens.size()) {
        if (tokens[pos].type == NodeType::Func) {
            SymbolTable funcSym;
            prog.funcs.push_back(parseFunc(tokens, pos, funcSym));
        } else {
            mainBlock->statements.push_back(parseStatement(tokens, pos, mainSym));
        }
    }
    mainSym.exitScope();
    prog.main = std::move(mainBlock);
    return prog;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <source_file> [--dump]\n";
        return 1;
    }

    std::ifstream file(argv[1]);
    if (!file) {
        std::cerr << "Cannot open: " << argv[1] << "\n";
        return 1;
    }

    std::string src((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
    bool dump = (argc >= 3 && std::string(argv[2]) == "--dump");

    try {
        std::stringstream ss(src);
        auto pieces = lexer(ss);
        auto tokens = tokenize(pieces);
        auto prog   = buildProgram(tokens);

        // Stage 1 — IR generation
        SymbolTable sym;
        IR ir(sym);
        auto irCode = ir.generate(prog.funcs, prog.main.get());

        if (dump) {
            std::cout << "=== IR (" << irCode.size() << " instructions) ===\n";
            for (const auto& ins : irCode)
                std::cout << "  " << ins.op
                          << "\t" << ins.arg1
                          << "\t" << ins.arg2
                          << "\t-> " << ins.result << "\n";
            std::cout << "\n";
        }

        // Stage 2 — Assembly
        Assembler assembler;
        auto asmCode = assembler.generate(irCode);

        if (dump) {
            std::cout << "=== Assembly (" << asmCode.size() << " instructions) ===\n";
            for (const auto& ins : asmCode)
                std::cout << "  " << ins.op
                          << "\t" << ins.rd
                          << "\t" << ins.rs1
                          << "\t" << ins.rs2 << "\n";
            std::cout << "\n";
        }

        // Stage 3 — Machine code
        MachineCode mc;
        auto binary = mc.generate(asmCode);
        mc.saveToFile(binary, "program.bin");

        // Stage 4 — CPU simulation
        CPUSimulator cpu;
        cpu.loadBinary(binary);
        cpu.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
