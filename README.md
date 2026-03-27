# OOP Expression Projects

This repository contains three small C++ expression projects:

- `tree` - parses an infix expression into a syntax tree and evaluates it
- `parser` - evaluates expressions directly from command-line input
- `compiler` - builds an AST and executes it through a tiny VM

## Requirements

- C++ compiler with C++11 support
- `make`

## Project: `tree`

Build:

```bash
cd tree
make
```

Run:

```bash
./expr "3+4*2"
./expr "(5+7)/2"
./expr "(a-4)*9" a=7
```

Clean:

```bash
make fclean
```

## Project: `parser`

Build:

```bash
cd parser
make
```

Run:

```bash
./expr "3+4*2"
./expr "(5+7)/2"
./expr "(a-4)*9" a=7
```

Clean:

```bash
make fclean
```

## Project: `compiler`

Build:

```bash
cd compiler
make
```

Run (stdin-based):

```bash
./ast
```

Then enter:

1. expression line (example: `(a-4)*9`)
2. optional variable values as space-separated integers (example: `7`)
3. end input with EOF (`Ctrl+D`)

Clean:

```bash
make fclean
```

## Common Error Behavior

All projects are expected to report runtime errors without crashing, including:

- division by zero
- undefined variable usage
