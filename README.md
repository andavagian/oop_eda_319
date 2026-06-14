# OOP Expression & Compiler Projects

This repository contains four C++ projects demonstrating a progression from simple expression evaluation to a full bytecode compiler with a virtual machine.

| Project | Description |
|---------|-------------|
| `tree/` | Parses an infix expression into a syntax tree and evaluates it recursively |
| `parser/` | Evaluates expressions directly during recursive-descent parsing (no tree built) |
| `compiler/` | Builds an AST and executes it through a register-based VM (expressions only) |
| `compiler_v4/` | Full compiler: functions, recursion, control flow, variables, I/O |

All projects share `parser_v3/` as a lexing/infrastructure library.

## Requirements

- C++ compiler with C++11 support (`tree`, `parser`, `compiler`)
- C++17 support (`compiler_v4`)
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

## Project: `compiler_v4` — Full Compiler

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
compiler_v4/
├── AST/            ASTNode + node types (Expr/Assign/Block/Func/If/Return/While)
├── Tokenizer/      Token.hpp, tokenizer.{hpp,cpp}
├── SymbolTable/    SymbolTable.{hpp,cpp}
├── Parser/         parser.{hpp,cpp}
├── IR/             IR.{hpp,cpp}              (three-address intermediate code)
├── Compiler/       Assembler + MachineCode   (register allocation → uint32 binary)
├── VirtualMachine/ CPU.{hpp,cpp}             (bytecode CPU simulator)
└── Runner/         main.cpp                  (entry point / stage driver)
```

The lexer (`../parser_v3/lexer.cpp`) and `NodeType` enum stay in `parser_v3/`, shared with `compiler/`.

### Build

```bash
cd compiler_v4 && make
```

Produces a single binary, `compilerv4`.

### Run

```bash
./compilerv4 source            # compile and execute (print output)
./compilerv4 source --dump     # also print the IR and assembly listings
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
lexer      (../parser_v3/lexer.cpp)      → vector<string> tokens
    │
    ▼
tokenizer  (Tokenizer/tokenizer.cpp)     → vector<Token> with NodeType tags
    │
    ▼
parser     (Parser/parser.cpp)           → AST (unique_ptr<ASTNode> tree)
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
