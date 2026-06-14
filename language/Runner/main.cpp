#include "AST/AssignNode.hpp"
#include "AST/BlockNode.hpp"
#include "AST/ExprNode.hpp"
#include "AST/FuncNode.hpp"
#include "AST/IfNode.hpp"
#include "AST/ReturnNode.hpp"
#include "AST/WhileNode.hpp"
#include "Parser/Parser.hpp"
#include "Tokenizer/Tokenizer.hpp"
#include "IR/IR.hpp"
#include "Compiler/Assembler.hpp"
#include "Compiler/MachineCode.hpp"
#include "VirtualMachine/CPU.hpp"
#include "VirtualMachine/Debugger.hpp"
#include "Linker/Linker.hpp"

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

// Front + middle end: source file -> assembly. Sets hasMain = file has top-level code.
static std::vector<AssemblyInstruction>
compileToAsm(const std::string& path, bool dump, bool& hasMain) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("Cannot open: " + path);
    std::string src((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());

    std::stringstream ss(src);
    auto pieces = lexer(ss);
    auto tokens = tokenize(pieces);
    auto prog   = buildProgram(tokens);
    hasMain     = !prog.main->statements.empty();

    SymbolTable sym;
    IR ir(sym);
    auto irCode = ir.generate(prog.funcs, prog.main.get());

    if (dump) {
        std::cout << "=== IR (" << irCode.size() << " instructions) ===\n";
        for (const auto& ins : irCode)
            std::cout << "  " << ins.op << "\t" << ins.arg1 << "\t"
                      << ins.arg2 << "\t-> " << ins.result << "\n";
        std::cout << "\n";
    }

    Assembler assembler;
    auto asmCode = assembler.generate(irCode);

    if (dump) {
        std::cout << "=== Assembly (" << asmCode.size() << " instructions) ===\n";
        for (const auto& ins : asmCode)
            std::cout << "  " << ins.op << "\t" << ins.rd << "\t"
                      << ins.rs1 << "\t" << ins.rs2 << "\n";
        std::cout << "\n";
    }
    return asmCode;
}

static std::vector<uint32_t> readBinary(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot open program file: " + path);
    std::vector<uint32_t> bin;
    uint32_t w;
    while (in.read(reinterpret_cast<char*>(&w), sizeof(w))) bin.push_back(w);
    return bin;
}

// Pull the value following "-o" out of an argument list.
static std::string findOutput(int argc, char** argv, int from) {
    for (int i = from; i < argc - 1; i++)
        if (std::string(argv[i]) == "-o") return argv[i + 1];
    return "";
}

static void usage(const char* prog) {
    std::cerr <<
      "Usage:\n"
      "  " << prog << " <src> [--dump|--debug]      compile and run (or debug) a single file\n"
      "  " << prog << " compile <src> -o <out.vobj> compile to a linkable object\n"
      "  " << prog << " link <a.vobj> ... -o <bin>  link objects into an executable\n"
      "  " << prog << " run <program.bin>           run a compiled binary\n"
      "  " << prog << " debug <program.bin>         debug a compiled binary\n";
}

int main(int argc, char** argv) {
    if (argc < 2) { usage(argv[0]); return 1; }
    std::string verb = argv[1];

    try {
        // ---- compile <src> -o <out.vobj> ----
        if (verb == "compile") {
            if (argc < 3) { usage(argv[0]); return 1; }
            std::string out = findOutput(argc, argv, 3);
            if (out.empty()) out = std::string(argv[2]) + ".vobj";
            bool hasMain = false;
            auto asmCode = compileToAsm(argv[2], false, hasMain);
            MachineCode mc;
            ObjectCode oc = mc.generateObject(asmCode);
            ObjectFile obj{oc.code, oc.symbols, oc.relocs, hasMain};
            writeObject(out, obj);
            std::cout << "wrote " << out << " (" << oc.code.size()
                      << " instr, " << oc.symbols.size() << " symbols, "
                      << oc.relocs.size() << " relocs, entry=" << hasMain << ")\n";
            return 0;
        }

        // ---- link <objs...> -o <out.bin> ----
        if (verb == "link") {
            std::string out = findOutput(argc, argv, 2);
            if (out.empty()) out = "program.bin";
            std::vector<ObjectFile> units;
            for (int i = 2; i < argc; i++) {
                std::string a = argv[i];
                if (a == "-o") { i++; continue; }
                units.push_back(readObject(a));
            }
            if (units.empty()) { usage(argv[0]); return 1; }
            auto image = linkObjects(std::move(units));
            MachineCode mc;
            mc.saveToFile(image, out);
            std::cout << "linked " << image.size() << " instructions -> " << out << "\n";
            return 0;
        }

        // ---- run <bin> ----
        if (verb == "run") {
            if (argc < 3) { usage(argv[0]); return 1; }
            CPUSimulator cpu;
            cpu.loadBinary(readBinary(argv[2]));
            cpu.run();
            return 0;
        }

        // ---- debug <bin> ----
        if (verb == "debug") {
            if (argc < 3) { usage(argv[0]); return 1; }
            Debugger dbg;
            dbg.loadProgram(argv[2]);
            dbg.repl();
            return 0;
        }

        // ---- default: <src> [--dump|--debug] ----
        std::string flag = (argc >= 3) ? argv[2] : "";
        bool dump  = (flag == "--dump");
        bool debug = (flag == "--debug");

        bool hasMain = false;
        auto asmCode = compileToAsm(argv[1], dump, hasMain);
        MachineCode mc;
        auto binary = mc.generate(asmCode);
        mc.saveToFile(binary, "program.bin");

        if (debug) {
            Debugger dbg;
            dbg.loadBinary(binary);
            dbg.repl();
        } else {
            CPUSimulator cpu;
            cpu.loadBinary(binary);
            cpu.run();
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
