# OOP Expression & Compiler Projects

This repository contains four C++ projects demonstrating a progression from simple expression evaluation to a full bytecode compiler with a virtual machine.

| Project | Description |
|---------|-------------|
| `tree/` | Parses an infix expression into a syntax tree and evaluates it recursively |
| `parser/` | Evaluates expressions directly during recursive-descent parsing (no tree built) |
| `compiler/` | Builds an AST and executes it through a register-based VM (expressions only) |
| `language/` | Full compiler: functions, recursion, control flow, variables, I/O |

All projects share `common/` as a lexing/infrastructure library.

## Requirements

- C++ compiler with C++11 support (`tree`, `parser`, `compiler`)
- C++17 support (`language`)
- `make`

---

## Project: `tree`

Parses infix expressions into a binary syntax tree and evaluates recursively.

```bash
cd tree && make
./expr "3+4*2"
./expr "(5+7)/2"
./expr "(a-4)*9" a=7
```

---

## Project: `parser`

Evaluates expressions directly during parsing — no tree is built.

```bash
cd parser && make
./expr "3+4*2"
./expr "(a-4)*9" a=7
```

---

## Project: `compiler`

Compiles arithmetic expressions into bytecode and runs them on a simple register VM.

```bash
cd compiler && make
./ast
```

Interactive stdin: enter the expression, then any variable values (one per line), then `Ctrl+D`.

---

## Project: `language` — Full Compiler

A complete compiler pipeline with a bytecode VM that supports:

- Integer arithmetic (`+`, `-`, `*`, `/`, `%`)
- Comparison operators (`==`, `!=`, `<`, `>`, `<=`, `>=`)
- Logical operators (`&&`, `||`, `!`)
- Unary negation (`-x`)
- Variables with lexical scoping
- `if` / `else if` / `else`
- `while` loops
- User-defined functions with parameters and return values
- Recursion (stack-frame based, up to depth 64)
- Built-in `print(expr)` for output

The source is organized into one folder per compiler stage (Vahag-style modular layout):

```
language/
├── AST/            ASTNode + node types (Expr/Assign/Block/Func/If/Return/While)
├── Tokenizer/      Token.hpp, Tokenizer.{hpp,cpp}
├── SymbolTable/    SymbolTable.{hpp,cpp}
├── Parser/         Parser.{hpp,cpp}
├── IR/             IR.{hpp,cpp}              (three-address intermediate code)
├── Compiler/       Assembler + MachineCode   (register allocation → uint32 binary)
├── Linker/         Linker.{hpp,cpp}          (object files + multi-file linking)
├── VirtualMachine/ CPU + Debugger            (bytecode CPU simulator + CLI debugger)
└── Runner/         main.cpp                  (entry point / stage driver)
```

The lexer (`../common/lexer.cpp`) and `NodeType` enum stay in `common/`, shared with `compiler/`.

### Build

```bash
cd language && make
```

Produces a single binary, `langc`.

### Run

```bash
./langc source                        # compile and execute (print output)
./langc source --dump                 # also print the IR and assembly listings
./langc source --debug                # compile, then open the CLI debugger
```

### Multi-file linking

Compile each source to an object (`.vobj`), then link them into one executable:

```bash
./langc compile lib.code -o lib.vobj   # functions only (no top-level code)
./langc compile app.code -o app.vobj   # has top-level code = the entry unit
./langc link app.vobj lib.vobj -o program.bin
./langc run program.bin
```

The linker places the entry unit first, rebases jump targets, and resolves cross-file
function calls against a merged symbol table (errors on undefined/duplicate symbols).

### Debugger

`--debug` (on a source) or `debug <program.bin>` opens an interactive REPL:

```
s [n]        step one (or n) instructions
c            continue to next breakpoint or HALT
r            restart from the beginning
b <addr>     set breakpoint (instruction index); 'b' lists, 'd <addr>' deletes
regs         show registers (R0–R7, PC, SP, RA) and flags
mem <a> [n]  show memory bytes
dis [n]      disassemble n instructions from PC
help / q     help / quit
```

### Language syntax

```
Func int name(int a, int b) {   // function definition
    return a + b;
}

int x = name(3, 4);             // variable declaration with call
int y = 0;                      // variable declaration
y = x * 2;                      // assignment
if (x > 5) { ... } else { ... } // conditional
while (x != 0) { ... }          // loop
print(x);                       // print integer + newline
return x;                       // explicit return value
```

Functions must be defined before the main (top-level) code. Forward calls between functions are resolved automatically.

### Examples

**Fibonacci (recursive):**

```
Func int fib(int n) {
    if (n <= 1) { return n; }
    return fib(n - 1) + fib(n - 2);
}

int i = 0;
while (i <= 10) {
    print(fib(i));
    i = i + 1;
}
return 0;
```

**GCD (iterative):**

```
Func int gcd(int a, int b) {
    while (b != 0) {
        int t = b;
        b = a % b;
        a = t;
    }
    return a;
}
print(gcd(48, 18));
return 0;
```

### Pipeline

```
source file
    │
    ▼
lexer      (../common/lexer.cpp)      → vector<string> tokens
    │
    ▼
tokenizer  (Tokenizer/Tokenizer.cpp)     → vector<Token> with NodeType tags
    │
    ▼
parser     (Parser/Parser.cpp)           → AST (unique_ptr<ASTNode> tree)
    │
    ▼
IR         (IR/IR.cpp)                    → three-address IR (vector<IRInstruction>)
    │
    ▼
assembler  (Compiler/Assembler.cpp)      → register-allocated assembly
    │
    ▼
machine    (Compiler/MachineCode.cpp)    → uint32 bytecode (written to program.bin)
    │
    ▼
CPU sim    (VirtualMachine/CPU.cpp)      → executes bytecode, prints output
```

---

## Common Error Behavior

All projects report runtime errors without crashing, including division by zero, modulo by zero, undefined variables, and stack overflow.
